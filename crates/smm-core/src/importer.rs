use std::path::{Path, PathBuf};

use walkdir::WalkDir;

use crate::error::{Result, SmmError};
use crate::extractor::{extract_archive, is_supported_archive};
use crate::loader::ModLoader;
use crate::manager::save_mod_info;
use crate::normalizer::Normalizer;
use crate::types::{AssetCategory, AssetEntry, ModInfo};

/// Configuration options for mod package import.
#[derive(Debug, Clone, Default)]
pub struct ImportOptions {
    /// Custom unique mod ID (slug). If None, inferred from source file/folder name.
    pub custom_id: Option<String>,
    /// Custom display name. If None, derived from source name.
    pub custom_name: Option<String>,
    /// Numerical deployment priority. If None, automatically determined by asset category.
    pub priority: Option<u32>,
    /// Whether to replace an existing mod with the same ID in staging.
    pub overwrite: bool,
    /// Upstream source URL or package download location.
    pub source_url: Option<String>,
}

/// Imports a mod package (directory or `.zip` archive) into the staging area.
///
/// Workflow:
/// 1. Unpacks `.zip` or copies source directory into an isolated temporary workspace.
/// 2. Utilizes `Normalizer` heuristic analysis to discover the canonical Sekiro asset root
///    (stripping arbitrary nested directory wrappers).
/// 3. Locates or generates `mod.json`, inferring metadata and category from assets.
/// 4. Extracts and preserves documentation files (`README.md`, `LICENSE`, etc.).
/// 5. Atomically deploys the normalized mod structure into `staging/<mod_id>/`.
pub fn import_mod(
    source_path: &Path,
    staging_dir: &Path,
    options: &ImportOptions,
) -> Result<ModInfo> {
    if !source_path.exists() {
        return Err(SmmError::Io(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!("Source path does not exist: {}", source_path.display()),
        )));
    }

    // Ensure staging directory exists
    std::fs::create_dir_all(staging_dir)?;

    // Create an isolated temporary workspace inside staging directory to ensure same filesystem volume
    let temp_workspace = tempfile::Builder::new()
        .prefix(".smm_import_")
        .tempdir_in(staging_dir)
        .or_else(|_| tempfile::tempdir())?;

    let unpack_dir = temp_workspace.path().join("unpacked");
    std::fs::create_dir_all(&unpack_dir)?;

    // Step 1: Unpack archive or copy source directory
    if source_path.is_file() {
        if !is_supported_archive(source_path) {
            return Err(SmmError::NormalizationError {
                path: source_path.to_path_buf(),
                message: format!(
                    "Unsupported archive file format: '{}'. Only .zip, .7z, .rar archives or unpacked directories are supported.",
                    source_path.display()
                ),
            });
        }

        extract_archive(source_path, &unpack_dir)?;
    } else if source_path.is_dir() {
        copy_dir_recursive(source_path, &unpack_dir)?;
    } else {
        return Err(SmmError::NormalizationError {
            path: source_path.to_path_buf(),
            message: format!("Invalid source path: {}", source_path.display()),
        });
    }

    // Step 1.5: Smart recursive extraction for nested archives (e.g. multi-component / multi-pack zip archives)
    let mut nested_iteration = 0;
    while nested_iteration < 3 && Normalizer::find_canonical_root(&unpack_dir).is_err() {
        let nested_archives: Vec<PathBuf> = WalkDir::new(&unpack_dir)
            .into_iter()
            .filter_map(|e| e.ok())
            .filter(|e| e.file_type().is_file() && is_supported_archive(e.path()))
            .map(|e| e.into_path())
            .collect();

        if nested_archives.is_empty() {
            break;
        }

        // If there are multiple archives, determine if one is an integration/all-in-one pack
        if let Some(primary_archive) = pick_primary_nested_archive(&nested_archives) {
            let nested_out = temp_workspace
                .path()
                .join(format!("nested_extracted_{}", nested_iteration));
            std::fs::create_dir_all(&nested_out)?;

            // Extract the selected primary archive
            extract_archive(&primary_archive, &nested_out)?;

            // Remove processed nested archives from unpack_dir so they don't loop or clutter
            for arc in &nested_archives {
                let _ = std::fs::remove_file(arc);
            }

            // Copy extracted files into unpack_dir
            copy_dir_recursive(&nested_out, &unpack_dir)?;
        } else {
            // Extract all nested archives into unpack_dir
            for (idx, arc) in nested_archives.iter().enumerate() {
                let nested_out = temp_workspace
                    .path()
                    .join(format!("nested_extracted_{}_{}", nested_iteration, idx));
                std::fs::create_dir_all(&nested_out)?;
                if extract_archive(arc, &nested_out).is_ok() {
                    let _ = copy_dir_recursive(&nested_out, &unpack_dir);
                }
                let _ = std::fs::remove_file(arc);
            }
        }

        nested_iteration += 1;
    }

    // Step 2: Normalize directory and locate canonical Sekiro assets
    let norm_result = Normalizer::normalize_directory(&unpack_dir)?;
    if norm_result.assets.is_empty() {
        return Err(SmmError::NoAssetsFound(source_path.to_path_buf()));
    }

    // Step 3: Determine / Generate Mod Metadata
    let stem = source_path
        .file_stem()
        .and_then(|s| s.to_str())
        .unwrap_or("mod");

    let existing_mod_info =
        find_existing_mod_json(&unpack_dir, &norm_result.canonical_root);

    let mut mod_info = if let Some(mut existing) = existing_mod_info {
        if let Some(id) = &options.custom_id {
            existing.id = id.clone();
        }
        if let Some(name) = &options.custom_name {
            existing.name = name.clone();
        }
        if let Some(pri) = options.priority {
            existing.priority = pri;
        }
        if options.source_url.is_some() {
            existing.source_url = options.source_url.clone();
        }
        existing
    } else {
        // Smart automatic metadata generation
        let id = options
            .custom_id
            .clone()
            .unwrap_or_else(|| slugify(stem));

        let name = options
            .custom_name
            .clone()
            .unwrap_or_else(|| humanize_name(stem));

        let version = extract_version(stem).unwrap_or_else(|| "1.0.0".to_string());
        let category = infer_category(&norm_result.assets);
        let priority = options
            .priority
            .unwrap_or_else(|| default_priority_for_category(&category));

        let description = Some(format!(
            "Imported package from {}",
            source_path
                .file_name()
                .and_then(|n| n.to_str())
                .unwrap_or(stem)
        ));

        let mut info = ModInfo::new(id, name, version, "Unknown", category);
        info.priority = priority;
        info.description = description;
        info.source_url = options.source_url.clone();
        info.enabled = true;
        info
    };

    // Step 4: Verify target existence in staging
    let target_dir = staging_dir.join(&mod_info.id);
    if target_dir.exists() {
        if !options.overwrite {
            return Err(SmmError::ModAlreadyExists(mod_info.id.clone()));
        }
    }

    // Step 5: Build normalized target layout in a clean staging subfolder
    let build_dir = temp_workspace.path().join("normalized_mod");
    std::fs::create_dir_all(&build_dir)?;

    // 5a. Copy normalized game assets
    for asset in &norm_result.assets {
        let dest_path = build_dir.join(&asset.relative_path);
        if let Some(parent) = dest_path.parent() {
            std::fs::create_dir_all(parent)?;
        }
        std::fs::copy(&asset.source_path, &dest_path)?;
    }

    // 5b. Extract and preserve documentation files (README, LICENSE, etc.)
    let docs = collect_documentation_files(&unpack_dir);
    for (filename, src_path) in docs {
        let dest_path = build_dir.join(&filename);
        if !dest_path.exists() {
            let _ = std::fs::copy(&src_path, &dest_path);
        }
    }

    // 5c. Preserve .smm_source if existing
    let build_source_dir = build_dir.join(".smm_source");
    if unpack_dir.join(".smm_source").is_dir() {
        let _ = copy_dir_recursive(&unpack_dir.join(".smm_source"), &build_source_dir);
    } else if source_path.is_dir() && source_path.join(".smm_source").is_dir() {
        let _ = copy_dir_recursive(&source_path.join(".smm_source"), &build_source_dir);
    }

    // 5d. Backup original source package if imported from archive file
    if source_path.is_file() {
        std::fs::create_dir_all(&build_source_dir)?;
        let file_name = source_path
            .file_name()
            .unwrap_or_else(|| std::ffi::OsStr::new("source.archive"));
        let dest_file = build_source_dir.join(file_name);
        std::fs::copy(source_path, &dest_file)?;
    }

    // 5e. Persist normalized mod.json & .smm_mod.json
    save_mod_info(&build_dir, &mod_info)?;

    // Step 6: Atomically move build_dir to target_dir in staging
    if target_dir.exists() {
        std::fs::remove_dir_all(&target_dir)?;
    }

    if std::fs::rename(&build_dir, &target_dir).is_err() {
        // Fallback for cross-volume moves
        copy_dir_recursive(&build_dir, &target_dir)?;
        let _ = std::fs::remove_dir_all(&build_dir);
    }

    mod_info.root_path = Some(target_dir);
    Ok(mod_info)
}

/// Imports multiple files/archives merged together into a single unified mod package.
/// Designed for mods like Nexus #1454 (NPC Cloth Physics) where multiple separate download
/// archives belong to the same mod concept.
pub fn import_multiple_files_as_mod(
    source_paths: &[PathBuf],
    staging_dir: &Path,
    options: &ImportOptions,
) -> Result<ModInfo> {
    if source_paths.is_empty() {
        return Err(SmmError::NormalizationError {
            path: PathBuf::new(),
            message: "No source files provided for merged import".to_string(),
        });
    }

    if source_paths.len() == 1 {
        return import_mod(&source_paths[0], staging_dir, options);
    }

    std::fs::create_dir_all(staging_dir)?;

    let temp_workspace = tempfile::Builder::new()
        .prefix(".smm_merge_import_")
        .tempdir_in(staging_dir)
        .or_else(|_| tempfile::tempdir())?;

    let merged_staging = temp_workspace.path().join("merged");
    std::fs::create_dir_all(&merged_staging)?;

    let source_backup_dir = merged_staging.join(".smm_source");
    std::fs::create_dir_all(&source_backup_dir)?;

    // Extract / copy each file into intermediate folders, normalize, and merge into merged_staging
    for (idx, src) in source_paths.iter().enumerate() {
        if !src.exists() {
            return Err(SmmError::NormalizationError {
                path: src.clone(),
                message: format!("Source path does not exist: {}", src.display()),
            });
        }

        // Backup original source
        if src.is_file() {
            if let Some(name) = src.file_name() {
                let _ = std::fs::copy(src, source_backup_dir.join(name));
            }
        }

        let item_unpack = temp_workspace.path().join(format!("item_{}", idx));
        std::fs::create_dir_all(&item_unpack)?;

        if src.is_file() {
            if !is_supported_archive(src) {
                return Err(SmmError::NormalizationError {
                    path: src.clone(),
                    message: format!("Unsupported archive format: {}", src.display()),
                });
            }
            extract_archive(src, &item_unpack)?;
        } else if src.is_dir() {
            copy_dir_recursive(src, &item_unpack)?;
        }

        // Check for nested archives inside item_unpack
        let nested_archives: Vec<PathBuf> = WalkDir::new(&item_unpack)
            .into_iter()
            .filter_map(|e| e.ok())
            .filter(|e| e.file_type().is_file() && is_supported_archive(e.path()))
            .map(|e| e.into_path())
            .collect();

        for arc in nested_archives {
            let nested_out = temp_workspace.path().join(format!(
                "item_nested_{}_{}",
                idx,
                arc.file_stem().and_then(|s| s.to_str()).unwrap_or("sub")
            ));
            std::fs::create_dir_all(&nested_out)?;
            if extract_archive(&arc, &nested_out).is_ok() {
                let _ = copy_dir_recursive(&nested_out, &item_unpack);
            }
            let _ = std::fs::remove_file(arc);
        }

        // Normalize this item
        let norm = Normalizer::normalize_directory(&item_unpack)?;
        for asset in norm.assets {
            let dest = merged_staging.join(&asset.relative_path);
            if let Some(parent) = dest.parent() {
                std::fs::create_dir_all(parent)?;
            }
            let _ = std::fs::copy(&asset.source_path, &dest);
        }

        // Also copy any documentation files
        let docs = collect_documentation_files(&item_unpack);
        for (filename, src_path) in docs {
            let dest_path = merged_staging.join(&filename);
            if !dest_path.exists() {
                let _ = std::fs::copy(&src_path, &dest_path);
            }
        }
    }

    // Normalize final merged directory
    let norm_result = Normalizer::normalize_directory(&merged_staging)?;
    if norm_result.assets.is_empty() {
        return Err(SmmError::NoAssetsFound(source_paths[0].clone()));
    }

    let default_name = options.custom_name.clone().unwrap_or_else(|| {
        let first_stem = source_paths[0]
            .file_stem()
            .and_then(|s| s.to_str())
            .unwrap_or("merged-mod");
        format!("Merged - {}", humanize_name(first_stem))
    });

    let mod_id = options
        .custom_id
        .clone()
        .unwrap_or_else(|| slugify(&default_name));

    let category = infer_category(&norm_result.assets);
    let priority = options
        .priority
        .unwrap_or_else(|| default_priority_for_category(&category));

    let mut mod_info = ModInfo::new(&mod_id, &default_name, "1.0.0", "Community", &category);
    mod_info.category = category;
    mod_info.priority = priority;
    mod_info.source_url = options.source_url.clone();
    mod_info.description = Some(format!(
        "Merged mod bundle consisting of {} component files.",
        source_paths.len()
    ));

    // Write metadata
    save_mod_info(&merged_staging, &mod_info)?;

    let target_dir = staging_dir.join(&mod_id);
    if target_dir.exists() {
        if options.overwrite {
            let _ = std::fs::remove_dir_all(&target_dir);
        } else {
            return Err(SmmError::NormalizationError {
                path: target_dir,
                message: format!("Mod with ID '{}' already exists in staging", mod_id),
            });
        }
    }

    if std::fs::rename(&merged_staging, &target_dir).is_err() {
        copy_dir_recursive(&merged_staging, &target_dir)?;
        let _ = std::fs::remove_dir_all(&merged_staging);
    }

    mod_info.root_path = Some(target_dir);
    Ok(mod_info)
}

/// Unpacks a zip archive safely into the destination directory.
#[inline]
pub fn extract_zip_archive(zip_path: &Path, extract_to: &Path) -> Result<()> {
    crate::extractor::extract_zip(zip_path, extract_to)
}


/// Recursively copies a directory tree from `src` to `dst`.
fn copy_dir_recursive(src: &Path, dst: &Path) -> Result<()> {
    std::fs::create_dir_all(dst)?;

    for entry in WalkDir::new(src).into_iter().filter_map(|e| e.ok()) {
        let rel_path = match entry.path().strip_prefix(src) {
            Ok(p) => p,
            Err(_) => continue,
        };

        let target_path = dst.join(rel_path);
        if entry.file_type().is_dir() {
            std::fs::create_dir_all(&target_path)?;
        } else if entry.file_type().is_file() {
            if let Some(parent) = target_path.parent() {
                std::fs::create_dir_all(parent)?;
            }
            std::fs::copy(entry.path(), &target_path)?;
        }
    }

    Ok(())
}

/// Discovers an existing `mod.json` file in canonical root, unpack root, or intermediate directories.
fn find_existing_mod_json(unpack_dir: &Path, canonical_root: &Path) -> Option<ModInfo> {
    if let Ok(info) = ModLoader::load_mod_info(canonical_root) {
        return Some(info);
    }

    if let Ok(info) = ModLoader::load_mod_info(unpack_dir) {
        return Some(info);
    }

    let mut curr = canonical_root.parent();
    while let Some(dir) = curr {
        if let Ok(info) = ModLoader::load_mod_info(dir) {
            return Some(info);
        }
        if dir == unpack_dir {
            break;
        }
        curr = dir.parent();
    }

    None
}

/// Collects documentation files (README, LICENSE, etc.) from the unpacked source.
pub fn collect_documentation_files(unpack_dir: &Path) -> Vec<(String, PathBuf)> {
    let mut docs = Vec::new();

    for entry in WalkDir::new(unpack_dir).into_iter().filter_map(|e| e.ok()) {
        if entry.file_type().is_file() {
            // Ignore hidden directories relative to unpack_dir like .smm_source
            if let Ok(rel) = entry.path().strip_prefix(unpack_dir) {
                if rel.components().any(|c| {
                    let s = c.as_os_str().to_string_lossy();
                    s.starts_with('.') && s != "." && s != ".."
                }) {
                    continue;
                }
            }

            let filename = entry.file_name().to_string_lossy();
            let lower = filename.to_ascii_lowercase();

            if lower.starts_with("readme")
                || lower.starts_with("license")
                || lower.starts_with("licence")
                || lower.starts_with("changelog")
                || lower.ends_with(".md")
            {
                docs.push((filename.to_string(), entry.path().to_path_buf()));
            }
        }
    }

    // Sort by depth so shallower/root docs take precedence
    docs.sort_by_key(|(_, path)| path.components().count());

    let mut deduped = Vec::new();
    let mut seen = std::collections::HashSet::new();

    for (name, path) in docs {
        let lower = name.to_ascii_lowercase();
        if seen.insert(lower) {
            deduped.push((name, path));
        }
    }

    deduped
}

/// Converts an arbitrary string into a URL- and filesystem-safe slug ID.
pub fn slugify(input: &str) -> String {
    let mut result = String::new();
    let mut last_was_dash = false;

    for ch in input.chars() {
        if ch.is_ascii_alphanumeric() {
            result.push(ch.to_ascii_lowercase());
            last_was_dash = false;
        } else if !last_was_dash && !result.is_empty() {
            result.push('-');
            last_was_dash = true;
        }
    }

    // Trim trailing dash
    if result.ends_with('-') {
        result.pop();
    }

    if result.is_empty() {
        "imported-mod".to_string()
    } else {
        result
    }
}

/// Strips common archive extensions (.zip, .7z, .rar) from a filename.
pub fn strip_archive_extension(name: &str) -> &str {
    let lower = name.to_ascii_lowercase();
    if lower.ends_with(".zip") || lower.ends_with(".rar") {
        &name[..name.len() - 4]
    } else if lower.ends_with(".7z") {
        &name[..name.len() - 3]
    } else {
        name
    }
}

/// Converts a slug or raw filename into a clean, capitalized display name.
pub fn humanize_name(input: &str) -> String {
    let clean = strip_archive_extension(input);
    let words: Vec<String> = clean
        .split(|c: char| c == '_' || c == '-' || c == '.' || c == ' ')
        .filter(|w| !w.is_empty())
        .map(|w| {
            let mut chars = w.chars();
            match chars.next() {
                None => String::new(),
                Some(first) => first.to_uppercase().collect::<String>() + chars.as_str(),
            }
        })
        .collect();

    if words.is_empty() {
        "Imported Mod".to_string()
    } else {
        words.join(" ")
    }
}

/// Heuristically extracts a semantic or numeric version string from a filename.
pub fn extract_version(text: &str) -> Option<String> {
    let clean_text = strip_archive_extension(text);
    for part in clean_text.split(|c: char| c == '_' || c == '-' || c == ' ') {
        let trimmed = part
            .strip_prefix('v')
            .or_else(|| part.strip_prefix('V'))
            .unwrap_or(part);

        let components: Vec<&str> = trimmed.split('.').collect();
        if components.len() >= 2
            && components
                .iter()
                .all(|c| !c.is_empty() && c.chars().all(|ch| ch.is_ascii_digit()))
        {
            return Some(trimmed.to_string());
        }
    }
    None
}

/// Infers the primary mod category based on normalized asset classifications.
pub fn infer_category(assets: &[AssetEntry]) -> String {
    let mut has_loader = false;
    let mut has_param = false;
    let mut has_weapon = false;
    let mut has_chr = false;
    let mut has_parts = false;
    let mut has_ui = false;
    let mut has_sound = false;
    let mut has_map = false;
    let mut has_script = false;
    let mut has_sfx = false;

    for a in assets {
        match a.category {
            AssetCategory::Loader => has_loader = true,
            AssetCategory::Param => has_param = true,
            AssetCategory::Parts => {
                has_parts = true;
                let rel = a.relative_path.to_ascii_lowercase();
                if rel.contains("wp_") {
                    has_weapon = true;
                }
            }
            AssetCategory::Chr => has_chr = true,
            AssetCategory::Menu | AssetCategory::Font | AssetCategory::Msg => has_ui = true,
            AssetCategory::Sound => has_sound = true,
            AssetCategory::Map => has_map = true,
            AssetCategory::Script => has_script = true,
            AssetCategory::Sfx => has_sfx = true,
            _ => {}
        }
    }

    if has_loader {
        "loader".to_string()
    } else if has_param {
        "gameplay_overhaul".to_string()
    } else if has_sfx {
        "vfx".to_string()
    } else if has_weapon {
        "weapon_skin".to_string()
    } else if has_chr || has_parts {
        "character_skin".to_string()
    } else if has_ui {
        "ui".to_string()
    } else if has_sound {
        "audio".to_string()
    } else if has_map {
        "map".to_string()
    } else if has_script {
        "script".to_string()
    } else {
        "general".to_string()
    }
}

/// Default deployment priority based on mod category.
pub fn default_priority_for_category(category: &str) -> u32 {
    match category {
        "loader" => 10,
        "gameplay_overhaul" => 50,
        "vfx" => 80,
        "weapon_skin" | "character_skin" => 100,
        "ui" => 100,
        "audio" => 100,
        "map" => 100,
        "script" => 100,
        _ => 100,
    }
}

/// Intelligently chooses the primary mod archive when an archive contains multiple nested packages.
/// Returns Some(archive) if an integration/full/main package is identified or if only one exists.
pub fn pick_primary_nested_archive(archives: &[PathBuf]) -> Option<PathBuf> {
    if archives.is_empty() {
        return None;
    }
    if archives.len() == 1 {
        return Some(archives[0].clone());
    }

    let keywords = [
        "integration", "integrate", "aio", "all-in-one", "all in one",
        "full", "complete", "main", "base", "default",
        "整合", "完整", "全套", "主体", "基础", "默认",
    ];

    for keyword in &keywords {
        if let Some(found) = archives.iter().find(|p| {
            let name = p.file_name().unwrap_or_default().to_string_lossy().to_ascii_lowercase();
            name.contains(keyword)
        }) {
            return Some(found.clone());
        }
    }

    // If no keyword matched, choose the largest archive by file size
    archives
        .iter()
        .max_by_key(|p| std::fs::metadata(p).map(|m| m.len()).unwrap_or(0))
        .cloned()
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;
    use tempfile::tempdir;
    use zip::write::SimpleFileOptions;
    use zip::ZipWriter;

    #[test]
    fn test_slugify_and_helpers() {
        assert_eq!(slugify("Kusabimaru_Reaper_v1.2.0"), "kusabimaru-reaper-v1-2-0");
        assert_eq!(slugify("My   Awesome  Mod!!"), "my-awesome-mod");
        assert_eq!(humanize_name("kusabimaru-reaper"), "Kusabimaru Reaper");
        assert_eq!(
            extract_version("Kusabimaru_Reaper_v1.2.0.zip"),
            Some("1.2.0".to_string())
        );
        assert_eq!(extract_version("SomeMod_2.5"), Some("2.5".to_string()));
        assert_eq!(extract_version("no_version_here"), None);
    }

    #[test]
    fn test_import_from_nested_zip_with_heuristic_recovery_and_doc_extraction() {
        let tmp = tempdir().unwrap();
        let zip_path = tmp.path().join("MessyMod_v2.5.zip");
        let staging_dir = tmp.path().join("staging");

        // 1. Pack a messy, unnormalized ZIP file dynamically in memory/tempfile
        let file = std::fs::File::create(&zip_path).unwrap();
        let mut zip = ZipWriter::new(file);
        let options = SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);

        // Nested canonical Sekiro assets
        zip.start_file("MessyMod_v2.5/Sekiro_Patch/mods/parts/wp_a_0300.partsbnd.dcx", options)
            .unwrap();
        zip.write_all(b"mock katana parts data").unwrap();

        zip.start_file("MessyMod_v2.5/Sekiro_Patch/mods/parts/bd_m_9000.partsbnd.dcx", options)
            .unwrap();
        zip.write_all(b"mock body parts data").unwrap();

        // Extra documentation files at root and nested
        zip.start_file("MessyMod_v2.5/README.md", options).unwrap();
        zip.write_all(b"# Messy Mod\nComprehensive weapon overhaul.").unwrap();

        zip.start_file("MessyMod_v2.5/LICENSE", options).unwrap();
        zip.write_all(b"MIT License").unwrap();

        zip.start_file("MessyMod_v2.5/HowToInstall.txt", options).unwrap();
        zip.write_all(b"Copy into mods directory.").unwrap();

        zip.finish().unwrap();

        // 2. Perform Mod Import
        let imported = import_mod(&zip_path, &staging_dir, &ImportOptions::default()).unwrap();

        // 3. Verify automatic metadata generation
        assert_eq!(imported.id, "messymod-v2-5");
        assert_eq!(imported.name, "MessyMod V2 5");
        assert_eq!(imported.version, "2.5");
        assert_eq!(imported.category, "weapon_skin");
        assert_eq!(imported.priority, 100);
        assert!(imported.enabled);

        let target_dir = staging_dir.join("messymod-v2-5");
        assert!(target_dir.exists());
        assert!(target_dir.join("mod.json").exists());

        // Verify normalized game asset placement
        assert!(target_dir.join("parts/wp_a_0300.partsbnd.dcx").exists());
        assert!(target_dir.join("parts/bd_m_9000.partsbnd.dcx").exists());

        // Verify documentation files preserved
        assert!(target_dir.join("README.md").exists());
        assert!(target_dir.join("LICENSE").exists());

        // 4. Verify ModLoader can seamlessly scan the imported mod
        let (info, assets) = ModLoader::scan_mod(&target_dir).unwrap();
        assert_eq!(info.id, "messymod-v2-5");
        assert_eq!(assets.len(), 2);
        assert_eq!(assets[0].relative_path, "parts/bd_m_9000.partsbnd.dcx");
        assert_eq!(assets[1].relative_path, "parts/wp_a_0300.partsbnd.dcx");
    }

    #[test]
    fn test_import_duplicate_protection_and_overwrite() {
        let tmp = tempdir().unwrap();
        let staging_dir = tmp.path().join("staging");

        // Create a simple directory mod
        let mod_src = tmp.path().join("SampleSkin_v1.0");
        let parts_dir = mod_src.join("parts");
        std::fs::create_dir_all(&parts_dir).unwrap();
        std::fs::write(parts_dir.join("wp_a_0300.partsbnd.dcx"), b"katana data").unwrap();

        let opts = ImportOptions {
            custom_id: Some("custom-skin".to_string()),
            custom_name: Some("Custom Katana Skin".to_string()),
            priority: Some(20),
            overwrite: false,
            source_url: None,
        };

        // First import succeeds
        let info = import_mod(&mod_src, &staging_dir, &opts).unwrap();
        assert_eq!(info.id, "custom-skin");
        assert_eq!(info.priority, 20);

        // Second import without overwrite fails with ModAlreadyExists
        let err = import_mod(&mod_src, &staging_dir, &opts).unwrap_err();
        assert!(matches!(err, SmmError::ModAlreadyExists(_)));

        // Third import with overwrite succeeds
        let mut overwrite_opts = opts.clone();
        overwrite_opts.overwrite = true;
        overwrite_opts.priority = Some(15);
        let updated = import_mod(&mod_src, &staging_dir, &overwrite_opts).unwrap();
        assert_eq!(updated.priority, 15);
    }

    #[test]
    fn test_import_archive_preserves_source_backup_and_source_url() {
        let tmp = tempdir().unwrap();
        let staging_dir = tmp.path().join("staging");
        let zip_path = tmp.path().join("MyCoolWeapon_v1.0.zip");

        // Create zip archive
        {
            let file = std::fs::File::create(&zip_path).unwrap();
            let mut zip = ZipWriter::new(file);
            let options = SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);
            zip.start_file("parts/wp_a_0300.partsbnd.dcx", options).unwrap();
            zip.write_all(b"katana raw bytes").unwrap();
            zip.finish().unwrap();
        }

        let opts = ImportOptions {
            custom_id: Some("cool-weapon".to_string()),
            custom_name: Some("Cool Katana Weapon".to_string()),
            priority: Some(10),
            overwrite: false,
            source_url: Some("https://www.nexusmods.com/sekiro/mods/555".to_string()),
        };

        let imported = import_mod(&zip_path, &staging_dir, &opts).unwrap();
        assert_eq!(imported.id, "cool-weapon");
        assert_eq!(
            imported.source_url.as_deref(),
            Some("https://www.nexusmods.com/sekiro/mods/555")
        );

        let staged_mod_dir = staging_dir.join("cool-weapon");
        assert!(staged_mod_dir.exists());

        // Verify .smm_source/<filename> contains the original archive
        let backup_file = staged_mod_dir.join(".smm_source/MyCoolWeapon_v1.0.zip");
        assert!(backup_file.exists());
        assert_eq!(
            std::fs::read(&backup_file).unwrap(),
            std::fs::read(&zip_path).unwrap()
        );

        // Verify .smm_mod.json and mod.json contain source_url
        let smm_json = staged_mod_dir.join(".smm_mod.json");
        assert!(smm_json.exists());
        let info: ModInfo = serde_json::from_str(&std::fs::read_to_string(&smm_json).unwrap()).unwrap();
        assert_eq!(info.source_url.as_deref(), Some("https://www.nexusmods.com/sekiro/mods/555"));

        // Verify scan_mod does NOT treat .smm_source/ as a game asset
        let (scanned_info, scanned_assets) = ModLoader::scan_mod(&staged_mod_dir).unwrap();
        assert_eq!(scanned_info.id, "cool-weapon");
        assert_eq!(scanned_assets.len(), 1);
        assert_eq!(scanned_assets[0].relative_path, "parts/wp_a_0300.partsbnd.dcx");
    }
}
