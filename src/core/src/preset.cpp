#include "smm/preset.hpp"

#include <fstream>

#include "smm/error.hpp"
#include "smm/loader.hpp"
#include "smm/manager.hpp"

namespace smm {
namespace {

/// Builds the synthetic preset id: stable for a given name and creation second.
std::string make_preset_id(const std::string& name, uint64_t epoch) {
    std::string slug;
    slug.reserve(name.size());
    for (const unsigned char c : name) {
        if (std::isalnum(c)) {
            slug.push_back(static_cast<char>(std::tolower(c)));
        } else {
            slug.push_back('_');
        }
    }
    while (!slug.empty() && slug.front() == '_') {
        slug.erase(slug.begin());
    }
    while (!slug.empty() && slug.back() == '_') {
        slug.pop_back();
    }
    if (slug.empty()) {
        slug = "preset";
    }
    return "preset_" + slug + "_" + std::to_string(epoch);
}

} // namespace

fs::path PresetManager::presets_path(const fs::path& staging_dir) {
    return staging_dir / std::string(PRESETS_FILE_NAME);
}

std::vector<ModPreset> PresetManager::list_presets(const fs::path& staging_dir) {
    const fs::path path = presets_path(staging_dir);
    std::error_code ec;
    if (!fs::exists(path, ec) || fs::file_size(path, ec) == 0) {
        return {};
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        fail(ErrorCode::Io, "Cannot open presets file: " + path.string(), path);
    }

    json parsed;
    try {
        stream >> parsed;
    } catch (const json::exception& e) {
        // Losing the user's presets silently would be far worse than refusing to start.
        fail(ErrorCode::InvalidMetadata,
             std::string("Failed to parse ") + std::string(PRESETS_FILE_NAME) + ": " + e.what(), path);
    }
    if (!parsed.is_array()) {
        fail(ErrorCode::InvalidMetadata,
             std::string(PRESETS_FILE_NAME) + " must contain a JSON array", path);
    }

    try {
        return parsed.get<std::vector<ModPreset>>();
    } catch (const json::exception& e) {
        fail(ErrorCode::InvalidMetadata,
             std::string("Unexpected preset shape: ") + e.what(), path);
    }
}

std::optional<ModPreset> PresetManager::find_by_name(const fs::path& staging_dir, const std::string& name) {
    for (const auto& preset : list_presets(staging_dir)) {
        if (preset.name == name) {
            return preset;
        }
    }
    return std::nullopt;
}

ModPreset PresetManager::save_preset(const fs::path& staging_dir, const ModPreset& preset) {
    std::vector<ModPreset> presets = list_presets(staging_dir);

    const uint64_t now = now_epoch_seconds();
    ModPreset saved = preset;
    saved.updated_at = now;
    if (saved.created_at == 0) {
        saved.created_at = now;
    }
    if (saved.id.empty()) {
        saved.id = make_preset_id(saved.name, now);
    }

    const auto it = std::find_if(presets.begin(), presets.end(), [&saved](const ModPreset& candidate) {
        return candidate.id == saved.id;
    });
    if (it != presets.end()) {
        *it = saved;
    } else {
        presets.push_back(saved);
    }

    const std::string content = json(presets).dump(2);

    std::error_code ec;
    fs::create_directories(staging_dir, ec);
    std::ofstream stream(presets_path(staging_dir), std::ios::binary | std::ios::trunc);
    if (!stream) {
        fail(ErrorCode::Io, "Cannot write presets file in " + staging_dir.string(), staging_dir);
    }
    stream << content;
    stream.flush();
    if (!stream) {
        fail(ErrorCode::Io, "Failed to write presets file in " + staging_dir.string(), staging_dir);
    }

    return saved;
}

ModPreset PresetManager::create_preset_from_current(const fs::path& staging_dir, const std::string& name,
                                                    const std::optional<std::string>& description) {
    const ScanOutcome outcome = ModLoader::scan_mods_directory(staging_dir);

    ModPreset preset;
    preset.name = name;
    preset.description = description;
    for (const auto& mod : outcome.mods) {
        if (mod.info.enabled) {
            preset.mods.push_back(ModPresetEntry{mod.info.id, mod.info.priority});
        }
    }
    std::sort(preset.mods.begin(), preset.mods.end(), [](const ModPresetEntry& a, const ModPresetEntry& b) {
        if (a.priority != b.priority) {
            return a.priority < b.priority;
        }
        return a.mod_id < b.mod_id;
    });

    const uint64_t now = now_epoch_seconds();
    preset.created_at = now;
    preset.updated_at = now;
    preset.id = make_preset_id(name, now);

    return save_preset(staging_dir, preset);
}

ModPreset PresetManager::apply_preset(const fs::path& staging_dir, const std::string& preset_id) {
    ModPreset applied;
    bool found = false;
    for (const auto& preset : list_presets(staging_dir)) {
        if (preset.id == preset_id) {
            applied = preset;
            found = true;
            break;
        }
    }
    if (!found) {
        fail(ErrorCode::ModNotFound, "Preset '" + preset_id + "' not found", staging_dir);
    }

    const ScanOutcome outcome = ModLoader::scan_mods_directory(staging_dir);
    for (const auto& mod : outcome.mods) {
        const auto it = std::find_if(applied.mods.begin(), applied.mods.end(),
                                     [&mod](const ModPresetEntry& entry) {
                                         return entry.mod_id == mod.info.id;
                                     });
        if (it != applied.mods.end()) {
            if (!mod.info.enabled) {
                ModManager::set_mod_enabled(staging_dir, mod.info.id, true);
            }
            if (mod.info.priority != it->priority) {
                ModManager::set_mod_priority(staging_dir, mod.info.id, it->priority);
            }
        } else if (mod.info.enabled) {
            // A preset is a complete statement of intent, so anything it does not mention is off.
            ModManager::set_mod_enabled(staging_dir, mod.info.id, false);
        }
    }

    return applied;
}

void PresetManager::delete_preset(const fs::path& staging_dir, const std::string& preset_id) {
    std::vector<ModPreset> presets = list_presets(staging_dir);
    const std::size_t before = presets.size();
    presets.erase(std::remove_if(presets.begin(), presets.end(), [&preset_id](const ModPreset& preset) {
                      return preset.id == preset_id;
                  }),
                  presets.end());

    if (presets.size() == before) {
        fail(ErrorCode::ModNotFound, "Preset '" + preset_id + "' not found", staging_dir);
    }

    const std::string content = json(presets).dump(2);
    std::ofstream stream(presets_path(staging_dir), std::ios::binary | std::ios::trunc);
    if (!stream) {
        fail(ErrorCode::Io, "Cannot write presets file in " + staging_dir.string(), staging_dir);
    }
    stream << content;
    stream.flush();
    if (!stream) {
        fail(ErrorCode::Io, "Failed to write presets file in " + staging_dir.string(), staging_dir);
    }
}

} // namespace smm
