use std::path::{Path, PathBuf};

use crate::error::{Result, SmmError};
use crate::loader::ModLoader;
use crate::types::{AssetEntry, ModInfo};

/// Mod lifecycle and state management in staging directory.
pub struct ModManager;

impl ModManager {
    /// Locates the physical directory of a mod by its ID within the staging area.
    pub fn find_mod_dir(staging_dir: &Path, mod_id: &str) -> Result<PathBuf> {
        find_mod_dir(staging_dir, mod_id)
    }

    /// Updates the enabled/disabled state of a mod and persists it to `mod.json`.
    pub fn set_enabled(staging_dir: &Path, mod_id: &str, enabled: bool) -> Result<ModInfo> {
        set_mod_enabled(staging_dir, mod_id, enabled)
    }

    /// Updates the deployment priority number of a mod and persists it to `mod.json`.
    pub fn set_priority(staging_dir: &Path, mod_id: &str, priority: u32) -> Result<ModInfo> {
        set_mod_priority(staging_dir, mod_id, priority)
    }

    /// Updates mod metadata (source_url, homepage, author, version, etc.) and persists it.
    pub fn update_mod_info(staging_dir: &Path, mod_id: &str, updated: &ModInfo) -> Result<ModInfo> {
        update_mod_info(staging_dir, mod_id, updated)
    }

    /// Permanently removes a mod directory from the staging area.
    pub fn delete(staging_dir: &Path, mod_id: &str) -> Result<()> {
        delete_mod(staging_dir, mod_id)
    }

    /// Retrieves detailed metadata and normalized asset entries for a specific mod.
    pub fn get_details(staging_dir: &Path, mod_id: &str) -> Result<(ModInfo, Vec<AssetEntry>)> {
        get_mod_details(staging_dir, mod_id)
    }
}

/// Locates the physical directory of a mod by its ID within the staging directory.
pub fn find_mod_dir(staging_dir: &Path, mod_id: &str) -> Result<PathBuf> {
    if !staging_dir.exists() {
        return Err(SmmError::Io(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!(
                "Staging directory does not exist: {}",
                staging_dir.display()
            ),
        )));
    }

    // Step 1: Direct path match (fast path)
    let direct_path = staging_dir.join(mod_id);
    let direct_has_meta = direct_path.is_dir()
        && (direct_path.join(".smm_mod.json").exists() || direct_path.join("mod.json").exists());
    if direct_has_meta {
        if let Ok(info) = ModLoader::load_mod_info(&direct_path) {
            if info.id == mod_id {
                return Ok(direct_path);
            }
        }
    }

    // Step 2: Full directory scan matching exact ID
    let mut case_insensitive_candidate: Option<PathBuf> = None;
    let entries = std::fs::read_dir(staging_dir)?;

    for entry in entries.filter_map(|e| e.ok()) {
        let path = entry.path();
        let path_has_meta = path.is_dir()
            && (path.join(".smm_mod.json").exists() || path.join("mod.json").exists());
        if path_has_meta {
            if let Ok(info) = ModLoader::load_mod_info(&path) {
                if info.id == mod_id {
                    return Ok(path);
                }
                if info.id.eq_ignore_ascii_case(mod_id) && case_insensitive_candidate.is_none() {
                    case_insensitive_candidate = Some(path);
                }
            }
        }
    }

    // Step 3: Case-insensitive fallback
    if let Some(candidate) = case_insensitive_candidate {
        return Ok(candidate);
    }

    Err(SmmError::ModNotFound(mod_id.to_string()))
}

/// Updates the enabled/disabled state of a mod and persists it to `mod.json`.
pub fn set_mod_enabled(staging_dir: &Path, mod_id: &str, enabled: bool) -> Result<ModInfo> {
    let mod_dir = find_mod_dir(staging_dir, mod_id)?;
    let mut info = ModLoader::load_mod_info(&mod_dir)?;
    info.enabled = enabled;
    save_mod_info(&mod_dir, &info)?;
    info.root_path = Some(mod_dir);
    Ok(info)
}

/// Updates the deployment priority number of a mod and persists it to `mod.json`.
pub fn set_mod_priority(staging_dir: &Path, mod_id: &str, priority: u32) -> Result<ModInfo> {
    let mod_dir = find_mod_dir(staging_dir, mod_id)?;
    let mut info = ModLoader::load_mod_info(&mod_dir)?;
    info.priority = priority;
    save_mod_info(&mod_dir, &info)?;
    info.root_path = Some(mod_dir);
    Ok(info)
}

/// Permanently removes a mod directory from the staging area.
pub fn delete_mod(staging_dir: &Path, mod_id: &str) -> Result<()> {
    let mod_dir = find_mod_dir(staging_dir, mod_id)?;

    // Strict security check: ensure mod_dir is strictly inside staging_dir
    let canonical_staging = staging_dir.canonicalize()?;
    let canonical_mod = mod_dir.canonicalize()?;

    if !canonical_mod.starts_with(&canonical_staging) || canonical_mod == canonical_staging {
        return Err(SmmError::Io(std::io::Error::new(
            std::io::ErrorKind::PermissionDenied,
            format!(
                "Refusing to delete path outside staging directory: {}",
                mod_dir.display()
            ),
        )));
    }

    std::fs::remove_dir_all(&mod_dir)?;
    Ok(())
}

/// Updates mod metadata (source_url, homepage, author, version, etc.) and persists it to `.smm_mod.json`.
pub fn update_mod_info(staging_dir: &Path, mod_id: &str, updated: &ModInfo) -> Result<ModInfo> {
    let mod_dir = find_mod_dir(staging_dir, mod_id)?;
    let mut current = ModLoader::load_mod_info(&mod_dir)?;

    current.name = updated.name.clone();
    current.version = updated.version.clone();
    current.author = updated.author.clone();
    current.category = updated.category.clone();
    current.description = updated.description.clone();
    current.homepage = updated.homepage.clone();
    current.source_url = updated.source_url.clone();
    current.license = updated.license.clone();
    current.enabled = updated.enabled;
    current.priority = updated.priority;
    current.tags = updated.tags.clone();
    current.root_path = Some(mod_dir.clone());

    save_mod_info(&mod_dir, &current)?;
    Ok(current)
}

/// Serializes and writes `ModInfo` to `.smm_mod.json` and `mod.json` inside the specified directory.
pub fn save_mod_info(mod_dir: &Path, info: &ModInfo) -> Result<()> {
    let mut to_save = info.clone();
    to_save.root_path = None; // Don't persist physical machine path into portable mod metadata

    let json_str = serde_json::to_string_pretty(&to_save)?;

    // Write both .smm_mod.json and mod.json for full ecosystem compatibility
    for filename in &[".smm_mod.json", "mod.json"] {
        let meta_path = mod_dir.join(filename);
        let tmp_path = mod_dir.join(format!(".{}.tmp", filename));
        let written = (|| -> std::io::Result<()> {
            std::fs::write(&tmp_path, json_str.as_bytes())?;
            let _ = std::fs::remove_file(&meta_path);
            std::fs::rename(&tmp_path, &meta_path)?;
            Ok(())
        })();

        if written.is_err() {
            let _ = std::fs::remove_file(&tmp_path);
            std::fs::write(&meta_path, json_str.as_bytes())?;
        }
    }

    Ok(())
}

/// Retrieves detailed metadata and normalized asset entries for a specific mod.
pub fn get_mod_details(staging_dir: &Path, mod_id: &str) -> Result<(ModInfo, Vec<AssetEntry>)> {
    let mod_dir = find_mod_dir(staging_dir, mod_id)?;
    ModLoader::scan_mod(&mod_dir)
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    fn setup_test_mod(staging: &Path, id: &str, enabled: bool, priority: u32) -> PathBuf {
        let mod_dir = staging.join(id);
        std::fs::create_dir_all(&mod_dir).unwrap();

        let info = ModInfo {
            id: id.to_string(),
            name: format!("Test Mod {id}"),
            version: "1.0.0".to_string(),
            author: "Author".to_string(),
            category: "weapon_skin".to_string(),
            description: Some("Test mod description".to_string()),
            homepage: None,
            source_url: None,
            license: Some("MIT".to_string()),
            enabled,
            priority,
            tags: vec!["test".to_string()],
            root_path: None,
        };

        save_mod_info(&mod_dir, &info).unwrap();
        mod_dir
    }

    #[test]
    fn test_find_mod_dir() {
        let tmp = tempdir().unwrap();
        let staging = tmp.path();

        setup_test_mod(staging, "my-mod-1", true, 50);

        let found = find_mod_dir(staging, "my-mod-1").unwrap();
        assert_eq!(found, staging.join("my-mod-1"));

        // Case-insensitive fallback
        let found_case = find_mod_dir(staging, "MY-MOD-1").unwrap();
        assert_eq!(found_case, staging.join("my-mod-1"));

        // Non-existent mod
        let err = find_mod_dir(staging, "non-existent").unwrap_err();
        assert!(matches!(err, SmmError::ModNotFound(_)));
    }

    #[test]
    fn test_set_mod_enabled_and_priority() {
        let tmp = tempdir().unwrap();
        let staging = tmp.path();

        setup_test_mod(staging, "state-mod", true, 50);

        // Toggle disabled
        let updated = set_mod_enabled(staging, "state-mod", false).unwrap();
        assert!(!updated.enabled);

        // Reload from disk to verify persistence
        let reloaded = ModLoader::load_mod_info(&staging.join("state-mod")).unwrap();
        assert!(!reloaded.enabled);

        // Change priority
        let reprioritized = set_mod_priority(staging, "state-mod", 25).unwrap();
        assert_eq!(reprioritized.priority, 25);

        let reloaded2 = ModLoader::load_mod_info(&staging.join("state-mod")).unwrap();
        assert_eq!(reloaded2.priority, 25);
    }

    #[test]
    fn test_delete_mod_safely() {
        let tmp = tempdir().unwrap();
        let staging = tmp.path();

        let mod_dir = setup_test_mod(staging, "delete-me", true, 50);
        assert!(mod_dir.exists());

        delete_mod(staging, "delete-me").unwrap();
        assert!(!mod_dir.exists());

        // Deleting again should yield ModNotFound
        let err = delete_mod(staging, "delete-me").unwrap_err();
        assert!(matches!(err, SmmError::ModNotFound(_)));
    }

    #[test]
    fn test_update_mod_info() {
        let tmp = tempdir().unwrap();
        let staging = tmp.path();

        setup_test_mod(staging, "update-mod", true, 50);

        let mut patch = ModLoader::load_mod_info(&staging.join("update-mod")).unwrap();
        patch.name = "Updated Mod Name".to_string();
        patch.version = "2.0.1".to_string();
        patch.author = "New Author".to_string();
        patch.homepage = Some("https://example.com/mod".to_string());
        patch.source_url = Some("https://nexusmods.com/sekiro/mods/888".to_string());
        patch.description = Some("Updated description".to_string());
        patch.priority = 10;
        patch.tags = vec!["tag1".to_string(), "tag2".to_string()];

        let updated = update_mod_info(staging, "update-mod", &patch).unwrap();
        assert_eq!(updated.name, "Updated Mod Name");
        assert_eq!(updated.version, "2.0.1");
        assert_eq!(updated.author, "New Author");
        assert_eq!(updated.homepage.as_deref(), Some("https://example.com/mod"));
        assert_eq!(
            updated.source_url.as_deref(),
            Some("https://nexusmods.com/sekiro/mods/888")
        );
        assert_eq!(updated.description.as_deref(), Some("Updated description"));
        assert_eq!(updated.priority, 10);
        assert_eq!(updated.tags, vec!["tag1", "tag2"]);

        // Verify both .smm_mod.json and mod.json were updated on disk
        let smm_json = staging.join("update-mod").join(".smm_mod.json");
        let mod_json = staging.join("update-mod").join("mod.json");
        assert!(smm_json.exists());
        assert!(mod_json.exists());

        let raw_content = std::fs::read_to_string(&smm_json).unwrap();
        let reloaded: ModInfo = serde_json::from_str(&raw_content).unwrap();
        assert_eq!(reloaded.name, "Updated Mod Name");
        assert_eq!(
            reloaded.source_url.as_deref(),
            Some("https://nexusmods.com/sekiro/mods/888")
        );
        assert_eq!(
            reloaded.homepage.as_deref(),
            Some("https://example.com/mod")
        );
    }
}
