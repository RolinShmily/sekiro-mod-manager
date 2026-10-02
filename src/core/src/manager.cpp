#include "smm/manager.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>

#include "smm/error.hpp"
#include "smm/loader.hpp"

namespace smm {

fs::path ModManager::find_mod_dir(const fs::path& staging_dir, const std::string& mod_id) {
    std::error_code ec;
    if (!fs::exists(staging_dir, ec)) {
        fail(ErrorCode::Io,
             "Staging directory does not exist: " + staging_dir.string(), staging_dir);
    }

    // Fast path: the directory is named after the id.
    const fs::path direct = staging_dir / mod_id;
    if (ModLoader::is_mod_directory(direct)) {
        try {
            const ModInfo info = ModLoader::load_mod_info(direct);
            if (info.id == mod_id) {
                return direct;
            }
        } catch (const SmmError&) {
            // Fall through to the full scan: a broken direct hit must not shadow a good one.
        }
    }

    fs::path case_insensitive_candidate;
    for (const auto& entry : fs::directory_iterator(staging_dir, ec)) {
        if (ec) {
            break;
        }
        const fs::path path = entry.path();
        if (!ModLoader::is_mod_directory(path)) {
            continue;
        }
        ModInfo info;
        try {
            info = ModLoader::load_mod_info(path);
        } catch (const SmmError&) {
            continue;
        }
        if (info.id == mod_id) {
            return path;
        }
        if (case_insensitive_candidate.empty() && info.id.size() == mod_id.size() &&
            std::equal(info.id.begin(), info.id.end(), mod_id.begin(), [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) ==
                       std::tolower(static_cast<unsigned char>(b));
            })) {
            case_insensitive_candidate = path;
        }
    }

    if (!case_insensitive_candidate.empty()) {
        return case_insensitive_candidate;
    }

    fail(ErrorCode::ModNotFound, "Mod '" + mod_id + "' not found in staging", staging_dir);
}

ModInfo ModManager::load_mod_info(const fs::path& mod_dir) {
    return ModLoader::load_mod_info(mod_dir);
}

ModInfo ModManager::set_mod_enabled(const fs::path& staging_dir, const std::string& mod_id, bool enabled) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);
    ModInfo info = ModLoader::load_mod_info(mod_dir);
    info.enabled = enabled;
    save_mod_info(mod_dir, info);
    info.root_path = mod_dir;
    return info;
}

ModInfo ModManager::set_mod_priority(const fs::path& staging_dir, const std::string& mod_id, uint32_t priority) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);
    ModInfo info = ModLoader::load_mod_info(mod_dir);
    info.priority = priority;
    save_mod_info(mod_dir, info);
    info.root_path = mod_dir;
    return info;
}

ModInfo ModManager::set_asset_enabled(const fs::path& staging_dir, const std::string& mod_id,
                                      const std::string& rel_path, bool enabled) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);
    ModInfo info = ModLoader::load_mod_info(mod_dir);

    std::string norm_path = rel_path;
    std::replace(norm_path.begin(), norm_path.end(), '\\', '/');

    auto it = std::find(info.disabled_assets.begin(), info.disabled_assets.end(), norm_path);
    if (!enabled) {
        if (it == info.disabled_assets.end()) {
            info.disabled_assets.push_back(norm_path);
        }
    } else {
        if (it != info.disabled_assets.end()) {
            info.disabled_assets.erase(it);
        }
    }

    save_mod_info(mod_dir, info);
    info.root_path = mod_dir;
    return info;
}

ModInfo ModManager::set_mod_preview(const fs::path& staging_dir, const std::string& mod_id,
                                    const fs::path& image_src_path) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);
    std::error_code ec;
    if (!fs::exists(image_src_path, ec)) {
        fail(ErrorCode::Io, "Preview image file does not exist: " + image_src_path.string(), image_src_path);
    }

    std::string ext = image_src_path.extension().string();
    if (ext.empty()) {
        ext = ".png";
    }
    const std::string filename = "preview" + ext;
    const fs::path target_path = mod_dir / filename;

    fs::copy_file(image_src_path, target_path, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        fail(ErrorCode::Io, "Failed to copy preview image to staging: " + ec.message(), target_path);
    }

    ModInfo info = ModLoader::load_mod_info(mod_dir);
    info.preview_image = filename;
    save_mod_info(mod_dir, info);
    info.root_path = mod_dir;
    return info;
}

ModInfo ModManager::update_mod_info(const fs::path& staging_dir, const std::string& mod_id, const ModInfo& updated) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);
    ModInfo current = ModLoader::load_mod_info(mod_dir);

    // id and root_path are identity, not user data: they are never taken from the caller.
    current.name = updated.name;
    current.version = updated.version;
    current.author = updated.author;
    current.category = updated.category;
    current.description = updated.description;
    current.homepage = updated.homepage;
    current.source_url = updated.source_url;
    current.license = updated.license;
    current.enabled = updated.enabled;
    current.priority = updated.priority;
    current.tags = updated.tags;
    current.root_path = mod_dir;

    save_mod_info(mod_dir, current);
    return current;
}

void ModManager::delete_mod(const fs::path& staging_dir, const std::string& mod_id) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);

    std::error_code ec;
    const fs::path canonical_staging = fs::canonical(staging_dir, ec);
    if (ec) {
        fail(ErrorCode::Io, "Cannot resolve staging directory: " + ec.message(), staging_dir);
    }
    const fs::path canonical_mod = fs::canonical(mod_dir, ec);
    if (ec) {
        fail(ErrorCode::Io, "Cannot resolve mod directory: " + ec.message(), mod_dir);
    }

    // Guard rail: a metadata file can point anywhere, so deleting must never escape staging.
    const std::string staging_str = canonical_staging.string();
    const std::string mod_str = canonical_mod.string();
    const bool inside = mod_str.size() > staging_str.size() &&
                        mod_str.compare(0, staging_str.size(), staging_str) == 0 &&
                        (mod_str[staging_str.size()] == '/' || mod_str[staging_str.size()] == '\\');
    if (!inside) {
        fail(ErrorCode::Io,
             "Refusing to delete a path outside the staging directory: " + mod_dir.string(), mod_dir);
    }

    fs::remove_all(canonical_mod, ec);
    if (ec) {
        fail(ErrorCode::Io, "Failed to delete mod directory: " + ec.message(), mod_dir);
    }
}

StagedMod ModManager::get_mod_details(const fs::path& staging_dir, const std::string& mod_id) {
    return ModLoader::scan_mod(find_mod_dir(staging_dir, mod_id));
}

void ModManager::save_mod_info(const fs::path& mod_dir, const ModInfo& info) {
    ModInfo portable = info;
    portable.root_path.reset();

    const std::string payload = json(portable).dump(2);

    // Written twice on purpose: .smm_mod.json is SMM's own record, mod.json is what the wider
    // modding ecosystem reads. Both must always agree.
    for (const char* filename : {".smm_mod.json", "mod.json"}) {
        const fs::path meta_path = mod_dir / filename;
        const fs::path tmp_path = mod_dir / ("." + std::string(filename) + ".tmp");

        std::error_code ec;
        {
            std::ofstream stream(tmp_path, std::ios::binary | std::ios::trunc);
            if (stream) {
                stream << payload;
                stream.flush();
            }
            if (!stream) {
                ec = std::make_error_code(std::errc::io_error);
            }
        }

        if (!ec) {
            fs::remove(meta_path, ec);
            ec.clear();
            fs::rename(tmp_path, meta_path, ec);
        }

        if (ec) {
            // Some filesystems or a locked target defeat the atomic path; a direct write
            // still gets the metadata onto disk.
            std::error_code cleanup_ec;
            fs::remove(tmp_path, cleanup_ec);
            std::ofstream stream(meta_path, std::ios::binary | std::ios::trunc);
            stream << payload;
            stream.flush();
            if (!stream) {
                fail(ErrorCode::Io, "Failed to write mod metadata: " + meta_path.string(), meta_path);
            }
        }
    }
}

} // namespace smm
