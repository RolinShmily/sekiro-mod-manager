use std::fs;
use std::path::PathBuf;

use serde::{Deserialize, Serialize};

use crate::commands::util;

/// Application persistent settings.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AppSettings {
    pub game_dir: String,
    pub staging_dir: String,
    #[serde(default)]
    pub active_modal: Option<String>,
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
    let detected_staging = probe_staging_dir();
    let detected_game = probe_game_dir();

    AppSettings {
        game_dir: detected_game,
        staging_dir: detected_staging,
        active_modal: None,
    }
}

/// Finds an existing staging directory relative to the working directory.
fn probe_staging_dir() -> String {
    for cand in util::staging_candidates() {
        if cand.exists() && cand.is_dir() {
            if let Ok(abs) = cand.canonicalize() {
                return abs.to_string_lossy().to_string();
            }
        }
    }
    "staging".to_string()
}

/// Finds an installed Sekiro installation via Steam library paths or the working directory.
fn probe_game_dir() -> String {
    for cand in util::steam_install_candidates() {
        let p = PathBuf::from(cand);
        if p.exists() && p.join("sekiro.exe").is_file() {
            return cand.to_string();
        }
    }

    let cwd = std::env::current_dir().unwrap_or_else(|_| PathBuf::from("."));
    if cwd.join("sekiro.exe").is_file() {
        cwd.to_string_lossy().to_string()
    } else {
        String::new()
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
    fs::write(&path, json)
        .map_err(|e| format!("Failed to write settings to {}: {}", path.display(), e))?;
    Ok(settings)
}
