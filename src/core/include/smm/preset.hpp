#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "types.hpp"

namespace smm {

inline constexpr std::string_view PRESETS_FILE_NAME = ".smm_presets.json";

/// Saved activation states, stored as a single JSON array in the staging directory.
class PresetManager {
public:
    static fs::path presets_path(const fs::path& staging_dir);

    /// Reads every preset. A missing file yields an empty list; malformed JSON throws
    /// SmmError(InvalidMetadata) rather than silently discarding the user's presets.
    static std::vector<ModPreset> list_presets(const fs::path& staging_dir);

    /// Inserts or replaces a preset by id, stamping updated_at (and created_at when unset).
    static ModPreset save_preset(const fs::path& staging_dir, const ModPreset& preset);

    /// Snapshots the currently enabled mods and their priorities into a new preset.
    static ModPreset create_preset_from_current(const fs::path& staging_dir, const std::string& name,
                                                const std::optional<std::string>& description = {});

    /// Enables exactly the mods of the preset with their stored priorities and
    /// disables every other staged mod. Returns the applied preset.
    static ModPreset apply_preset(const fs::path& staging_dir, const std::string& preset_id);

    static void delete_preset(const fs::path& staging_dir, const std::string& preset_id);

    /// Rejects duplicate names; returns the existing preset when the name is taken.
    static std::optional<ModPreset> find_by_name(const fs::path& staging_dir, const std::string& name);
};

} // namespace smm
