use std::path::PathBuf;

use smm_core::{ModPreset, PresetManager};

/// Lists all saved user activation presets.
#[tauri::command]
pub fn list_presets(staging_dir: String) -> Result<Vec<ModPreset>, String> {
    let stg = PathBuf::from(&staging_dir);
    PresetManager::list_presets(&stg)
        .map_err(|e| format!("Failed to list presets: {}", e))
}

/// Creates a new preset capturing current enabled mods and their priorities.
#[tauri::command]
pub fn create_preset_from_current(
    staging_dir: String,
    name: String,
    description: Option<String>,
) -> Result<ModPreset, String> {
    let stg = PathBuf::from(&staging_dir);
    PresetManager::create_preset_from_current(&stg, &name, description)
        .map_err(|e| format!("Failed to create preset: {}", e))
}

/// Saves or updates a preset.
#[tauri::command]
pub fn save_preset(staging_dir: String, preset: ModPreset) -> Result<ModPreset, String> {
    let stg = PathBuf::from(&staging_dir);
    PresetManager::save_preset(&stg, preset)
        .map_err(|e| format!("Failed to save preset: {}", e))
}

/// Applies a preset: enables matching mods with priorities, disables non-matching mods.
#[tauri::command]
pub fn apply_preset(staging_dir: String, preset_id: String) -> Result<ModPreset, String> {
    let stg = PathBuf::from(&staging_dir);
    PresetManager::apply_preset(&stg, &preset_id)
        .map_err(|e| format!("Failed to apply preset: {}", e))
}

/// Deletes a preset by ID.
#[tauri::command]
pub fn delete_preset(staging_dir: String, preset_id: String) -> Result<(), String> {
    let stg = PathBuf::from(&staging_dir);
    PresetManager::delete_preset(&stg, &preset_id)
        .map_err(|e| format!("Failed to delete preset: {}", e))
}