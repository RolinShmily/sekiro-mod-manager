use std::path::Path;

use crate::error::{Result, SmmError};
use crate::normalizer::Normalizer;
use crate::types::{AssetEntry, ModInfo};

/// Utilities for loading and scanning mods from disk.
pub struct ModLoader;

impl ModLoader {
    /// Loads `.smm_mod.json` or `mod.json` from the specified mod directory.
    pub fn load_mod_info(mod_dir: &Path) -> Result<ModInfo> {
        let meta_path = if mod_dir.join(".smm_mod.json").exists() {
            mod_dir.join(".smm_mod.json")
        } else if mod_dir.join("mod.json").exists() {
            mod_dir.join("mod.json")
        } else {
            return Err(SmmError::MetadataNotFound(mod_dir.join(".smm_mod.json")));
        };

        let content = std::fs::read_to_string(&meta_path)?;
        let mut info: ModInfo = serde_json::from_str(&content).map_err(|e| {
            SmmError::InvalidMetadata {
                path: meta_path.clone(),
                message: e.to_string(),
            }
        })?;

        info.root_path = Some(mod_dir.to_path_buf());
        Ok(info)
    }

    /// Fully scans and normalizes a single mod directory.
    pub fn scan_mod(mod_dir: &Path) -> Result<(ModInfo, Vec<AssetEntry>)> {
        let info = Self::load_mod_info(mod_dir)?;
        let norm_result = Normalizer::normalize_directory(mod_dir)?;
        Ok((info, norm_result.assets))
    }

    /// Scans a directory containing multiple mods (e.g. `staging/`).
    pub fn scan_mods_directory(staging_dir: &Path) -> Result<Vec<(ModInfo, Vec<AssetEntry>)>> {
        if !staging_dir.exists() {
            return Err(SmmError::Io(std::io::Error::new(
                std::io::ErrorKind::NotFound,
                format!("Directory does not exist: {}", staging_dir.display()),
            )));
        }

        let mut results = Vec::new();
        let entries = std::fs::read_dir(staging_dir)?;

        for entry in entries.filter_map(|e| e.ok()) {
            let path = entry.path();
            let has_meta = path.is_dir()
                && (path.join(".smm_mod.json").exists() || path.join("mod.json").exists());
            if has_meta {
                match Self::scan_mod(&path) {
                    Ok(mod_tuple) => results.push(mod_tuple),
                    Err(err) => {
                        eprintln!("Warning: Failed to load mod at {}: {}", path.display(), err);
                    }
                }
            }
        }

        // Sort by priority, then by id
        results.sort_by(|a, b| {
            a.0.priority
                .cmp(&b.0.priority)
                .then_with(|| a.0.id.cmp(&b.0.id))
        });

        Ok(results)
    }
}
