use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};

use crate::error::{Result, SmmError};
use crate::loader::ModLoader;
use crate::manager::ModManager;
use crate::types::{ModPreset, ModPresetEntry};

pub const PRESETS_FILE_NAME: &str = ".smm_presets.json";

/// Management of user activation presets.
pub struct PresetManager;

impl PresetManager {
    /// Path to the presets file in the staging directory.
    pub fn presets_path(staging_dir: &Path) -> PathBuf {
        staging_dir.join(PRESETS_FILE_NAME)
    }

    /// Reads all saved presets from the staging directory.
    pub fn list_presets(staging_dir: &Path) -> Result<Vec<ModPreset>> {
        let path = Self::presets_path(staging_dir);
        if !path.exists() {
            return Ok(Vec::new());
        }

        let content = std::fs::read_to_string(&path)?;
        let presets: Vec<ModPreset> = serde_json::from_str(&content).map_err(|e| {
            SmmError::InvalidMetadata {
                path: path.clone(),
                message: format!("Failed to parse {}: {}", PRESETS_FILE_NAME, e),
            }
        })?;

        Ok(presets)
    }

    /// Saves or updates a preset.
    pub fn save_preset(staging_dir: &Path, preset: ModPreset) -> Result<ModPreset> {
        let mut presets = Self::list_presets(staging_dir)?;
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_secs();

        let mut saved = preset.clone();
        saved.updated_at = now;
        if saved.created_at == 0 {
            saved.created_at = now;
        }

        if let Some(idx) = presets.iter().position(|p| p.id == saved.id) {
            presets[idx] = saved.clone();
        } else {
            presets.push(saved.clone());
        }

        Self::write_presets(staging_dir, &presets)?;
        Ok(saved)
    }

    /// Creates and saves a new preset capturing the currently active/enabled mods and their priorities.
    pub fn create_preset_from_current(
        staging_dir: &Path,
        name: &str,
        description: Option<String>,
    ) -> Result<ModPreset> {
        let mods = ModLoader::scan_mods_directory(staging_dir)?;
        let mut enabled_entries = Vec::new();

        for (m, _) in mods {
            if m.enabled {
                enabled_entries.push(ModPresetEntry {
                    mod_id: m.id,
                    priority: m.priority,
                });
            }
        }

        // Sort by priority ascending
        enabled_entries.sort_by_key(|e| e.priority);

        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_secs();

        let id = format!(
            "preset_{}_{}",
            name.to_lowercase()
                .chars()
                .map(|c| if c.is_alphanumeric() { c } else { '_' })
                .collect::<String>()
                .trim_matches('_'),
            now
        );

        let preset = ModPreset {
            id,
            name: name.to_string(),
            description,
            created_at: now,
            updated_at: now,
            mods: enabled_entries,
        };

        Self::save_preset(staging_dir, preset)
    }

    /// Applies a preset to staging: enables mods in the preset with their exact priorities, and disables all other mods.
    pub fn apply_preset(staging_dir: &Path, preset_id: &str) -> Result<ModPreset> {
        let presets = Self::list_presets(staging_dir)?;
        let preset = presets
            .into_iter()
            .find(|p| p.id == preset_id)
            .ok_or_else(|| SmmError::ModNotFound(preset_id.to_string()))?;

        let staged_mods = ModLoader::scan_mods_directory(staging_dir)?;

        for (m, _) in staged_mods {
            if let Some(entry) = preset.mods.iter().find(|e| e.mod_id == m.id) {
                // Mod is in preset: enable it and update priority
                if !m.enabled {
                    ModManager::set_enabled(staging_dir, &m.id, true)?;
                }
                if m.priority != entry.priority {
                    ModManager::set_priority(staging_dir, &m.id, entry.priority)?;
                }
            } else {
                // Mod is not in preset: disable it
                if m.enabled {
                    ModManager::set_enabled(staging_dir, &m.id, false)?;
                }
            }
        }

        Ok(preset)
    }

    /// Deletes a preset by its identifier.
    pub fn delete_preset(staging_dir: &Path, preset_id: &str) -> Result<()> {
        let mut presets = Self::list_presets(staging_dir)?;
        let initial_len = presets.len();
        presets.retain(|p| p.id != preset_id);

        if presets.len() == initial_len {
            return Err(SmmError::ModNotFound(format!(
                "Preset '{}' not found",
                preset_id
            )));
        }

        Self::write_presets(staging_dir, &presets)?;
        Ok(())
    }

    fn write_presets(staging_dir: &Path, presets: &[ModPreset]) -> Result<()> {
        let path = Self::presets_path(staging_dir);
        let json = serde_json::to_string_pretty(presets)?;
        std::fs::write(&path, json)?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::types::ModInfo;
    use tempfile::tempdir;

    fn create_mock_mod(staging_dir: &Path, id: &str, name: &str, priority: u32, enabled: bool) {
        let mod_dir = staging_dir.join(id);
        std::fs::create_dir_all(mod_dir.join("parts")).unwrap();
        std::fs::write(
            mod_dir.join("parts/wp_a_0300.partsbnd.dcx"),
            b"mock_weapon",
        )
        .unwrap();

        let mut info = ModInfo::new(id, name, "1.0.0", "Community", "weapon_skin");
        info.priority = priority;
        info.enabled = enabled;
        let json = serde_json::to_string_pretty(&info).unwrap();
        std::fs::write(mod_dir.join(".smm_mod.json"), json).unwrap();
    }

    #[test]
    fn test_preset_lifecycle() {
        let tmp = tempdir().unwrap();
        let staging = tmp.path();

        create_mock_mod(staging, "mod-a", "Mod A", 100, true);
        create_mock_mod(staging, "mod-b", "Mod B", 50, true);
        create_mock_mod(staging, "mod-c", "Mod C", 20, false);

        // 1. Initial presets list is empty
        let initial = PresetManager::list_presets(staging).unwrap();
        assert!(initial.is_empty());

        // 2. Create preset from current state -> should capture mod-a and mod-b
        let preset1 = PresetManager::create_preset_from_current(
            staging,
            "Boss Battle Pack",
            Some("Optimized for combat".to_string()),
        )
        .unwrap();

        assert_eq!(preset1.name, "Boss Battle Pack");
        assert_eq!(preset1.mods.len(), 2);
        assert_eq!(preset1.mods[0].mod_id, "mod-b"); // sorted by priority 50
        assert_eq!(preset1.mods[1].mod_id, "mod-a"); // priority 100

        // 3. List presets
        let list = PresetManager::list_presets(staging).unwrap();
        assert_eq!(list.len(), 1);
        assert_eq!(list[0].id, preset1.id);

        // 4. Change states: enable mod-c, disable mod-a
        ModManager::set_enabled(staging, "mod-c", true).unwrap();
        ModManager::set_enabled(staging, "mod-a", false).unwrap();

        let current_mods = ModLoader::scan_mods_directory(staging).unwrap();
        let mod_a = current_mods.iter().find(|(m, _)| m.id == "mod-a").unwrap();
        let mod_c = current_mods.iter().find(|(m, _)| m.id == "mod-c").unwrap();
        assert!(!mod_a.0.enabled);
        assert!(mod_c.0.enabled);

        // 5. Apply preset1 -> restores mod-a and mod-b enabled, mod-c disabled
        PresetManager::apply_preset(staging, &preset1.id).unwrap();

        let restored_mods = ModLoader::scan_mods_directory(staging).unwrap();
        let res_a = restored_mods.iter().find(|(m, _)| m.id == "mod-a").unwrap();
        let res_b = restored_mods.iter().find(|(m, _)| m.id == "mod-b").unwrap();
        let res_c = restored_mods.iter().find(|(m, _)| m.id == "mod-c").unwrap();
        assert!(res_a.0.enabled);
        assert_eq!(res_a.0.priority, 100);
        assert!(res_b.0.enabled);
        assert_eq!(res_b.0.priority, 50);
        assert!(!res_c.0.enabled);

        // 6. Delete preset
        PresetManager::delete_preset(staging, &preset1.id).unwrap();
        let after_delete = PresetManager::list_presets(staging).unwrap();
        assert!(after_delete.is_empty());
    }
}
