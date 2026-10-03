#include "smm/loader.hpp"

#include <algorithm>
#include <fstream>

#include "smm/error.hpp"
#include "smm/manager.hpp"
#include "smm/normalizer.hpp"

namespace smm {

const StagedMod* ScanOutcome::find(const std::string& mod_id) const {
    const auto it = std::find_if(mods.begin(), mods.end(), [&mod_id](const StagedMod& mod) {
        return mod.info.id == mod_id;
    });
    return it == mods.end() ? nullptr : &*it;
}

bool ModLoader::is_mod_directory(const fs::path& dir) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec) || ec) {
        return false;
    }
    return fs::exists(dir / ".smm_mod.json", ec) || fs::exists(dir / "mod.json", ec);
}

ModInfo ModLoader::load_mod_info(const fs::path& mod_dir) {
    std::error_code ec;
    fs::path meta_path = mod_dir / ".smm_mod.json";
    if (!fs::exists(meta_path, ec)) {
        meta_path = mod_dir / "mod.json";
        if (!fs::exists(meta_path, ec)) {
            fail(ErrorCode::MetadataNotFound,
                 "No mod metadata (.smm_mod.json or mod.json) in " + path_to_utf8(mod_dir), mod_dir);
        }
    }

    std::ifstream stream(meta_path, std::ios::binary);
    if (!stream) {
        fail(ErrorCode::Io, "Cannot open mod metadata: " + path_to_utf8(meta_path), meta_path);
    }

    json parsed;
    try {
        stream >> parsed;
    } catch (const json::exception& e) {
        fail(ErrorCode::InvalidMetadata,
             std::string("Malformed mod metadata: ") + e.what(), meta_path);
    }

    ModInfo info;
    try {
        info = parsed.get<ModInfo>();
    } catch (const json::exception& e) {
        fail(ErrorCode::InvalidMetadata,
             std::string("Unexpected metadata shape: ") + e.what(), meta_path);
    }

    if (info.id.empty()) {
        // A mod without an id cannot be addressed by any other command, so the directory
        // name is the only sane fallback.
        info.id = path_to_utf8(mod_dir.filename());
    }
    info.root_path = mod_dir;

    bool metadata_changed = false;

    // Auto-detect preview image if not explicitly configured in metadata
    if (!info.preview_image.has_value() || info.preview_image->empty()) {
        constexpr std::string_view candidates[] = {
            "preview.png", "preview.jpg", "preview.jpeg", "preview.webp",
            "cover.png", "cover.jpg", "cover.jpeg", "cover.webp",
            "banner.png", "banner.jpg"
        };
        for (const auto cand : candidates) {
            if (fs::exists(mod_dir / cand, ec)) {
                info.preview_image = std::string(cand);
                metadata_changed = true;
                break;
            }
        }
    }

    if (info.preview_image.has_value() && !info.preview_image->empty()) {
        std::string current_preview = *info.preview_image;
        if (ModManager::normalize_preview_image(mod_dir, current_preview)) {
            if (current_preview != *info.preview_image) {
                info.preview_image = current_preview;
                metadata_changed = true;
            }
        }
    }

    // 若图片标准化、自动探测到预览图，或原读取自遗留 mod.json，统一持久化为 .smm_mod.json 并清理旧文件
    if (metadata_changed || meta_path.filename() == "mod.json") {
        ModManager::save_mod_info(mod_dir, info);
    }

    return info;
}

StagedMod ModLoader::scan_mod(const fs::path& mod_dir) {
    StagedMod staged;
    staged.info = load_mod_info(mod_dir);
    staged.assets = Normalizer::normalize_directory(mod_dir).assets;

    // Apply file-level toggle state from disabled_assets metadata
    for (auto& asset : staged.assets) {
        if (std::find(staged.info.disabled_assets.begin(),
                      staged.info.disabled_assets.end(),
                      asset.relative_path) != staged.info.disabled_assets.end()) {
            asset.enabled = false;
        } else {
            asset.enabled = true;
        }
    }

    return staged;
}

ScanOutcome ModLoader::scan_mods_directory(const fs::path& staging_dir) {
    std::error_code ec;
    if (!fs::exists(staging_dir, ec)) {
        fail(ErrorCode::Io,
             "Staging directory does not exist: " + path_to_utf8(staging_dir), staging_dir);
    }

    ScanOutcome outcome;
    for (const auto& entry : fs::directory_iterator(staging_dir, ec)) {
        if (ec) {
            break;
        }
        const fs::path path = entry.path();
        if (!is_mod_directory(path)) {
            continue;
        }
        try {
            outcome.mods.push_back(scan_mod(path));
        } catch (const SmmError& e) {
            // One broken mod must never hide the rest of the library; the UI lists these.
            outcome.failures.push_back(ScanFailure{path, e.what()});
        }
    }

    std::sort(outcome.mods.begin(), outcome.mods.end(), [](const StagedMod& a, const StagedMod& b) {
        if (a.info.priority != b.info.priority) {
            return a.info.priority < b.info.priority;
        }
        return a.info.id < b.info.id;
    });
    std::sort(outcome.failures.begin(), outcome.failures.end(), [](const ScanFailure& a, const ScanFailure& b) {
        return a.path.native() < b.path.native();
    });

    return outcome;
}

} // namespace smm
