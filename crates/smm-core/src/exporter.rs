use std::io::Write;
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::error::{Result, SmmError};
use crate::extractor::extract_zip;
use crate::importer::{collect_documentation_files, import_mod, slugify, ImportOptions};
use crate::loader::ModLoader;
use crate::manager::{find_mod_dir, save_mod_info};
use crate::normalizer::Normalizer;
use crate::types::ModInfo;

fn default_format_version() -> u32 {
    1
}

fn default_true() -> bool {
    true
}

fn default_priority() -> u32 {
    100
}

/// A single mod descriptor inside a modpack manifest.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ModPackItem {
    pub id: String,
    pub name: String,
    pub version: String,
    #[serde(default)]
    pub author: String,
    #[serde(default)]
    pub category: String,
    #[serde(default = "default_priority")]
    pub priority: u32,
    #[serde(default = "default_true")]
    pub enabled: bool,
    #[serde(default)]
    pub source_url: Option<String>,
    #[serde(default)]
    pub homepage: Option<String>,
    #[serde(default)]
    pub description: Option<String>,
}

impl From<&ModInfo> for ModPackItem {
    fn from(info: &ModInfo) -> Self {
        Self {
            id: info.id.clone(),
            name: info.name.clone(),
            version: info.version.clone(),
            author: info.author.clone(),
            category: info.category.clone(),
            priority: info.priority,
            enabled: info.enabled,
            source_url: info.source_url.clone(),
            homepage: info.homepage.clone(),
            description: info.description.clone(),
        }
    }
}

/// Manifest describing a `.smmpack` modpack container.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ModPackManifest {
    #[serde(default = "default_format_version")]
    pub format_version: u32,
    pub name: String,
    pub version: String,
    #[serde(default)]
    pub author: Option<String>,
    #[serde(default)]
    pub description: Option<String>,
    #[serde(default)]
    pub created_at: Option<u64>,
    #[serde(default)]
    pub mods: Vec<ModPackItem>,
}

impl ModPackManifest {
    pub fn new(name: impl Into<String>, version: impl Into<String>) -> Self {
        Self {
            format_version: 1,
            name: name.into(),
            version: version.into(),
            author: None,
            description: None,
            created_at: Some(
                std::time::SystemTime::now()
                    .duration_since(std::time::UNIX_EPOCH)
                    .map(|d| d.as_secs())
                    .unwrap_or(0),
            ),
            mods: Vec::new(),
        }
    }
}

/// Exports a single staged mod to a standalone `.zip` archive.
///
/// Contains:
/// - Normalized game assets
/// - `.smm_mod.json` & `mod.json` metadata
/// - Preserved documentation files (`README.md`, `LICENSE`, etc.)
/// - If `include_source == true` and `.smm_source/` exists: original source packages.
pub fn export_single_mod(
    staging_dir: &Path,
    mod_id: &str,
    output_path: &Path,
    include_source: bool,
) -> Result<PathBuf> {
    let mod_dir = find_mod_dir(staging_dir, mod_id)?;
    let info = ModLoader::load_mod_info(&mod_dir)?;
    let norm = Normalizer::normalize_directory(&mod_dir)?;

    let final_path = if output_path.is_dir() {
        output_path.join(format!("{}.zip", info.id))
    } else if output_path
        .extension()
        .map(|e| e.eq_ignore_ascii_case("zip"))
        .unwrap_or(false)
    {
        output_path.to_path_buf()
    } else {
        let mut p = output_path.to_path_buf();
        p.set_extension("zip");
        p
    };

    if let Some(parent) = final_path.parent() {
        std::fs::create_dir_all(parent)?;
    }

    let temp_parent = final_path.parent().unwrap_or_else(|| Path::new("."));
    let temp_file = tempfile::NamedTempFile::new_in(temp_parent)?;
    {
        let file = std::fs::File::create(temp_file.path())?;
        let mut zip = zip::ZipWriter::new(file);
        let zip_opts = zip::write::SimpleFileOptions::default()
            .compression_method(zip::CompressionMethod::Deflated);

        // 1. Write metadata: both .smm_mod.json and mod.json
        let mut clean_info = info.clone();
        clean_info.root_path = None;
        let meta_json = serde_json::to_string_pretty(&clean_info)?;

        zip.start_file(".smm_mod.json", zip_opts)?;
        zip.write_all(meta_json.as_bytes())?;

        zip.start_file("mod.json", zip_opts)?;
        zip.write_all(meta_json.as_bytes())?;

        // 2. Write normalized game assets
        for asset in &norm.assets {
            let zip_rel = asset.relative_path.replace('\\', "/");
            zip.start_file(&zip_rel, zip_opts)?;
            let mut asset_file = std::fs::File::open(&asset.source_path)?;
            std::io::copy(&mut asset_file, &mut zip)?;
        }

        // 3. Write documentation files
        let docs = collect_documentation_files(&mod_dir);
        for (doc_name, doc_path) in docs {
            if doc_name != "mod.json" && doc_name != ".smm_mod.json" {
                zip.start_file(&doc_name, zip_opts)?;
                let mut doc_file = std::fs::File::open(&doc_path)?;
                std::io::copy(&mut doc_file, &mut zip)?;
            }
        }

        // 4. Write source archives if requested
        if include_source {
            let source_dir = mod_dir.join(".smm_source");
            if source_dir.is_dir() {
                for entry in walkdir::WalkDir::new(&source_dir).into_iter().filter_map(|e| e.ok()) {
                    if entry.file_type().is_file() {
                        let rel = entry.path().strip_prefix(&source_dir).unwrap_or(entry.path());
                        let rel_str = rel.to_string_lossy().replace('\\', "/");
                        let zip_path = format!(".smm_source/{}", rel_str);
                        zip.start_file(zip_path, zip_opts)?;
                        let mut sf = std::fs::File::open(entry.path())?;
                        std::io::copy(&mut sf, &mut zip)?;
                    }
                }
            }
        }

        zip.finish()?;
    }

    let _ = std::fs::remove_file(&final_path);
    temp_file.persist(&final_path).map_err(|e| e.error)?;
    Ok(final_path)
}

/// Exports multiple staged mods into a packaged `.smmpack` container (standard ZIP format).
///
/// Contains:
/// - Root `smm_pack.json` manifest
/// - Subfolders containing each selected mod
/// - If `include_source == true`, `.smm_source/` packages inside each mod folder
pub fn export_modpack(
    staging_dir: &Path,
    mod_ids: &[String],
    mut manifest: ModPackManifest,
    output_path: &Path,
    include_source: bool,
) -> Result<PathBuf> {
    if mod_ids.is_empty() {
        return Err(SmmError::ExportError(
            "Cannot export empty modpack: no mods selected".to_string(),
        ));
    }

    // Collect all mod data
    let mut mods_data = Vec::new();
    for id in mod_ids {
        let mod_dir = find_mod_dir(staging_dir, id)?;
        let info = ModLoader::load_mod_info(&mod_dir)?;
        let norm = Normalizer::normalize_directory(&mod_dir)?;
        mods_data.push((mod_dir, info, norm));
    }

    // Synchronize manifest items
    let mut updated_items = Vec::new();
    for (_, info, _) in &mods_data {
        if let Some(existing_item) = manifest.mods.iter().find(|m| m.id == info.id) {
            let mut item = existing_item.clone();
            if item.source_url.is_none() {
                item.source_url = info.source_url.clone();
            }
            updated_items.push(item);
        } else {
            updated_items.push(ModPackItem::from(info));
        }
    }
    manifest.mods = updated_items;

    if manifest.created_at.is_none() {
        manifest.created_at = Some(
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .map(|d| d.as_secs())
                .unwrap_or(0),
        );
    }

    let final_path = if output_path.is_dir() {
        let safe_name = if manifest.name.trim().is_empty() {
            "modpack".to_string()
        } else {
            slugify(&manifest.name)
        };
        output_path.join(format!("{}.smmpack", safe_name))
    } else if output_path
        .extension()
        .map(|e| e.eq_ignore_ascii_case("smmpack"))
        .unwrap_or(false)
    {
        output_path.to_path_buf()
    } else {
        let mut p = output_path.to_path_buf();
        p.set_extension("smmpack");
        p
    };

    if let Some(parent) = final_path.parent() {
        std::fs::create_dir_all(parent)?;
    }

    let temp_parent = final_path.parent().unwrap_or_else(|| Path::new("."));
    let temp_file = tempfile::NamedTempFile::new_in(temp_parent)?;
    {
        let file = std::fs::File::create(temp_file.path())?;
        let mut zip = zip::ZipWriter::new(file);
        let zip_opts = zip::write::SimpleFileOptions::default()
            .compression_method(zip::CompressionMethod::Deflated);

        // 1. Root manifest smm_pack.json
        let manifest_json = serde_json::to_string_pretty(&manifest)?;
        zip.start_file("smm_pack.json", zip_opts)?;
        zip.write_all(manifest_json.as_bytes())?;

        // 2. Add each mod into subdirectory <mod_id>/
        for (mod_dir, info, norm) in &mods_data {
            let mod_id = &info.id;

            // Metadata inside mod subfolder
            let mut clean_info = info.clone();
            clean_info.root_path = None;
            let meta_json = serde_json::to_string_pretty(&clean_info)?;

            let meta_subpath = format!("{}/.smm_mod.json", mod_id);
            zip.start_file(&meta_subpath, zip_opts)?;
            zip.write_all(meta_json.as_bytes())?;

            let meta_legacy = format!("{}/mod.json", mod_id);
            zip.start_file(&meta_legacy, zip_opts)?;
            zip.write_all(meta_json.as_bytes())?;

            // Normalized game assets inside mod subfolder
            for asset in &norm.assets {
                let asset_rel = asset.relative_path.replace('\\', "/");
                let entry_path = format!("{}/{}", mod_id, asset_rel);
                zip.start_file(&entry_path, zip_opts)?;
                let mut f = std::fs::File::open(&asset.source_path)?;
                std::io::copy(&mut f, &mut zip)?;
            }

            // Documentation files inside mod subfolder
            let docs = collect_documentation_files(mod_dir);
            for (doc_name, doc_path) in docs {
                if doc_name != "mod.json" && doc_name != ".smm_mod.json" {
                    let entry_path = format!("{}/{}", mod_id, doc_name);
                    zip.start_file(&entry_path, zip_opts)?;
                    let mut f = std::fs::File::open(&doc_path)?;
                    std::io::copy(&mut f, &mut zip)?;
                }
            }

            // Source archives inside mod subfolder if requested
            if include_source {
                let source_dir = mod_dir.join(".smm_source");
                if source_dir.is_dir() {
                    for entry in walkdir::WalkDir::new(&source_dir).into_iter().filter_map(|e| e.ok()) {
                        if entry.file_type().is_file() {
                            let rel = entry.path().strip_prefix(&source_dir).unwrap_or(entry.path());
                            let rel_str = rel.to_string_lossy().replace('\\', "/");
                            let entry_path = format!("{}/.smm_source/{}", mod_id, rel_str);
                            zip.start_file(&entry_path, zip_opts)?;
                            let mut f = std::fs::File::open(entry.path())?;
                            std::io::copy(&mut f, &mut zip)?;
                        }
                    }
                }
            }
        }

        zip.finish()?;
    }

    let _ = std::fs::remove_file(&final_path);
    temp_file.persist(&final_path).map_err(|e| e.error)?;
    Ok(final_path)
}

/// Imports an `.smmpack` modpack archive into the staging area.
///
/// Workflow:
/// 1. Unpacks `.smmpack` into a temporary workspace.
/// 2. Verifies and parses `smm_pack.json`.
/// 3. Ingests each contained mod into `staging/<mod_id>/`, preserving priority, enabled state, and source_url.
pub fn import_modpack(
    pack_path: &Path,
    staging_dir: &Path,
    overwrite: bool,
) -> Result<ModPackManifest> {
    if !pack_path.exists() {
        return Err(SmmError::Io(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!("Modpack file does not exist: {}", pack_path.display()),
        )));
    }

    std::fs::create_dir_all(staging_dir)?;

    let temp_workspace = tempfile::Builder::new()
        .prefix(".smm_modpack_")
        .tempdir_in(staging_dir)
        .or_else(|_| tempfile::tempdir())?;
    let unpack_dir = temp_workspace.path().join("unpacked");
    std::fs::create_dir_all(&unpack_dir)?;

    // Safe decompression of the .smmpack container (standard ZIP format)
    extract_zip(pack_path, &unpack_dir)?;

    // Locate smm_pack.json
    let manifest_path = if unpack_dir.join("smm_pack.json").is_file() {
        unpack_dir.join("smm_pack.json")
    } else {
        find_file_recursive(&unpack_dir, "smm_pack.json").ok_or_else(|| {
            SmmError::InvalidMetadata {
                path: pack_path.to_path_buf(),
                message: "Missing 'smm_pack.json' in modpack archive".to_string(),
            }
        })?
    };

    let manifest_content = std::fs::read_to_string(&manifest_path)?;
    let manifest: ModPackManifest = serde_json::from_str(&manifest_content).map_err(|e| {
        SmmError::InvalidMetadata {
            path: manifest_path.clone(),
            message: format!("Invalid smm_pack.json: {}", e),
        }
    })?;

    if manifest.mods.is_empty() {
        return Err(SmmError::InvalidMetadata {
            path: manifest_path,
            message: "Modpack contains no mods in manifest".to_string(),
        });
    }

    // Pre-check duplicate mods if overwrite is disabled
    if !overwrite {
        for item in &manifest.mods {
            let target_dir = staging_dir.join(&item.id);
            if target_dir.exists() {
                return Err(SmmError::ModAlreadyExists(item.id.clone()));
            }
        }
    }

    // Ingest each mod into staging
    let pack_root = manifest_path.parent().unwrap_or(&unpack_dir);
    for item in &manifest.mods {
        let mod_folder = locate_mod_folder(pack_root, &item.id)?;

        let opts = ImportOptions {
            custom_id: Some(item.id.clone()),
            custom_name: Some(item.name.clone()),
            priority: Some(item.priority),
            overwrite: true,
            source_url: item.source_url.clone(),
        };

        let mut info = import_mod(&mod_folder, staging_dir, &opts)?;

        // Ensure enabled state, priority, and source_url strictly match manifest
        let final_dir = staging_dir.join(&item.id);
        info.enabled = item.enabled;
        info.priority = item.priority;
        if item.source_url.is_some() {
            info.source_url = item.source_url.clone();
        }
        info.root_path = Some(final_dir.clone());
        save_mod_info(&final_dir, &info)?;
    }

    Ok(manifest)
}

/// Locates the directory containing mod assets inside the unpacked modpack archive.
fn locate_mod_folder(base: &Path, mod_id: &str) -> Result<PathBuf> {
    // 1. Direct subfolder
    let candidate = base.join(mod_id);
    if candidate.is_dir() {
        return Ok(candidate);
    }

    // 2. Subfolder nested under mods/
    let mods_candidate = base.join("mods").join(mod_id);
    if mods_candidate.is_dir() {
        return Ok(mods_candidate);
    }

    // 3. Recursive directory scan checking metadata ID
    for entry in walkdir::WalkDir::new(base)
        .min_depth(1)
        .max_depth(4)
        .into_iter()
        .filter_map(|e| e.ok())
    {
        if entry.file_type().is_dir() {
            let p = entry.path();
            if p.join(".smm_mod.json").exists() || p.join("mod.json").exists() {
                if let Ok(info) = ModLoader::load_mod_info(p) {
                    if info.id == mod_id || info.id.eq_ignore_ascii_case(mod_id) {
                        return Ok(p.to_path_buf());
                    }
                }
            }
        }
    }

    Err(SmmError::ModNotFound(format!(
        "Mod '{}' not found inside modpack",
        mod_id
    )))
}

/// Helper to recursively find a specific file by name.
fn find_file_recursive(base: &Path, file_name: &str) -> Option<PathBuf> {
    for entry in walkdir::WalkDir::new(base).into_iter().filter_map(|e| e.ok()) {
        if entry.file_type().is_file() {
            if entry.file_name().to_string_lossy().eq_ignore_ascii_case(file_name) {
                return Some(entry.path().to_path_buf());
            }
        }
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    fn setup_mock_mod(staging: &Path, id: &str, name: &str, priority: u32, enabled: bool, source_url: Option<&str>) {
        let mod_dir = staging.join(id);
        let parts_dir = mod_dir.join("parts");
        std::fs::create_dir_all(&parts_dir).unwrap();
        std::fs::write(parts_dir.join(format!("{id}.partsbnd.dcx")), b"asset payload").unwrap();

        // Documentation
        std::fs::write(mod_dir.join("README.md"), format!("# {name}\nDocumentation")).unwrap();
        std::fs::write(mod_dir.join("LICENSE"), "MIT License").unwrap();

        // Source archive backup
        let source_dir = mod_dir.join(".smm_source");
        std::fs::create_dir_all(&source_dir).unwrap();
        std::fs::write(source_dir.join(format!("{id}_original.zip")), b"mock original zip").unwrap();

        let info = ModInfo {
            id: id.to_string(),
            name: name.to_string(),
            version: "1.2.0".to_string(),
            author: "Tester".to_string(),
            category: "weapon_skin".to_string(),
            description: Some("Test mod description".to_string()),
            homepage: Some("https://example.com/mod".to_string()),
            source_url: source_url.map(|s| s.to_string()),
            license: Some("MIT".to_string()),
            enabled,
            priority,
            tags: vec!["test".to_string()],
            root_path: Some(mod_dir.clone()),
        };

        save_mod_info(&mod_dir, &info).unwrap();
    }

    #[test]
    fn test_export_single_mod_with_and_without_source() {
        let tmp = tempdir().unwrap();
        let staging = tmp.path().join("staging");
        let exports = tmp.path().join("exports");
        std::fs::create_dir_all(&staging).unwrap();
        std::fs::create_dir_all(&exports).unwrap();

        setup_mock_mod(
            &staging,
            "katana-test",
            "Katana Test",
            10,
            true,
            Some("https://nexusmods.com/sekiro/mods/999"),
        );

        // 1. Export without source
        let out_no_source = exports.join("katana_no_source.zip");
        let exported_path = export_single_mod(&staging, "katana-test", &out_no_source, false).unwrap();
        assert_eq!(exported_path, out_no_source);

        let inspect_dir = tmp.path().join("inspect_no_source");
        extract_zip(&out_no_source, &inspect_dir).unwrap();
        assert!(inspect_dir.join("parts/katana-test.partsbnd.dcx").exists());
        assert!(inspect_dir.join(".smm_mod.json").exists());
        assert!(inspect_dir.join("mod.json").exists());
        assert!(inspect_dir.join("README.md").exists());
        assert!(!inspect_dir.join(".smm_source").exists());

        // 2. Export with source
        let out_with_source = exports.join("katana_with_source.zip");
        let exported_with_source = export_single_mod(&staging, "katana-test", &out_with_source, true).unwrap();
        assert_eq!(exported_with_source, out_with_source);

        let inspect_source_dir = tmp.path().join("inspect_with_source");
        extract_zip(&out_with_source, &inspect_source_dir).unwrap();
        assert!(inspect_source_dir.join("parts/katana-test.partsbnd.dcx").exists());
        assert!(inspect_source_dir.join(".smm_mod.json").exists());
        assert!(inspect_source_dir.join(".smm_source/katana-test_original.zip").exists());
    }

    #[test]
    fn test_export_and_import_modpack_roundtrip() {
        let tmp = tempdir().unwrap();
        let staging_src = tmp.path().join("staging_src");
        let staging_dst = tmp.path().join("staging_dst");
        let pack_output = tmp.path().join("packs").join("ultimate_pack.smmpack");

        setup_mock_mod(
            &staging_src,
            "mod-alpha",
            "Mod Alpha",
            5,
            true,
            Some("https://nexusmods.com/sekiro/mods/1"),
        );
        setup_mock_mod(
            &staging_src,
            "mod-beta",
            "Mod Beta",
            25,
            false,
            Some("https://nexusmods.com/sekiro/mods/2"),
        );

        let mut manifest = ModPackManifest::new("Ultimate Sekiro Pack", "1.0.0");
        manifest.author = Some("Pack Creator".to_string());
        manifest.description = Some("A curated collection of quality mods".to_string());

        let mod_ids = vec!["mod-alpha".to_string(), "mod-beta".to_string()];
        let pack_path = export_modpack(&staging_src, &mod_ids, manifest, &pack_output, true).unwrap();
        assert!(pack_path.exists());

        // Import into clean staging_dst
        let imported_manifest = import_modpack(&pack_path, &staging_dst, false).unwrap();
        assert_eq!(imported_manifest.name, "Ultimate Sekiro Pack");
        assert_eq!(imported_manifest.version, "1.0.0");
        assert_eq!(imported_manifest.mods.len(), 2);

        // Verify mod-alpha
        let alpha_dir = staging_dst.join("mod-alpha");
        assert!(alpha_dir.exists());
        let (alpha_info, alpha_assets) = ModLoader::scan_mod(&alpha_dir).unwrap();
        assert_eq!(alpha_info.id, "mod-alpha");
        assert_eq!(alpha_info.priority, 5);
        assert!(alpha_info.enabled);
        assert_eq!(alpha_info.source_url.as_deref(), Some("https://nexusmods.com/sekiro/mods/1"));
        assert_eq!(alpha_assets.len(), 1);
        assert!(alpha_dir.join(".smm_source/mod-alpha_original.zip").exists());

        // Verify mod-beta
        let beta_dir = staging_dst.join("mod-beta");
        assert!(beta_dir.exists());
        let (beta_info, beta_assets) = ModLoader::scan_mod(&beta_dir).unwrap();
        assert_eq!(beta_info.id, "mod-beta");
        assert_eq!(beta_info.priority, 25);
        assert!(!beta_info.enabled); // Preserved disabled state!
        assert_eq!(beta_info.source_url.as_deref(), Some("https://nexusmods.com/sekiro/mods/2"));
        assert_eq!(beta_assets.len(), 1);
        assert!(beta_dir.join(".smm_source/mod-beta_original.zip").exists());

        // Verify duplicate protection without overwrite fails
        let dup_err = import_modpack(&pack_path, &staging_dst, false).unwrap_err();
        assert!(matches!(dup_err, SmmError::ModAlreadyExists(_)));

        // Verify import with overwrite succeeds
        let overwrite_res = import_modpack(&pack_path, &staging_dst, true);
        assert!(overwrite_res.is_ok());
    }

    #[test]
    fn test_export_and_import_error_scenarios() {
        let tmp = tempdir().unwrap();
        let staging = tmp.path().join("staging");
        std::fs::create_dir_all(&staging).unwrap();

        // 1. Export non-existent mod
        let err = export_single_mod(&staging, "does-not-exist", &tmp.path().join("out.zip"), false).unwrap_err();
        assert!(matches!(err, SmmError::ModNotFound(_)));

        // 2. Export empty modpack
        let manifest = ModPackManifest::new("Empty", "1.0");
        let empty_err = export_modpack(&staging, &[], manifest, &tmp.path().join("empty.smmpack"), false).unwrap_err();
        assert!(matches!(empty_err, SmmError::ExportError(_)));

        // 3. Import non-existent modpack
        let not_found_err = import_modpack(&tmp.path().join("non_existent.smmpack"), &staging, false).unwrap_err();
        assert!(matches!(not_found_err, SmmError::Io(_)));

        // 4. Import invalid archive (corrupted file)
        let corrupt_path = tmp.path().join("corrupt.smmpack");
        std::fs::write(&corrupt_path, b"not a zip file").unwrap();
        let corrupt_err = import_modpack(&corrupt_path, &staging, false).unwrap_err();
        assert!(matches!(corrupt_err, SmmError::Zip(_)));
    }
}
