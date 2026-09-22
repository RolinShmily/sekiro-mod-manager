use std::path::PathBuf;

use smm_core::{import_mod, import_multiple_files_as_mod, ImportOptions, ModInfo};

/// Imports a mod package (zip, 7z, or folder) into staging.
#[tauri::command]
pub fn import_mod_file(
    source_path: String,
    staging_dir: String,
    source_url: Option<String>,
) -> Result<ModInfo, String> {
    let clean_source = source_path
        .trim()
        .trim_matches('"')
        .trim_matches('\'')
        .to_string();
    let src = PathBuf::from(&clean_source);
    let stg = PathBuf::from(&staging_dir);

    if !src.exists() {
        return Err(format!("Source mod path does not exist: {}", clean_source));
    }

    import_mod(
        &src,
        &stg,
        &ImportOptions {
            custom_id: None,
            custom_name: None,
            priority: None,
            overwrite: true,
            source_url,
        },
    )
    .map_err(|e| format!("Failed to import mod from '{}': {}", clean_source, e))
}

/// Imports multiple files merged as a single mod package.
#[tauri::command]
pub fn import_merged_mod_files(
    source_paths: Vec<String>,
    staging_dir: String,
    custom_name: Option<String>,
    source_url: Option<String>,
) -> Result<ModInfo, String> {
    let paths: Vec<PathBuf> = source_paths
        .into_iter()
        .map(|p| PathBuf::from(p.trim().trim_matches('"').trim_matches('\'')))
        .collect();
    let stg = PathBuf::from(&staging_dir);

    import_multiple_files_as_mod(
        &paths,
        &stg,
        &ImportOptions {
            custom_id: None,
            custom_name,
            priority: None,
            overwrite: true,
            source_url,
        },
    )
    .map_err(|e| format!("Failed to import merged mods: {}", e))
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
///
/// The flat parameter list mirrors Tauri's IPC contract (each name is an `invoke` argument);
/// grouping them into a struct would change the frontend call shape, so the arg-count lint is
/// allowed here on purpose.
#[allow(clippy::too_many_arguments)]
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

    let mut manifest = smm_core::ModPackManifest::new(name, version);
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
) -> Result<smm_core::ModPackManifest, String> {
    let s_path = PathBuf::from(&staging_dir);
    let p_path = PathBuf::from(&pack_path);

    smm_core::import_modpack(&p_path, &s_path, overwrite)
        .map_err(|e| format!("Failed to import modpack from '{}': {}", pack_path, e))
}
