use std::path::PathBuf;

use serde::{Deserialize, Serialize};
use smm_core::{AssetEntry, ModInfo, ModLoader, ModManager};

/// Enriched mod item for list view.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ModSummary {
    #[serde(flatten)]
    pub info: ModInfo,
    pub asset_count: usize,
    pub total_size: u64,
}

/// Detailed mod view payload containing asset tree entries.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ModDetailsPayload {
    pub info: ModInfo,
    pub assets: Vec<AssetEntry>,
    pub total_size: u64,
    pub total_assets: usize,
}

fn require_staging(staging_dir: &str) -> Result<PathBuf, String> {
    let path = PathBuf::from(staging_dir);
    if !path.exists() {
        return Err(format!(
            "Staging directory does not exist: {}",
            staging_dir
        ));
    }
    Ok(path)
}

fn summarize((info, assets): (ModInfo, Vec<AssetEntry>)) -> ModSummary {
    ModSummary {
        total_size: assets.iter().map(|a| a.file_size).sum(),
        asset_count: assets.len(),
        info,
    }
}

/// Lists all mods currently in the staging directory with asset count.
#[tauri::command]
pub fn list_mods(staging_dir: String) -> Result<Vec<ModSummary>, String> {
    let path = require_staging(&staging_dir)?;
    ModLoader::scan_mods_directory(&path)
        .map(|scanned| scanned.into_iter().map(summarize).collect())
        .map_err(|e| format!("Failed to scan mods directory {}: {}", staging_dir, e))
}

/// Retrieves details and normalized asset entries for a specific mod.
#[tauri::command]
pub fn get_mod_details(
    staging_dir: String,
    mod_id: String,
) -> Result<ModDetailsPayload, String> {
    let path = PathBuf::from(&staging_dir);
    let (info, assets) = ModManager::get_details(&path, &mod_id)
        .map_err(|e| format!("Failed to load mod '{}' details: {}", mod_id, e))?;

    Ok(ModDetailsPayload {
        total_size: assets.iter().map(|a| a.file_size).sum(),
        total_assets: assets.len(),
        info,
        assets,
    })
}

/// Toggles the enabled/disabled state of a mod.
#[tauri::command]
pub fn toggle_mod(staging_dir: String, mod_id: String, enabled: bool) -> Result<ModInfo, String> {
    let path = PathBuf::from(&staging_dir);
    ModManager::set_enabled(&path, &mod_id, enabled)
        .map_err(|e| format!("Failed to toggle mod '{}': {}", mod_id, e))
}

/// Updates the priority of a single mod.
#[tauri::command]
pub fn set_mod_priority(
    staging_dir: String,
    mod_id: String,
    priority: u32,
) -> Result<ModInfo, String> {
    let path = PathBuf::from(&staging_dir);
    ModManager::set_priority(&path, &mod_id, priority)
        .map_err(|e| format!("Failed to set priority for mod '{}': {}", mod_id, e))
}

/// Reorders a list of mods according to drag-and-drop or batch reordering.
#[tauri::command]
pub fn reorder_mods(staging_dir: String, ordered_ids: Vec<String>) -> Result<Vec<ModInfo>, String> {
    let path = PathBuf::from(&staging_dir);
    ordered_ids
        .iter()
        .enumerate()
        .map(|(idx, id)| {
            ModManager::set_priority(&path, id, ((idx + 1) * 10) as u32)
                .map_err(|e| format!("Failed to reorder mod '{}': {}", id, e))
        })
        .collect()
}

/// Permanently deletes a mod directory from staging.
#[tauri::command]
pub fn delete_mod(staging_dir: String, mod_id: String) -> Result<(), String> {
    let path = PathBuf::from(&staging_dir);
    ModManager::delete(&path, &mod_id)
        .map_err(|e| format!("Failed to delete mod '{}': {}", mod_id, e))
}

/// Updates mod metadata (source_url, homepage, author, version, etc.) and persists it.
#[tauri::command]
pub fn update_mod_info(
    staging_dir: String,
    mod_id: String,
    info: ModInfo,
) -> Result<ModInfo, String> {
    let path = PathBuf::from(&staging_dir);
    smm_core::update_mod_info(&path, &mod_id, &info)
        .map_err(|e| format!("Failed to update mod '{}' info: {}", mod_id, e))
}