#include "smm/normalizer.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

#include "smm/error.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace smm {
namespace {

std::string to_lower_ascii(std::string_view text) {
    std::string lower;
    lower.reserve(text.size());
    for (const unsigned char c : text) {
        lower.push_back(static_cast<char>(std::tolower(c)));
    }
    return lower;
}

bool starts_with(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with(std::string_view text, std::string_view suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool contains_any(std::string_view haystack, const std::string_view* needles, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        if (haystack.find(needles[i]) != std::string_view::npos) {
            return true;
        }
    }
    return false;
}

/// True when \`count\` characters starting at \`from\` are all ASCII digits.
/// Mirrors the Rust \`.skip(n).take(m).all(is_ascii_digit)\` idiom, which does not require
/// the digit run to end there - "c12345678.dcx" satisfies the c#### model test.
bool digits_at(std::string_view text, std::size_t from, std::size_t count) {
    if (text.size() < from + count) {
        return false;
    }
    for (std::size_t i = from; i < from + count; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
    }
    return true;
}

/// Joins a relative path with '/' separators, matching how targets are addressed in-game.
std::string to_slash_path(const fs::path& path) {
    std::string result;
    for (const auto& part : path) {
        if (!result.empty()) {
            result.push_back('/');
        }
        result.append(path_to_utf8(part));
    }
    return result;
}

/// Recursively enumerates directories, mirroring walkdir's min/max depth semantics.
/// Symlinked directories are not descended into, otherwise a link cycle would hang the scan.
template <typename Fn>
void for_each_directory(const fs::path& root, unsigned max_depth, Fn&& fn) {
    std::error_code ec;
    fs::recursive_directory_iterator it(
        root, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    while (!ec && it != end) {
        const fs::directory_entry& entry = *it;
        std::error_code type_ec;
        const bool is_dir = entry.is_directory(type_ec);
        if (!type_ec && is_dir && it.depth() <= static_cast<int>(max_depth)) {
            fn(entry.path(), static_cast<unsigned>(it.depth()));
        }
        if (type_ec || (is_dir && it.depth() >= static_cast<int>(max_depth))) {
            it.disable_recursion_pending();
        }
        it.increment(ec);
    }
}

/// Enumerates every regular file below \`root\`.
template <typename Fn>
void for_each_file(const fs::path& root, Fn&& fn) {
    std::error_code ec;
    fs::recursive_directory_iterator it(
        root, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    while (!ec && it != end) {
        const fs::directory_entry& entry = *it;
        std::error_code type_ec;
        if (entry.is_regular_file(type_ec) && !type_ec) {
            fn(entry.path());
        }
        it.increment(ec);
    }
}

} // namespace

AssetCategory Normalizer::classify_asset(std::string_view rel_path) {
    const std::string lower = to_lower_ascii(rel_path);

    if (lower == "dinput8.dll" || lower == "modengine.ini") {
        return AssetCategory::Loader;
    }
    if (starts_with(lower, "parts/")) return AssetCategory::Parts;
    if (starts_with(lower, "chr/")) return AssetCategory::Chr;
    if (starts_with(lower, "obj/")) return AssetCategory::Obj;
    if (starts_with(lower, "param/")) return AssetCategory::Param;
    if (starts_with(lower, "sfx/") || ends_with(lower, ".ffxbnd.dcx") || ends_with(lower, ".fxr")) {
        return AssetCategory::Sfx;
    }
    if (starts_with(lower, "sound/")) return AssetCategory::Sound;
    if (starts_with(lower, "msg/")) return AssetCategory::Msg;
    if (starts_with(lower, "menu/")) {
        if (lower.find("/font/") != std::string::npos || ends_with(lower, ".gfx") ||
            ends_with(lower, ".swf")) {
            return AssetCategory::Font;
        }
        return AssetCategory::Menu;
    }
    if (starts_with(lower, "font/") || ends_with(lower, ".gfx") || ends_with(lower, ".swf")) {
        return AssetCategory::Font;
    }
    if (starts_with(lower, "mtd/")) return AssetCategory::Mtd;
    if (starts_with(lower, "event/")) return AssetCategory::Event;
    if (starts_with(lower, "map/")) return AssetCategory::Map;
    if (starts_with(lower, "action/")) return AssetCategory::Other;
    if (starts_with(lower, "cutscene/")) return AssetCategory::Cutscene;
    if (starts_with(lower, "script/")) return AssetCategory::Script;

    for (const FileSignature& signature : FILE_SIGNATURES) {
        if (ends_with(lower, signature.suffix)) {
            return signature.category;
        }
    }
    return AssetCategory::Other;
}

std::string Normalizer::normalize_relative_path(std::string_view rel_path) {
    std::string rel(rel_path);
    const std::string lower = to_lower_ascii(rel);

    // Authors routinely ship menu assets without their `menu/` parent, and audio with a
    // directory name of their own invention. Both are folded onto the canonical layout here.
    if (starts_with(lower, "hi/") || starts_with(lower, "font/") || starts_with(lower, "low/")) {
        return "menu/" + rel;
    }
    if (starts_with(lower, "sounds/")) {
        return "sound/" + rel.substr(7);
    }
    if (starts_with(lower, "audio/") || starts_with(lower, "voice/")) {
        return "sound/" + rel.substr(6);
    }
    if (starts_with(lower, "voices/")) {
        return "sound/" + rel.substr(7);
    }
    if (starts_with(lower, "music/")) {
        return "sound/" + rel.substr(6);
    }
    if (starts_with(lower, "menu/")) {
        // The shared UI atlas and the solo-menu textures have to live in menu/hi/, while
        // Scaleform movies belong in menu/font/; misplacing either makes them invisible.
        if (lower == "menu/01_common.tpf.dcx" || lower == "menu/01_common.tpf" ||
            starts_with(lower, "menu/00_solo")) {
            return "menu/hi/" + rel.substr(5);
        }
        if ((ends_with(lower, ".gfx") || ends_with(lower, ".swf")) &&
            !starts_with(lower, "menu/font/")) {
            return "menu/font/" + rel.substr(5);
        }
        return rel;
    }

    if (rel.find('/') != std::string::npos) {
        // Nested paths keep whatever canonical structure the author chose; only loose files
        // are re-homed, because guessing a destination for a nested tree does more harm.
        return rel;
    }

    bool is_root_loader = false;
    for (const std::string_view root_file : CANONICAL_ROOT_FILES) {
        if (rel == root_file) {
            is_root_loader = true;
            break;
        }
    }
    if (is_root_loader) {
        return rel;
    }

    if (lower == "01_common.tpf.dcx" || lower == "01_common.tpf" || starts_with(lower, "00_solo")) {
        return "menu/hi/" + rel;
    }
    if (ends_with(lower, ".gfx") || ends_with(lower, ".swf")) {
        return "menu/font/" + rel;
    }

    for (const FileSignature& signature : FILE_SIGNATURES) {
        if (ends_with(lower, signature.suffix)) {
            return std::string(signature.canonical_dir) + "/" + rel;
        }
    }

    // Nothing matched by extension, so fall back to Sekiro's own naming conventions.
    const bool parts_prefix = starts_with(lower, "wp_") || starts_with(lower, "am_") ||
                              starts_with(lower, "bd_") || starts_with(lower, "fc_") ||
                              starts_with(lower, "lg_");
    if (parts_prefix && (ends_with(lower, ".dcx") || ends_with(lower, ".partsbnd") ||
                         ends_with(lower, ".tpf"))) {
        return "parts/" + rel;
    }
    if (starts_with(lower, "c") && digits_at(lower, 1, 4) &&
        (ends_with(lower, ".dcx") || ends_with(lower, ".chrbnd") ||
         ends_with(lower, ".texbnd") || ends_with(lower, ".anibnd"))) {
        return "chr/" + rel;
    }
    if ((starts_with(lower, "sfx") || starts_with(lower, "f000")) &&
        (ends_with(lower, ".dcx") || ends_with(lower, ".ffxbnd") || ends_with(lower, ".fxr"))) {
        return "sfx/" + rel;
    }
    if (starts_with(lower, "o") && digits_at(lower, 1, 6) &&
        (ends_with(lower, ".dcx") || ends_with(lower, ".objbnd"))) {
        return "obj/" + rel;
    }

    return rel;
}

bool Normalizer::is_ignored_file(const fs::path& path, const fs::path& canonical_root) {
    const std::string file_name = path_to_utf8(path.filename());
    const std::string lower = to_lower_ascii(file_name);

    if (!file_name.empty() && file_name.front() == '.') {
        return true;
    }

    // Yabber / Witchy unpacking workspaces are scaffolding, never shippable game data.
    if (starts_with(lower, "_yabber") || starts_with(lower, "_witchy") || ends_with(lower, ".xml")) {
        return true;
    }

    std::error_code ec;
    const fs::path relative = fs::relative(path, canonical_root, ec);
    if (!ec) {
        const std::string rel_lower = to_lower_ascii(to_slash_path(relative));
        constexpr std::string_view kWitchyMarkers[] = {
            "-tpf-dcx",     "-bnd-dcx",    "-partsbnd-dcx", "-chrbnd-dcx", "-menubnd-dcx",
            "-geombnd-dcx", "-ffxbnd-dcx", "-anibnd-dcx",
        };
        if (contains_any(rel_lower, kWitchyMarkers, std::size(kWitchyMarkers))) {
            return true;
        }
        for (const auto& part : relative) {
            const std::string name = path_to_utf8(part);
            if (name.size() > 1 && name.front() == '.') {
                return true;
            }
        }
    }

    if (ends_with(lower, ".zip") || ends_with(lower, ".7z") || ends_with(lower, ".rar") ||
        ends_with(lower, ".tar") || ends_with(lower, ".gz")) {
        return true;
    }

    if (lower == "mod.json" || lower == ".smm_mod.json" || lower == "desktop.ini" ||
        lower == ".ds_store" || lower == "thumbs.db") {
        return true;
    }

    if (starts_with(lower, "readme") || starts_with(lower, "license") ||
        starts_with(lower, "changelog") || ends_with(lower, ".md") || ends_with(lower, ".txt")) {
        return true;
    }

    // Preview screenshots sitting beside the assets are documentation, not game content.
    const bool is_image = ends_with(lower, ".jpg") || ends_with(lower, ".png") ||
                          ends_with(lower, ".jpeg") || ends_with(lower, ".webp") ||
                          ends_with(lower, ".bmp");
    if (is_image && path.parent_path() == canonical_root) {
        return true;
    }

    return false;
}

namespace {

/// True when \`dir\` directly holds at least one canonical directory or root loader file.
std::size_t count_canonical_anchors(const fs::path& dir) {
    std::error_code ec;
    fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        return 0;
    }

    std::size_t count = 0;
    for (const auto& entry : it) {
        const std::string name = to_lower_ascii(path_to_utf8(entry.path().filename()));
        std::error_code type_ec;
        if (entry.is_directory(type_ec) && !type_ec) {
            bool matched = false;
            for (const std::string_view candidate : CANONICAL_DIRS) {
                if (name == candidate) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                for (const std::string_view candidate : UI_SUB_DIRS) {
                    if (name == candidate) {
                        matched = true;
                        break;
                    }
                }
            }
            if (!matched) {
                for (const std::string_view candidate : AUDIO_SUB_DIRS) {
                    if (name == candidate) {
                        matched = true;
                        break;
                    }
                }
            }
            if (matched) {
                ++count;
            }
        } else {
            for (const std::string_view candidate : CANONICAL_ROOT_FILES) {
                if (name == candidate) {
                    ++count;
                    break;
                }
            }
        }
    }
    return count;
}

bool looks_like_asset_file(const std::string& lower_name) {
    if (starts_with(lower_name, "wp_") || starts_with(lower_name, "am_") ||
        starts_with(lower_name, "bd_") || starts_with(lower_name, "fc_") ||
        starts_with(lower_name, "lg_")) {
        if (ends_with(lower_name, ".dcx") || ends_with(lower_name, ".partsbnd") ||
            ends_with(lower_name, ".tpf")) {
            return true;
        }
    }
    if (starts_with(lower_name, "c") && digits_at(lower_name, 1, 4) &&
        (ends_with(lower_name, ".dcx") || ends_with(lower_name, ".chrbnd") ||
         ends_with(lower_name, ".texbnd") || ends_with(lower_name, ".anibnd"))) {
        return true;
    }
    if ((starts_with(lower_name, "sfx") || starts_with(lower_name, "f000")) &&
        (ends_with(lower_name, ".dcx") || ends_with(lower_name, ".ffxbnd") ||
         ends_with(lower_name, ".fxr"))) {
        return true;
    }
    if (starts_with(lower_name, "o") && digits_at(lower_name, 1, 6) &&
        (ends_with(lower_name, ".dcx") || ends_with(lower_name, ".objbnd"))) {
        return true;
    }
    for (const FileSignature& signature : FILE_SIGNATURES) {
        if (ends_with(lower_name, signature.suffix)) {
            return true;
        }
    }
    return false;
}

/// True when \`dir\` directly holds loose files that look like Sekiro assets.
bool contains_signature_files(const fs::path& dir) {
    std::error_code ec;
    fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        return false;
    }
    for (const auto& entry : it) {
        std::error_code type_ec;
        if (!entry.is_regular_file(type_ec) || type_ec) {
            continue;
        }
        if (looks_like_asset_file(to_lower_ascii(path_to_utf8(entry.path().filename())))) {
            return true;
        }
    }
    return false;
}

} // namespace

fs::path Normalizer::find_canonical_root(const fs::path& base_path) {
    if (count_canonical_anchors(base_path) > 0) {
        return base_path;
    }

    // Prefer the directory with the most canonical anchors; when two tie, the shallower one
    // is the mod root and the deeper one is its payload.
    fs::path best;
    std::size_t best_count = 0;
    unsigned best_depth = 0;
    bool have_candidate = false;

    for_each_directory(base_path, 6, [&](const fs::path& dir, unsigned depth) {
        const std::size_t count = count_canonical_anchors(dir);
        if (count == 0) {
            return;
        }
        if (!have_candidate || count > best_count || (count == best_count && depth < best_depth)) {
            best = dir;
            best_count = count;
            best_depth = depth;
            have_candidate = true;
        }
    });
    if (have_candidate) {
        return best;
    }

    // Nothing canonical, but a directory of loose signature files still identifies the root.
    // Shallowest first, so a nested asset folder is not mistaken for the mod root.
    for (unsigned depth = 0; depth <= 4; ++depth) {
        fs::path found;
        bool found_here = false;
        for_each_directory(base_path, 4, [&](const fs::path& dir, unsigned current) {
            if (found_here || current != depth) {
                return;
            }
            if (contains_signature_files(dir)) {
                found = dir;
                found_here = true;
            }
        });
        if (found_here) {
            return found;
        }
        if (depth == 0 && contains_signature_files(base_path)) {
            return base_path;
        }
    }

    fail(ErrorCode::NormalizationError,
         "No canonical Sekiro directories (parts/, chr/, ...) or signature files found under "
         "this path. It does not look like an unpacked Sekiro mod.",
         base_path);
}

NormalizationResult Normalizer::normalize_directory(const fs::path& base_path) {
    std::error_code ec;
    if (!fs::exists(base_path, ec)) {
        fail(ErrorCode::NormalizationError, "Specified path does not exist", base_path);
    }

    NormalizationResult result;
    result.canonical_root = find_canonical_root(base_path);

    for_each_file(result.canonical_root, [&](const fs::path& path) {
        if (is_ignored_file(path, result.canonical_root)) {
            result.ignored_files.push_back(path);
            return;
        }

        std::error_code rel_ec;
        const fs::path relative = fs::relative(path, result.canonical_root, rel_ec);
        if (rel_ec || relative.empty()) {
            return;
        }

        const std::string normalized = normalize_relative_path(to_slash_path(relative));
        const AssetCategory category = classify_asset(normalized);

        std::error_code size_ec;
        const uint64_t size = static_cast<uint64_t>(fs::file_size(path, size_ec));
        if (size_ec) {
            // A file that vanished mid-scan (or is unreadable) is not worth failing the whole
            // mod over; it simply contributes no bytes to the plan.
            return;
        }

        result.assets.emplace_back(normalized, path, size, category);
    });

    std::sort(result.assets.begin(), result.assets.end(),
              [](const AssetEntry& a, const AssetEntry& b) {
                  return a.relative_path < b.relative_path;
              });

    if (result.assets.empty()) {
        fail(ErrorCode::NoAssetsFound, "No deployable Sekiro assets found in this path", base_path);
    }

    return result;
}

} // namespace smm
