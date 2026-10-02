#include "smm/importer.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <iterator>
#include <random>
#include <set>
#include <string>
#include <system_error>
#include <vector>

#include "smm/error.hpp"
#include "smm/extractor.hpp"
#include "smm/loader.hpp"
#include "smm/manager.hpp"
#include "smm/normalizer.hpp"

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

/// Scratch directory that cleans itself up, including on the exception path.
/// Import holds hundreds of megabytes of unpacked data, so leaking it would be visible.
class TempDir {
public:
    explicit TempDir(fs::path path) : path_(std::move(path)) {}
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

/// Creates a uniquely named scratch directory, preferring \`parent\` so the final move into
/// staging stays on one volume and can therefore be an atomic rename.
TempDir make_workspace(const fs::path& parent, std::string_view prefix) {
    const fs::path base = [&] {
        std::error_code ec;
        fs::create_directories(parent, ec);
        if (!ec && fs::is_directory(parent, ec) && !ec) {
            return parent;
        }
        return fs::temp_directory_path(ec);
    }();

    std::random_device device;
    std::mt19937_64 generator(device() ^ static_cast<uint64_t>(
                                              std::chrono::steady_clock::now()
                                                  .time_since_epoch()
                                                  .count()));

    for (int attempt = 0; attempt < 64; ++attempt) {
        char suffix[32] = {};
        std::snprintf(suffix, sizeof(suffix), "%08llx",
                      static_cast<unsigned long long>(generator() & 0xFFFFFFFFULL));
        const fs::path candidate = base / (std::string(prefix) + suffix);
        std::error_code ec;
        if (fs::create_directory(candidate, ec) && !ec) {
            return TempDir(candidate);
        }
    }

    fail(ErrorCode::Io, "Cannot create a temporary workspace directory", base);
}

std::string unique_name(std::string_view prefix, std::size_t index) {
    return std::string(prefix) + std::to_string(index);
}

void copy_dir_recursive(const fs::path& source, const fs::path& destination) {
    std::error_code ec;
    fs::create_directories(destination, ec);

    fs::recursive_directory_iterator it(source, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    while (!ec && it != end) {
        const fs::path path = it->path();
        std::error_code rel_ec;
        const fs::path relative = fs::relative(path, source, rel_ec);
        if (!rel_ec && !relative.empty()) {
            const fs::path target = destination / relative;
            std::error_code type_ec;
            if (it->is_directory(type_ec) && !type_ec) {
                fs::create_directories(target, ec);
            } else if (it->is_regular_file(type_ec) && !type_ec) {
                if (const fs::path parent = target.parent_path(); !parent.empty()) {
                    fs::create_directories(parent, ec);
                }
                fs::copy_file(path, target, fs::copy_options::overwrite_existing, ec);
                if (ec) {
                    fail(ErrorCode::Io, "Failed to copy '" + path_to_utf8(path) + "': " + ec.message(),
                         path);
                }
            }
        }
        it.increment(ec);
    }
}

/// Every archive still sitting inside the just-unpacked tree.
std::vector<fs::path> find_nested_archives(const fs::path& root) {
    std::vector<fs::path> archives;
    std::error_code ec;
    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    while (!ec && it != end) {
        std::error_code type_ec;
        if (it->is_regular_file(type_ec) && !type_ec && is_supported_archive(it->path())) {
            archives.push_back(it->path());
        }
        it.increment(ec);
    }
    std::sort(archives.begin(), archives.end());
    return archives;
}

/// Documentation worth keeping beside the assets. Deduplicated case-insensitively so a pack
/// shipping both README.md and readme.md does not collide on Windows.
std::vector<std::pair<std::string, fs::path>> collect_documentation_files_impl(const fs::path& root) {
    struct Candidate {
        std::string name;
        fs::path path;
        std::size_t depth;
    };
    std::vector<Candidate> candidates;

    std::error_code ec;
    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    while (!ec && it != end) {
        std::error_code type_ec;
        if (it->is_regular_file(type_ec) && !type_ec) {
            const fs::path path = it->path();
            std::error_code rel_ec;
            const fs::path relative = fs::relative(path, root, rel_ec);
            bool in_hidden_dir = false;
            if (!rel_ec) {
                for (const auto& part : relative.parent_path()) {
                    const std::string piece = path_to_utf8(part);
                    if (piece.size() > 1 && piece.front() == '.') {
                        in_hidden_dir = true;
                        break;
                    }
                }
            }
            if (!in_hidden_dir) {
                const std::string name = path_to_utf8(path.filename());
                const std::string lower = to_lower_ascii(name);
                if (starts_with(lower, "readme") || starts_with(lower, "license") ||
                    starts_with(lower, "licence") || starts_with(lower, "changelog") ||
                    ends_with(lower, ".md")) {
                    candidates.push_back(Candidate{name, path,
                                                   static_cast<std::size_t>(
                                                       std::distance(relative.begin(),
                                                                     relative.end()))});
                }
            }
        }
        it.increment(ec);
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        if (a.depth != b.depth) {
            return a.depth < b.depth;
        }
        return a.name < b.name;
    });

    std::vector<std::pair<std::string, fs::path>> result;
    std::set<std::string> seen;
    for (const auto& candidate : candidates) {
        if (seen.insert(to_lower_ascii(candidate.name)).second) {
            result.emplace_back(candidate.name, candidate.path);
        }
    }
    return result;
}

/// Existing metadata to inherit: the canonical root first, then the unpack root, then any
/// ancestor up to the unpack root (packages often keep mod.json above their payload).
std::optional<ModInfo> find_existing_mod_info(const fs::path& unpack_dir,
                                              const fs::path& canonical_root) {
    try {
        return ModLoader::load_mod_info(canonical_root);
    } catch (const SmmError&) {
    }
    try {
        return ModLoader::load_mod_info(unpack_dir);
    } catch (const SmmError&) {
    }

    fs::path current = canonical_root.parent_path();
    while (!current.empty() && current != unpack_dir) {
        try {
            return ModLoader::load_mod_info(current);
        } catch (const SmmError&) {
        }
        const fs::path parent = current.parent_path();
        if (parent == current) {
            break;
        }
        current = parent;
    }
    return std::nullopt;
}

/// Picks the "integration / all-in-one" package out of several nested archives.
/// Chinese keywords are included because Sekiro mod packs are frequently distributed there
/// and label the combined package explicitly.
fs::path pick_primary_nested_archive(const std::vector<fs::path>& archives) {
    if (archives.empty()) {
        return {};
    }
    if (archives.size() == 1) {
        return archives.front();
    }

    constexpr std::string_view kKeywords[] = {
        "integration", "integrate", "aio", "all-in-one", "all in one", "full", "complete", "main",
        "base",        "default",   "整合", "完整",       "全套",       "主体", "基础",     "默认",
    };

    for (const std::string_view keyword : kKeywords) {
        const auto it = std::find_if(archives.begin(), archives.end(), [keyword](const fs::path& p) {
            return to_lower_ascii(path_to_utf8(p.filename())).find(keyword) != std::string::npos;
        });
        if (it != archives.end()) {
            return *it;
        }
    }

    // No naming hint: the combined package is reliably the largest one.
    return *std::max_element(archives.begin(), archives.end(), [](const fs::path& a, const fs::path& b) {
        std::error_code ec_a;
        std::error_code ec_b;
        return fs::file_size(a, ec_a) < fs::file_size(b, ec_b);
    });
}

/// Unpacks any archives nested one level inside an already-unpacked tree, up to \`max_rounds\`.
/// Multi-part downloads (a zip containing one archive per component) are common enough that
/// failing on them would look like a bug to the user.
void unpack_nested_archives(const fs::path& unpack_dir, const fs::path& workspace) {
    for (int round = 0; round < 3; ++round) {
        bool canonical_found = true;
        try {
            Normalizer::find_canonical_root(unpack_dir);
        } catch (const SmmError&) {
            canonical_found = false;
        }
        if (canonical_found) {
            return;
        }

        const std::vector<fs::path> archives = find_nested_archives(unpack_dir);
        if (archives.empty()) {
            return;
        }

        const fs::path primary = pick_primary_nested_archive(archives);
        if (!primary.empty()) {
            const fs::path nested_out = workspace / unique_name("nested_", round);
            extract_archive(primary, nested_out);
            for (const auto& archive : archives) {
                std::error_code ec;
                fs::remove(archive, ec);
            }
            copy_dir_recursive(nested_out, unpack_dir);
            continue;
        }

        for (std::size_t index = 0; index < archives.size(); ++index) {
            const fs::path nested_out =
                workspace / ("nested_" + std::to_string(round) + "_" + std::to_string(index));
            try {
                extract_archive(archives[index], nested_out);
                copy_dir_recursive(nested_out, unpack_dir);
            } catch (const SmmError&) {
                // One unreadable component must not sink the whole bundle; the remaining
                // components and the outer payload are still imported.
            }
            std::error_code ec;
            fs::remove(archives[index], ec);
        }
    }
}

/// The metadata a fresh import generates when the package carries none.
ModInfo derive_mod_info(const fs::path& source, const std::vector<AssetEntry>& assets,
                        const ImportOptions& options) {
    fs::path stem_path = source.filename();
    if (source.has_extension()) {
        stem_path = source.stem();
    }
    const std::string stem = path_to_utf8(stem_path);

    ModInfo info;
    info.id = options.id.value_or(slugify(stem));
    info.name = options.name.value_or(humanize_name(stem));
    info.version = extract_version(stem).value_or("1.0.0");
    info.author = "Unknown";
    info.category = infer_category(assets);
    info.priority = options.priority.value_or(default_priority_for_category(info.category));
    info.description = "Imported package from " + path_to_utf8(source.filename());
    info.source_url = options.source_url;
    info.enabled = true;
    return info;
}

/// Builds the normalised staging layout for \`info\` inside \`build_dir\`.
void materialize_mod(const NormalizationResult& normalized, const fs::path& unpack_dir,
                     const fs::path& source, const fs::path& build_dir, const ModInfo& info) {
    std::error_code ec;
    fs::create_directories(build_dir, ec);

    for (const auto& asset : normalized.assets) {
        const fs::path destination = build_dir / asset.relative_path;
        if (const fs::path parent = destination.parent_path(); !parent.empty()) {
            fs::create_directories(parent, ec);
        }
        fs::copy_file(asset.source_path, destination, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            fail(ErrorCode::Io,
                 "Failed to stage '" + asset.relative_path + "': " + ec.message(),
                 asset.source_path);
        }
    }

    for (const auto& [name, path] : collect_documentation_files_impl(unpack_dir)) {
        const fs::path destination = build_dir / name;
        if (!fs::exists(destination, ec)) {
            fs::copy_file(path, destination, fs::copy_options::overwrite_existing, ec);
            ec.clear();
        }
    }

    // .smm_source keeps the untouched original beside the normalised tree, which is what makes
    // "re-import from the same file later" and post-mortem debugging possible.
    const fs::path build_source = build_dir / ".smm_source";
    const fs::path unpack_source = unpack_dir / ".smm_source";
    if (fs::is_directory(unpack_source, ec)) {
        copy_dir_recursive(unpack_source, build_source);
    } else if (fs::is_directory(source, ec)) {
        const fs::path source_backup = source / ".smm_source";
        if (fs::is_directory(source_backup, ec)) {
            copy_dir_recursive(source_backup, build_source);
        }
    }

    if (fs::is_regular_file(source, ec)) {
        fs::create_directories(build_source, ec);
        fs::copy_file(source, build_source / source.filename(),
                      fs::copy_options::overwrite_existing, ec);
        if (ec) {
            fail(ErrorCode::Io, "Failed to back up the source archive: " + ec.message(), source);
        }
    }

    ModManager::save_mod_info(build_dir, info);
}

/// Moves the finished layout into place, preferring a rename so the mod appears atomically.
void move_into_staging(const fs::path& build_dir, const fs::path& target_dir, bool overwrite) {
    std::error_code ec;
    if (fs::exists(target_dir, ec)) {
        if (!overwrite) {
            fail(ErrorCode::ModAlreadyExists,
                 "A mod with this id is already staged: " + path_to_utf8(target_dir.filename()),
                 target_dir);
        }
        fs::remove_all(target_dir, ec);
        if (ec) {
            fail(ErrorCode::Io, "Failed to replace the existing mod: " + ec.message(), target_dir);
        }
    }

    fs::rename(build_dir, target_dir, ec);
    if (!ec) {
        return;
    }

    // Cross-volume staging (staging on another drive than the temp workspace) cannot rename.
    ec.clear();
    copy_dir_recursive(build_dir, target_dir);
    std::error_code cleanup_ec;
    fs::remove_all(build_dir, cleanup_ec);
}

} // namespace

std::vector<std::pair<std::string, fs::path>> collect_documentation_files(const fs::path& root) {
    return collect_documentation_files_impl(root);
}

std::string slugify(std::string_view input) {
    std::string result;
    bool last_was_dash = false;
    for (const char ch : input) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            last_was_dash = false;
        } else if (!last_was_dash && !result.empty()) {
            result.push_back('-');
            last_was_dash = true;
        }
    }
    if (!result.empty() && result.back() == '-') {
        result.pop_back();
    }
    return result.empty() ? std::string("imported-mod") : result;
}

namespace {

std::string strip_archive_extension(std::string_view name) {
    const std::string lower = to_lower_ascii(name);
    if (ends_with(lower, ".zip") || ends_with(lower, ".rar")) {
        return std::string(name.substr(0, name.size() - 4));
    }
    if (ends_with(lower, ".7z")) {
        return std::string(name.substr(0, name.size() - 3));
    }
    return std::string(name);
}

/// Splits on any of the separators authors actually use in package names.
std::vector<std::string> split_words(std::string_view text) {
    std::vector<std::string> words;
    std::string current;
    for (const char ch : text) {
        if (ch == '_' || ch == '-' || ch == '.' || ch == ' ') {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        words.push_back(current);
    }
    return words;
}

} // namespace

std::string humanize_name(std::string_view input) {
    const std::string clean = strip_archive_extension(input);
    const std::vector<std::string> words = split_words(clean);

    std::string result;
    for (const std::string& word : words) {
        if (!result.empty()) {
            result.push_back(' ');
        }
        result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(word.front()))));
        result.append(word.substr(1));
    }
    return result.empty() ? std::string("Imported Mod") : result;
}

std::optional<std::string> extract_version(std::string_view text) {
    const std::string clean = strip_archive_extension(text);
    for (const std::string& part : split_words(clean)) {
        std::string_view trimmed(part);
        if (!trimmed.empty() && (trimmed.front() == 'v' || trimmed.front() == 'V')) {
            trimmed.remove_prefix(1);
        }

        std::vector<std::string_view> components;
        std::size_t start = 0;
        while (start <= trimmed.size()) {
            const std::size_t end = trimmed.find('.', start);
            components.push_back(trimmed.substr(start, end == std::string_view::npos
                                                           ? std::string_view::npos
                                                           : end - start));
            if (end == std::string_view::npos) {
                break;
            }
            start = end + 1;
        }

        if (components.size() < 2) {
            continue;
        }
        const bool all_numeric = std::all_of(
            components.begin(), components.end(), [](std::string_view component) {
                return !component.empty() &&
                       std::all_of(component.begin(), component.end(), [](char ch) {
                           return std::isdigit(static_cast<unsigned char>(ch)) != 0;
                       });
            });
        if (all_numeric) {
            return std::string(trimmed);
        }
    }
    return std::nullopt;
}

std::string infer_category(const std::vector<AssetEntry>& assets) {
    bool has_loader = false;
    bool has_param = false;
    bool has_weapon = false;
    bool has_chr = false;
    bool has_parts = false;
    bool has_ui = false;
    bool has_sound = false;
    bool has_map = false;
    bool has_obj = false;
    bool has_script = false;
    bool has_sfx = false;

    for (const auto& asset : assets) {
        switch (asset.category) {
            case AssetCategory::Loader:
                has_loader = true;
                break;
            case AssetCategory::Param:
                has_param = true;
                break;
            case AssetCategory::Parts:
                has_parts = true;
                if (to_lower_ascii(asset.relative_path).find("wp_") != std::string::npos) {
                    has_weapon = true;
                }
                break;
            case AssetCategory::Chr:
                has_chr = true;
                break;
            case AssetCategory::Menu:
            case AssetCategory::Font:
            case AssetCategory::Msg:
                has_ui = true;
                break;
            case AssetCategory::Sound:
                has_sound = true;
                break;
            case AssetCategory::Map:
                has_map = true;
                break;
            case AssetCategory::Obj:
                has_obj = true;
                break;
            case AssetCategory::Script:
                has_script = true;
                break;
            case AssetCategory::Sfx:
                has_sfx = true;
                break;
            default:
                break;
        }
    }

    // Ordered by how much a mod of that kind overrides: a payload that touches gameparam
    // is an overhaul no matter what else it ships, and must not be labelled a skin.
    if (has_loader) return "loader";
    if (has_param) return "gameplay_overhaul";
    if (has_sfx) return "vfx";
    if (has_weapon) return "weapon_skin";
    if (has_chr || has_parts) return "character_skin";
    if (has_ui) return "ui";
    if (has_sound) return "audio";
    if (has_obj || has_map) return "map";
    if (has_script) return "script";
    return "general";
}

uint32_t default_priority_for_category(std::string_view category) {
    if (category == "loader") {
        return 10;
    }
    if (category == "gameplay_overhaul") {
        return 50;
    }
    if (category == "vfx") {
        return 80;
    }
    return 100;
}

void to_json(json& j, const ImportResult& r) {
    j = json{
        {"mod", r.info},
        {"mod_dir", path_to_utf8(r.mod_dir)},
        {"asset_count", r.asset_count},
        {"ignored_count", r.ignored_count},
        {"total_bytes", r.total_bytes},
        {"replaced_existing", r.replaced_existing},
        {"dry_run", r.dry_run},
        {"extracted_archive", r.extracted_archive},
    };
}

ImportResult import_mod(const fs::path& source, const fs::path& staging_dir,
                        const ImportOptions& options) {
    std::error_code ec;
    if (!fs::exists(source, ec)) {
        fail(ErrorCode::Io, "Source path does not exist: " + path_to_utf8(source), source);
    }

    fs::create_directories(staging_dir, ec);

    TempDir workspace = make_workspace(staging_dir, ".smm_import_");
    const fs::path unpack_dir = workspace.path() / "unpacked";
    fs::create_directories(unpack_dir, ec);

    const bool from_archive = fs::is_regular_file(source, ec);
    if (from_archive) {
        if (!is_supported_archive(source)) {
            fail(ErrorCode::NormalizationError,
                 "Unsupported archive format. SMM can import .zip, .7z and .rar packages, or an "
                 "already unpacked mod directory.",
                 source);
        }
        extract_archive(source, unpack_dir);
    } else if (fs::is_directory(source, ec)) {
        copy_dir_recursive(source, unpack_dir);
    } else {
        fail(ErrorCode::NormalizationError, "Invalid source path", source);
    }

    unpack_nested_archives(unpack_dir, workspace.path());

    const NormalizationResult normalized = Normalizer::normalize_directory(unpack_dir);
    if (normalized.assets.empty()) {
        fail(ErrorCode::NoAssetsFound, "No deployable Sekiro assets found in the package", source);
    }

    ModInfo info;
    if (const auto existing = find_existing_mod_info(unpack_dir, normalized.canonical_root)) {
        // A package that ships its own metadata is authoritative; the caller only overrides
        // the fields it explicitly asked for.
        info = *existing;
        if (options.id) info.id = *options.id;
        if (options.name) info.name = *options.name;
        if (options.priority) info.priority = *options.priority;
        if (options.source_url) info.source_url = options.source_url;
    } else {
        info = derive_mod_info(source, normalized.assets, options);
    }

    ImportResult result;
    result.info = info;
    result.asset_count = normalized.assets.size();
    result.ignored_count = normalized.ignored_files.size();
    for (const auto& asset : normalized.assets) {
        result.total_bytes += asset.file_size;
    }
    result.dry_run = options.dry_run;
    result.extracted_archive = from_archive;

    const fs::path target_dir = staging_dir / info.id;
    const bool already_staged = fs::exists(target_dir, ec);
    result.replaced_existing = already_staged;

    if (options.dry_run) {
        result.mod_dir = target_dir;
        return result;
    }

    const fs::path build_dir = workspace.path() / "normalized_mod";
    materialize_mod(normalized, unpack_dir, source, build_dir, info);
    move_into_staging(build_dir, target_dir, options.overwrite);

    info.root_path = target_dir;
    result.info = info;
    result.mod_dir = target_dir;
    return result;
}

ImportResult import_multiple_files_as_mod(const std::vector<fs::path>& files,
                                          const fs::path& staging_dir,
                                          const ImportOptions& options) {
    if (files.empty()) {
        fail(ErrorCode::InvalidArgument, "No source files were provided for a merged import");
    }
    if (files.size() == 1) {
        return import_mod(files.front(), staging_dir, options);
    }

    std::error_code ec;
    fs::create_directories(staging_dir, ec);

    TempDir workspace = make_workspace(staging_dir, ".smm_merge_");
    const fs::path merged = workspace.path() / "merged";
    const fs::path source_backup = merged / ".smm_source";
    fs::create_directories(source_backup, ec);

    for (std::size_t index = 0; index < files.size(); ++index) {
        const fs::path& source = files[index];
        if (!fs::exists(source, ec)) {
            fail(ErrorCode::Io, "Source path does not exist: " + path_to_utf8(source), source);
        }

        if (fs::is_regular_file(source, ec)) {
            fs::copy_file(source, source_backup / source.filename(),
                          fs::copy_options::overwrite_existing, ec);
            ec.clear();
        }

        const fs::path item_unpack = workspace.path() / ("item_" + std::to_string(index));
        fs::create_directories(item_unpack, ec);

        if (fs::is_regular_file(source, ec)) {
            if (!is_supported_archive(source)) {
                fail(ErrorCode::NormalizationError,
                     "Unsupported archive format in a merged import", source);
            }
            extract_archive(source, item_unpack);
        } else if (fs::is_directory(source, ec)) {
            copy_dir_recursive(source, item_unpack);
        }

        for (const fs::path& archive : find_nested_archives(item_unpack)) {
            const fs::path nested_out =
                workspace.path() / ("item_" + std::to_string(index) + "_" + path_to_utf8(archive.stem()));
            try {
                extract_archive(archive, nested_out);
                copy_dir_recursive(nested_out, item_unpack);
            } catch (const SmmError&) {
            }
            std::error_code remove_ec;
            fs::remove(archive, remove_ec);
        }

        // Each component is normalised on its own before merging, so a component that nests its
        // payload under an unusual wrapper does not drag the whole bundle to the wrong root.
        const NormalizationResult item = Normalizer::normalize_directory(item_unpack);
        for (const auto& asset : item.assets) {
            const fs::path destination = merged / asset.relative_path;
            if (const fs::path parent = destination.parent_path(); !parent.empty()) {
                fs::create_directories(parent, ec);
            }
            fs::copy_file(asset.source_path, destination, fs::copy_options::overwrite_existing, ec);
            ec.clear();
        }
        for (const auto& [name, path] : collect_documentation_files_impl(item_unpack)) {
            const fs::path destination = merged / name;
            if (!fs::exists(destination, ec)) {
                fs::copy_file(path, destination, fs::copy_options::overwrite_existing, ec);
                ec.clear();
            }
        }
    }

    const NormalizationResult normalized = Normalizer::normalize_directory(merged);
    if (normalized.assets.empty()) {
        fail(ErrorCode::NoAssetsFound, "No deployable Sekiro assets found in the merged import",
             files.front());
    }

    const fs::path first_stem = files.front().has_extension() ? files.front().stem()
                                                             : files.front().filename();
    const std::string default_name =
        options.name.value_or("Merged - " + humanize_name(path_to_utf8(first_stem)));

    ModInfo info;
    info.id = options.id.value_or(slugify(default_name));
    info.name = default_name;
    info.version = "1.0.0";
    info.author = "Community";
    info.category = infer_category(normalized.assets);
    info.priority = options.priority.value_or(default_priority_for_category(info.category));
    info.source_url = options.source_url;
    info.description =
        "Merged mod bundle assembled from " + std::to_string(files.size()) + " component files.";
    info.enabled = true;

    ImportResult result;
    result.info = info;
    result.asset_count = normalized.assets.size();
    result.ignored_count = normalized.ignored_files.size();
    for (const auto& asset : normalized.assets) {
        result.total_bytes += asset.file_size;
    }
    result.dry_run = options.dry_run;
    result.extracted_archive = true;

    const fs::path target_dir = staging_dir / info.id;
    result.replaced_existing = fs::exists(target_dir, ec);
    result.mod_dir = target_dir;
    if (options.dry_run) {
        return result;
    }

    const fs::path build_dir = workspace.path() / "normalized_mod";
    materialize_mod(normalized, merged, files.front(), build_dir, info);
    move_into_staging(build_dir, target_dir, options.overwrite);

    info.root_path = target_dir;
    result.info = info;
    return result;
}

} // namespace smm
