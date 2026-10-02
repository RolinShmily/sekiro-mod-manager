#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "types.hpp"

namespace smm {

/// Mod lifecycle inside the staging directory.
class ModManager {
public:
    /// Resolves a mod id to its staging directory. Falls back to a case-insensitive match.
    /// Throws SmmError(ModNotFound).
    static fs::path find_mod_dir(const fs::path& staging_dir, const std::string& mod_id);

    static ModInfo set_mod_enabled(const fs::path& staging_dir, const std::string& mod_id, bool enabled);
    static ModInfo set_mod_priority(const fs::path& staging_dir, const std::string& mod_id, uint32_t priority);

    /// Toggles a single asset's enabled state within a mod.
    static ModInfo set_asset_enabled(const fs::path& staging_dir, const std::string& mod_id,
                                     const std::string& rel_path, bool enabled);

    /// Sets or updates the mod's preview background image by copying image_path into staging.
    static ModInfo set_mod_preview(const fs::path& staging_dir, const std::string& mod_id,
                                   const fs::path& image_src_path);

    /// Ensures preview image in mod_dir is in standard decodable PNG format, converting WebP if needed.
    static bool normalize_preview_image(const fs::path& mod_dir, std::string& preview_name);

    /// Applies user-editable metadata fields (never id, never root_path).
    static ModInfo update_mod_info(const fs::path& staging_dir, const std::string& mod_id, const ModInfo& updated);

    /// Recursively deletes the mod directory after verifying it really lives inside staging.
    static void delete_mod(const fs::path& staging_dir, const std::string& mod_id);

    static StagedMod get_mod_details(const fs::path& staging_dir, const std::string& mod_id);

    /// Writes metadata to both `.smm_mod.json` and `mod.json` atomically
    /// (temp file + rename, with a direct-write fallback).
    static void save_mod_info(const fs::path& mod_dir, const ModInfo& info);

    /// Reads raw metadata without normalising assets; used by the CLI's edit paths.
    static ModInfo load_mod_info(const fs::path& mod_dir);
};

} // namespace smm
