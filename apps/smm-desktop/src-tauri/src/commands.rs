use std::fs;
use std::path::PathBuf;

use serde::{Deserialize, Serialize};
use smm_core::{
    diagnose_environment, execute_deploy, import_mod, install_mod_engine, restore_deploy,
    AssetEntry, ConflictEngine, ConflictReport, DeployResult, DeploymentPlanner, HealthReport,
    ImportOptions, ModInfo, ModLoader, ModManager, ModPackManifest, ModPreset, PresetManager,
    RestoreResult,
};

/// Application persistent settings.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AppSettings {
    pub game_dir: String,
    pub staging_dir: String,
    #[serde(default)]
    pub active_modal: Option<String>,
}

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

/// Helper function to locate settings file.
fn settings_file_path() -> PathBuf {
    // Check current directory first
    let local = PathBuf::from(".smm_settings.json");
    if local.exists() {
        return local;
    }

    // Try user config / home directory
    if let Ok(home) = std::env::var("USERPROFILE").or_else(|_| std::env::var("HOME")) {
        let p = PathBuf::from(home).join(".smm_settings.json");
        return p;
    }

    local
}

/// Probes the system for default Sekiro game and staging directory locations.
fn probe_default_settings() -> AppSettings {
    // 1. Staging directory probe (clean production staging directory)
    let staging_candidates = [
        PathBuf::from("staging"),
        PathBuf::from("../staging"),
        PathBuf::from("../../staging"),
        PathBuf::from("../../../staging"),
        PathBuf::from("mods_staging"),
        PathBuf::from("../mods_staging"),
    ];

    let mut detected_staging = String::new();
    for cand in &staging_candidates {
        if cand.exists() && cand.is_dir() {
            if let Ok(abs) = cand.canonicalize() {
                detected_staging = abs.to_string_lossy().to_string();
                break;
            }
        }
    }

    if detected_staging.is_empty() {
        detected_staging = "staging".to_string();
    }

    // 2. Sekiro game directory probe
    let game_candidates = [
        r"C:\Program Files (x86)\Steam\steamapps\common\Sekiro",
        r"C:\Program Files\Steam\steamapps\common\Sekiro",
        r"D:\SteamLibrary\steamapps\common\Sekiro",
        r"E:\SteamLibrary\steamapps\common\Sekiro",
    ];

    let mut detected_game = String::new();
    for cand in &game_candidates {
        let p = PathBuf::from(cand);
        if p.exists() && p.join("sekiro.exe").is_file() {
            detected_game = cand.to_string();
            break;
        }
    }

    if detected_game.is_empty() {
        // Fallback: check cwd or relative test environment
        let cwd = std::env::current_dir().unwrap_or_else(|_| PathBuf::from("."));
        if cwd.join("sekiro.exe").is_file() {
            detected_game = cwd.to_string_lossy().to_string();
        } else {
            detected_game = String::new();
        }
    }

    AppSettings {
        game_dir: detected_game,
        staging_dir: detected_staging,
        active_modal: None,
    }
}

/// Retrieves application settings.
#[tauri::command]
pub fn get_settings() -> Result<AppSettings, String> {
    let path = settings_file_path();
    if path.exists() {
        if let Ok(content) = fs::read_to_string(&path) {
            if let Ok(settings) = serde_json::from_str::<AppSettings>(&content) {
                return Ok(settings);
            }
        }
    }
    Ok(probe_default_settings())
}

/// Persists application settings to disk.
#[tauri::command]
pub fn save_settings(settings: AppSettings) -> Result<AppSettings, String> {
    let path = settings_file_path();
    let json = serde_json::to_string_pretty(&settings).map_err(|e| e.to_string())?;
    fs::write(&path, json).map_err(|e| format!("Failed to write settings to {}: {}", path.display(), e))?;
    Ok(settings)
}

/// Lists all mods currently in the staging directory with asset count.
#[tauri::command]
pub fn list_mods(staging_dir: String) -> Result<Vec<ModSummary>, String> {
    let path = PathBuf::from(&staging_dir);
    if !path.exists() {
        return Err(format!("Staging directory does not exist: {}", staging_dir));
    }

    let scanned = ModLoader::scan_mods_directory(&path)
        .map_err(|e| format!("Failed to scan mods directory {}: {}", staging_dir, e))?;

    let summaries: Vec<ModSummary> = scanned
        .into_iter()
        .map(|(info, assets)| {
            let total_size: u64 = assets.iter().map(|a| a.file_size).sum();
            let asset_count = assets.len();
            ModSummary {
                info,
                asset_count,
                total_size,
            }
        })
        .collect();

    Ok(summaries)
}

/// Retrieves details and normalized asset entries for a specific mod.
#[tauri::command]
pub fn get_mod_details(staging_dir: String, mod_id: String) -> Result<ModDetailsPayload, String> {
    let path = PathBuf::from(&staging_dir);
    let (info, assets) = ModManager::get_details(&path, &mod_id)
        .map_err(|e| format!("Failed to load mod '{}' details: {}", mod_id, e))?;

    let total_size: u64 = assets.iter().map(|a| a.file_size).sum();
    let total_assets = assets.len();

    Ok(ModDetailsPayload {
        info,
        assets,
        total_size,
        total_assets,
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
pub fn set_mod_priority(staging_dir: String, mod_id: String, priority: u32) -> Result<ModInfo, String> {
    let path = PathBuf::from(&staging_dir);
    ModManager::set_priority(&path, &mod_id, priority)
        .map_err(|e| format!("Failed to set priority for mod '{}': {}", mod_id, e))
}

/// Reorders a list of mods according to drag-and-drop or batch reordering.
#[tauri::command]
pub fn reorder_mods(staging_dir: String, ordered_ids: Vec<String>) -> Result<Vec<ModInfo>, String> {
    let path = PathBuf::from(&staging_dir);
    let mut updated_mods = Vec::new();

    for (idx, id) in ordered_ids.iter().enumerate() {
        let new_priority = ((idx + 1) * 10) as u32;
        let updated = ModManager::set_priority(&path, id, new_priority)
            .map_err(|e| format!("Failed to reorder mod '{}': {}", id, e))?;
        updated_mods.push(updated);
    }

    Ok(updated_mods)
}

/// Permanently deletes a mod directory from staging.
#[tauri::command]
pub fn delete_mod(staging_dir: String, mod_id: String) -> Result<(), String> {
    let path = PathBuf::from(&staging_dir);
    ModManager::delete(&path, &mod_id)
        .map_err(|e| format!("Failed to delete mod '{}': {}", mod_id, e))
}

/// Imports a mod package (zip, 7z, or folder) into staging.
#[tauri::command]
pub fn import_mod_file(
    source_path: String,
    staging_dir: String,
    source_url: Option<String>,
) -> Result<ModInfo, String> {
    let src = PathBuf::from(&source_path);
    let stg = PathBuf::from(&staging_dir);

    if !src.exists() {
        return Err(format!("Source mod path does not exist: {}", source_path));
    }

    let options = ImportOptions {
        custom_id: None,
        custom_name: None,
        priority: None,
        overwrite: true,
        source_url,
    };

    import_mod(&src, &stg, &options)
        .map_err(|e| format!("Failed to import mod from '{}': {}", source_path, e))
}

/// Scans active mods in staging and returns collision / shadowing matrix report.
#[tauri::command]
pub fn scan_conflicts(staging_dir: String) -> Result<ConflictReport, String> {
    let path = PathBuf::from(&staging_dir);
    if !path.exists() {
        return Err(format!("Staging directory does not exist: {}", staging_dir));
    }

    let mods = ModLoader::scan_mods_directory(&path)
        .map_err(|e| format!("Failed to read mods from {}: {}", staging_dir, e))?;

    let report = ConflictEngine::scan_conflicts(&mods);
    Ok(report)
}

/// Executes Win32 NTFS hard link deployment of enabled mods into the Sekiro game directory.
#[tauri::command]
pub fn deploy_mods(game_dir: String, staging_dir: String) -> Result<DeployResult, String> {
    let g_path = PathBuf::from(&game_dir);
    let s_path = PathBuf::from(&staging_dir);

    if !s_path.exists() {
        return Err(format!("Staging directory does not exist: {}", staging_dir));
    }

    let mods = ModLoader::scan_mods_directory(&s_path)
        .map_err(|e| format!("Failed to read mods from {}: {}", staging_dir, e))?;

    if mods.is_empty() {
        return Err("No mods found in staging directory to deploy.".to_string());
    }

    let plan = DeploymentPlanner::build_plan("default", &mods)
        .map_err(|e| format!("Failed to build deployment plan: {}", e))?;

    let target_mods_dir = if g_path
        .file_name()
        .map(|n| n.to_string_lossy().eq_ignore_ascii_case("mods"))
        .unwrap_or(false)
    {
        g_path
    } else {
        g_path.join("mods")
    };

    execute_deploy(&plan, &target_mods_dir)
        .map_err(|e| format!("Deployment execution failed: {}", e))
}

/// Restores game directory to vanilla state by removing deployed hardlinks and files.
#[tauri::command]
pub fn restore_mods(game_dir: String) -> Result<RestoreResult, String> {
    let g_path = PathBuf::from(&game_dir);
    let target_mods_dir = if g_path
        .file_name()
        .map(|n| n.to_string_lossy().eq_ignore_ascii_case("mods"))
        .unwrap_or(false)
    {
        g_path
    } else {
        g_path.join("mods")
    };

    restore_deploy(&target_mods_dir)
        .map_err(|e| format!("Restore execution failed: {}", e))
}

/// Performs multi-dimensional health check on the Sekiro game directory and ModEngine setup.
#[tauri::command]
pub fn diagnose_env(game_dir: String, staging_dir: String) -> Result<HealthReport, String> {
    let g_path = PathBuf::from(&game_dir);
    let s_path = if staging_dir.trim().is_empty() {
        None
    } else {
        Some(PathBuf::from(&staging_dir))
    };

    let report = diagnose_environment(&g_path, s_path.as_deref());
    Ok(report)
}

/// Installs or patches ModEngine and dinput8.dll into the game directory.
#[tauri::command]
pub fn setup_mod_engine(game_dir: String, staging_dir: String) -> Result<(), String> {
    let g_path = PathBuf::from(&game_dir);
    let s_path = if staging_dir.trim().is_empty() {
        None
    } else {
        Some(PathBuf::from(&staging_dir))
    };

    install_mod_engine(&g_path, s_path.as_deref())
        .map_err(|e| format!("Failed to setup ModEngine: {}", e))
}

/// Provisions or downloads Sekiro Mod Engine into the staging directory as a managed Mod in the list.
#[tauri::command]
pub fn provision_engine_mod(staging_dir: String) -> Result<ModInfo, String> {
    let s_path = PathBuf::from(&staging_dir);
    smm_core::provision_mod_engine(&s_path)
        .map_err(|e| format!("Failed to provision ModEngine: {}", e))
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

/// Exports a single mod to a .zip archive.
#[tauri::command]
pub fn export_single_mod(
    staging_dir: String,
    mod_id: String,
    output_path: String,
    include_source: bool,
) -> Result<String, String> {
    let s_path = PathBuf::from(&staging_dir);
    let out_path = PathBuf::from(&output_path);
    smm_core::export_single_mod(&s_path, &mod_id, &out_path, include_source)
        .map(|p| p.to_string_lossy().to_string())
        .map_err(|e| format!("Failed to export mod '{}': {}", mod_id, e))
}

/// Exports multiple mods into an .smmpack archive.
#[tauri::command]
pub fn export_modpack(
    staging_dir: String,
    mod_ids: Vec<String>,
    name: String,
    version: String,
    author: Option<String>,
    description: Option<String>,
    output_path: String,
    include_source: bool,
) -> Result<String, String> {
    let s_path = PathBuf::from(&staging_dir);
    let out_path = PathBuf::from(&output_path);

    let mut manifest = ModPackManifest::new(name, version);
    manifest.author = author;
    manifest.description = description;

    smm_core::export_modpack(&s_path, &mod_ids, manifest, &out_path, include_source)
        .map(|p| p.to_string_lossy().to_string())
        .map_err(|e| format!("Failed to export modpack: {}", e))
}

/// Imports an .smmpack archive into staging.
#[tauri::command]
pub fn import_modpack(
    staging_dir: String,
    pack_path: String,
    overwrite: bool,
) -> Result<ModPackManifest, String> {
    let s_path = PathBuf::from(&staging_dir);
    let p_path = PathBuf::from(&pack_path);

    smm_core::import_modpack(&p_path, &s_path, overwrite)
        .map_err(|e| format!("Failed to import modpack from '{}': {}", pack_path, e))
}

/// Opens native Windows file explorer folder selection dialog.
#[tauri::command]
pub async fn pick_folder(
    title: Option<String>,
    default_path: Option<String>,
) -> Result<Option<String>, String> {
    let mut dialog = rfd::AsyncFileDialog::new();
    if let Some(ref t) = title {
        dialog = dialog.set_title(t);
    }
    if let Some(ref p) = default_path {
        if !p.trim().is_empty() {
            let path = PathBuf::from(p);
            if path.exists() {
                dialog = dialog.set_directory(&path);
            }
        }
    }

    let folder = dialog.pick_folder().await;
    Ok(folder.map(|h| h.path().to_string_lossy().to_string()))
}

/// Opens native Windows file explorer file selection dialog.
#[tauri::command]
pub async fn pick_file(
    title: Option<String>,
    default_path: Option<String>,
    filter_name: Option<String>,
    extensions: Option<Vec<String>>,
) -> Result<Option<String>, String> {
    let mut dialog = rfd::AsyncFileDialog::new();
    if let Some(ref t) = title {
        dialog = dialog.set_title(t);
    }
    if let Some(ref p) = default_path {
        if !p.trim().is_empty() {
            let path = PathBuf::from(p);
            if path.exists() {
                dialog = dialog.set_directory(&path);
            }
        }
    }
    if let (Some(name), Some(exts)) = (filter_name, extensions) {
        let ext_refs: Vec<&str> = exts.iter().map(|s| s.as_str()).collect();
        dialog = dialog.add_filter(&name, &ext_refs);
    }

    let file = dialog.pick_file().await;
    Ok(file.map(|h| h.path().to_string_lossy().to_string()))
}

/// Opens a URL in the user's default system browser.
#[tauri::command]
pub async fn open_external_url(url: String) -> Result<(), String> {
    let trimmed = url.trim();
    if trimmed.is_empty() {
        return Err("URL cannot be empty".to_string());
    }
    let target = if trimmed.starts_with("http://") || trimmed.starts_with("https://") {
        trimmed.to_string()
    } else {
        format!("https://{}", trimmed)
    };

    open::that_detached(&target).map_err(|e| format!("Failed to open URL in browser: {}", e))
}

/// Opens native Windows file explorer multiple files selection dialog.
#[tauri::command]
pub async fn pick_files(
    title: Option<String>,
    default_path: Option<String>,
    filter_name: Option<String>,
    extensions: Option<Vec<String>>,
) -> Result<Option<Vec<String>>, String> {
    let mut dialog = rfd::AsyncFileDialog::new();
    if let Some(ref t) = title {
        dialog = dialog.set_title(t);
    }
    if let Some(ref p) = default_path {
        if !p.trim().is_empty() {
            let path = PathBuf::from(p);
            if path.exists() {
                dialog = dialog.set_directory(&path);
            }
        }
    }
    if let (Some(name), Some(exts)) = (filter_name, extensions) {
        let ext_refs: Vec<&str> = exts.iter().map(|s| s.as_str()).collect();
        dialog = dialog.add_filter(&name, &ext_refs);
    }

    let files = dialog.pick_files().await;
    Ok(files.map(|list| {
        list.into_iter()
            .map(|h| h.path().to_string_lossy().to_string())
            .collect()
    }))
}

/// Imports multiple files merged as a single mod package.
#[tauri::command]
pub fn import_merged_mod_files(
    source_paths: Vec<String>,
    staging_dir: String,
    custom_name: Option<String>,
    source_url: Option<String>,
) -> Result<ModInfo, String> {
    let paths: Vec<PathBuf> = source_paths.into_iter().map(PathBuf::from).collect();
    let stg = PathBuf::from(&staging_dir);

    let options = ImportOptions {
        custom_id: None,
        custom_name,
        priority: None,
        overwrite: true,
        source_url,
    };

    smm_core::import_multiple_files_as_mod(&paths, &stg, &options)
        .map_err(|e| format!("Failed to import merged mods: {}", e))
}

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
pub fn save_preset(
    staging_dir: String,
    preset: ModPreset,
) -> Result<ModPreset, String> {
    let stg = PathBuf::from(&staging_dir);
    PresetManager::save_preset(&stg, preset)
        .map_err(|e| format!("Failed to save preset: {}", e))
}

/// Applies a preset: enables matching mods with priorities, disables non-matching mods.
#[tauri::command]
pub fn apply_preset(
    staging_dir: String,
    preset_id: String,
) -> Result<ModPreset, String> {
    let stg = PathBuf::from(&staging_dir);
    PresetManager::apply_preset(&stg, &preset_id)
        .map_err(|e| format!("Failed to apply preset: {}", e))
}

/// Deletes a preset by ID.
#[tauri::command]
pub fn delete_preset(
    staging_dir: String,
    preset_id: String,
) -> Result<(), String> {
    let stg = PathBuf::from(&staging_dir);
    PresetManager::delete_preset(&stg, &preset_id)
        .map_err(|e| format!("Failed to delete preset: {}", e))
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::Path;
    use tempfile::tempdir;

    fn create_test_staging(dir: &Path) {
        let kr = dir.join("kusabimaru-reaper");
        let wp = kr.join("parts/wp_a_0300.partsbnd.dcx");
        std::fs::create_dir_all(wp.parent().unwrap()).unwrap();
        std::fs::write(&wp, b"KATANA_MESH_MOCK").unwrap();
        let kr_info = ModInfo {
            id: "kusabimaru-reaper".to_string(),
            name: "Kusabimaru Reaper".to_string(),
            version: "1.0.0".to_string(),
            author: "Author".to_string(),
            description: None,
            category: "weapon_skin".to_string(),
            license: None,
            enabled: true,
            priority: 10,
            tags: Vec::new(),
            root_path: None,
            source_url: None,
            homepage: None,
        };
        std::fs::write(kr.join("mod.json"), serde_json::to_string(&kr_info).unwrap()).unwrap();

        let ps4 = dir.join("native-ps4-buttons");
        let btn = ps4.join("menu/menu.menubnd.dcx");
        std::fs::create_dir_all(btn.parent().unwrap()).unwrap();
        std::fs::write(&btn, b"PS4_BUTTON_MOCK").unwrap();
        let ps4_info = ModInfo {
            id: "native-ps4-buttons".to_string(),
            name: "Native PS4 Buttons".to_string(),
            version: "1.0.0".to_string(),
            author: "Author".to_string(),
            description: None,
            category: "ui".to_string(),
            license: None,
            enabled: true,
            priority: 50,
            tags: Vec::new(),
            root_path: None,
            source_url: None,
            homepage: None,
        };
        std::fs::write(ps4.join("mod.json"), serde_json::to_string(&ps4_info).unwrap()).unwrap();

        // Conflict mod for scan_conflicts test
        let conflict = dir.join("conflict-reaper");
        let c_wp = conflict.join("parts/wp_a_0300.partsbnd.dcx");
        std::fs::create_dir_all(c_wp.parent().unwrap()).unwrap();
        std::fs::write(&c_wp, b"CONFLICTING_KATANA_MESH").unwrap();
        let c_info = ModInfo {
            id: "conflict-reaper".to_string(),
            name: "Conflict Reaper".to_string(),
            version: "1.0.0".to_string(),
            author: "Author".to_string(),
            description: None,
            category: "weapon_skin".to_string(),
            license: None,
            enabled: true,
            priority: 20,
            tags: Vec::new(),
            root_path: None,
            source_url: None,
            homepage: None,
        };
        std::fs::write(conflict.join("mod.json"), serde_json::to_string(&c_info).unwrap()).unwrap();
    }

    #[test]
    fn test_get_settings_probe() {
        let settings = get_settings().expect("Failed to get settings");
        assert!(!settings.staging_dir.is_empty());
    }

    #[test]
    fn test_list_mods_from_fixtures() {
        let tmp = tempdir().unwrap();
        create_test_staging(tmp.path());
        let mods = list_mods(tmp.path().to_string_lossy().to_string()).expect("list_mods failed");
        assert!(!mods.is_empty());
        let ids: Vec<&str> = mods.iter().map(|m| m.info.id.as_str()).collect();
        assert!(ids.contains(&"kusabimaru-reaper"));
        assert!(ids.contains(&"native-ps4-buttons"));
    }

    #[test]
    fn test_get_mod_details() {
        let tmp = tempdir().unwrap();
        create_test_staging(tmp.path());
        let details = get_mod_details(
            tmp.path().to_string_lossy().to_string(),
            "kusabimaru-reaper".to_string(),
        )
        .expect("get_mod_details failed");

        assert_eq!(details.info.id, "kusabimaru-reaper");
        assert!(!details.assets.is_empty());
        assert!(details.total_size > 0);
    }

    #[test]
    fn test_scan_conflicts() {
        let tmp = tempdir().unwrap();
        create_test_staging(tmp.path());
        let report = scan_conflicts(tmp.path().to_string_lossy().to_string()).expect("scan_conflicts failed");
        assert!(report.total_conflicts > 0);
    }

    #[test]
    fn test_deploy_and_restore_cycle() {
        let tmp = tempdir().unwrap();
        create_test_staging(tmp.path());
        let mock_game = tmp.path().join("SekiroGame");
        std::fs::create_dir_all(&mock_game).unwrap();

        let deploy_res = deploy_mods(
            mock_game.to_string_lossy().to_string(),
            tmp.path().to_string_lossy().to_string(),
        )
        .expect("deploy_mods failed");

        assert!(deploy_res.is_success());
        assert!(mock_game.join("mods").exists());
        assert!(mock_game.join("mods").join(".smm_manifest.json").exists());

        let restore_res = restore_mods(mock_game.to_string_lossy().to_string()).expect("restore_mods failed");
        assert!(restore_res.success);
        assert!(!mock_game.join("mods").join(".smm_manifest.json").exists());
    }

    #[test]
    fn test_diagnose_and_setup_engine() {
        let tmp = tempdir().unwrap();
        create_test_staging(tmp.path());
        let mock_game = tmp.path().join("SekiroGame");
        std::fs::create_dir_all(&mock_game).unwrap();

        let _ = provision_engine_mod(tmp.path().to_string_lossy().to_string());

        let report = diagnose_env(
            mock_game.to_string_lossy().to_string(),
            tmp.path().to_string_lossy().to_string(),
        )
        .expect("diagnose_env failed");

        assert!(!report.items.is_empty());
    }

    #[test]
    fn test_update_mod_info_ipc() {
        let tmp = tempdir().unwrap();
        let staging = tmp.path().join("staging");
        create_test_staging(&staging);

        let dst_mod = staging.join("kusabimaru-reaper");
        let mut patch = ModLoader::load_mod_info(&dst_mod).unwrap();
        patch.name = "Reaper Katana Overhaul".to_string();
        patch.source_url = Some("https://nexusmods.com/sekiro/mods/999".to_string());
        patch.homepage = Some("https://github.com/example/reaper".to_string());

        let res = update_mod_info(
            staging.to_string_lossy().to_string(),
            "kusabimaru-reaper".to_string(),
            patch,
        )
        .expect("update_mod_info IPC failed");

        assert_eq!(res.name, "Reaper Katana Overhaul");
        assert_eq!(res.source_url.as_deref(), Some("https://nexusmods.com/sekiro/mods/999"));
        assert_eq!(res.homepage.as_deref(), Some("https://github.com/example/reaper"));
    }

    #[test]
    fn test_export_and_import_modpack_ipc() {
        let tmp = tempdir().unwrap();
        let staging_src = tmp.path().join("staging_src");
        let staging_dst = tmp.path().join("staging_dst");
        let out_dir = tmp.path().join("exports");
        std::fs::create_dir_all(&staging_src).unwrap();
        std::fs::create_dir_all(&staging_dst).unwrap();
        std::fs::create_dir_all(&out_dir).unwrap();

        create_test_staging(&staging_src);

        // 1. Single mod export IPC
        let single_zip = out_dir.join("single_reaper.zip").to_string_lossy().to_string();
        let exported_zip = export_single_mod(
            staging_src.to_string_lossy().to_string(),
            "kusabimaru-reaper".to_string(),
            single_zip.clone(),
            true,
        )
        .expect("export_single_mod IPC failed");
        assert!(PathBuf::from(&exported_zip).exists());

        // 2. Modpack export IPC
        let pack_path = out_dir.join("test_pack.smmpack").to_string_lossy().to_string();
        let exported_pack = export_modpack(
            staging_src.to_string_lossy().to_string(),
            vec!["kusabimaru-reaper".to_string(), "native-ps4-buttons".to_string()],
            "Test Pack".to_string(),
            "1.0.0".to_string(),
            Some("Author".to_string()),
            Some("Test Modpack Description".to_string()),
            pack_path.clone(),
            true,
        )
        .expect("export_modpack IPC failed");
        assert!(PathBuf::from(&exported_pack).exists());

        // 3. Modpack import IPC
        let manifest = import_modpack(
            staging_dst.to_string_lossy().to_string(),
            exported_pack,
            false,
        )
        .expect("import_modpack IPC failed");

        assert_eq!(manifest.name, "Test Pack");
        assert_eq!(manifest.mods.len(), 2);
        assert!(staging_dst.join("kusabimaru-reaper").exists());
        assert!(staging_dst.join("native-ps4-buttons").exists());
    }

    #[test]
    fn test_preset_ipc_cycle() {
        let tmp = tempdir().unwrap();
        let staging = tmp.path().to_string_lossy().to_string();
        create_test_staging(tmp.path());

        // 1. Initially 0 presets
        let list1 = list_presets(staging.clone()).expect("list_presets failed");
        assert!(list1.is_empty());

        // 2. Create preset from current state
        let preset = create_preset_from_current(
            staging.clone(),
            "Combat Build".to_string(),
            Some("Active mods preset".to_string()),
        )
        .expect("create_preset failed");

        assert_eq!(preset.name, "Combat Build");
        assert!(!preset.mods.is_empty());

        // 3. Apply preset
        let applied = apply_preset(staging.clone(), preset.id.clone()).expect("apply_preset failed");
        assert_eq!(applied.id, preset.id);

        // 4. Delete preset
        delete_preset(staging.clone(), preset.id).expect("delete_preset failed");
        let list2 = list_presets(staging).expect("list_presets after delete failed");
        assert!(list2.is_empty());
    }
}

