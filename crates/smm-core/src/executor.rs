use std::collections::HashSet;
use std::fs;
use std::path::{Path, PathBuf};
use std::time::{Instant, SystemTime, UNIX_EPOCH};

use serde::{Deserialize, Serialize};

use crate::error::{Result, SmmError};
use crate::types::DeployPlan;

/// Persistent deployment manifest stored in the target game mods directory as `.smm_manifest.json`.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DeployManifest {
    pub version: u32,
    pub profile: String,
    pub deployed_at: u64,
    pub files: Vec<String>,
    pub total_bytes: u64,
}

/// Structured report returned by `execute_deploy`.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DeployResult {
    /// Target directory where mods were deployed
    pub target_dir: PathBuf,
    /// Total number of mapped asset files in the plan
    pub total_files: usize,
    /// Number of files successfully hard-linked
    pub hard_links_created: usize,
    /// Number of files copied (e.g. cross-volume fallback)
    pub copied_files: usize,
    /// Number of files skipped
    pub skipped_files: usize,
    /// Number of failed files
    pub failed_files: usize,
    /// Total disk bytes saved by using hard links instead of physical copies
    pub bytes_saved: u64,
    /// Elapsed execution time in milliseconds
    pub duration_ms: u128,
    /// Warnings generated during execution
    pub warnings: Vec<String>,
    /// Errors encountered for specific files: (target_relative_path, error_message)
    pub errors: Vec<(String, String)>,
}

impl DeployResult {
    pub fn is_success(&self) -> bool {
        self.failed_files == 0
    }
}

/// Structured report returned by `restore_deploy`.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RestoreResult {
    /// Target directory that was cleaned
    pub target_dir: PathBuf,
    /// Number of deployed files removed
    pub removed_files: usize,
    /// Number of empty subdirectories removed
    pub removed_dirs: usize,
    /// Elapsed execution time in milliseconds
    pub duration_ms: u128,
    /// Whether restoration succeeded cleanly
    pub success: bool,
    /// Warnings or diagnostic notes
    pub warnings: Vec<String>,
}

/// Validates that the target directory is safe to operate on (rejecting empty or root paths).
fn validate_target_dir(target_dir: &Path) -> Result<()> {
    let p = target_dir.to_str().unwrap_or("");
    if p.trim().is_empty() {
        return Err(SmmError::DeployError("Target directory path cannot be empty".into()));
    }

    // Reject filesystem root directories (e.g. "/" or "C:\")
    let abs_path = if target_dir.is_absolute() {
        target_dir.to_path_buf()
    } else {
        std::env::current_dir()
            .map(|cwd| cwd.join(target_dir))
            .unwrap_or_else(|_| target_dir.to_path_buf())
    };

    let has_normal_component = abs_path
        .components()
        .any(|c| matches!(c, std::path::Component::Normal(_)));
    if !has_normal_component {
        return Err(SmmError::DeployError(format!(
            "Refusing to operate on root directory: {}",
            target_dir.display()
        )));
    }

    Ok(())

}

/// Extracts a string representation of the volume/drive prefix for path comparison on Windows.
#[cfg(windows)]
fn get_volume_prefix(p: &Path) -> Option<String> {
    let abs = if p.is_absolute() {
        p.to_path_buf()
    } else {
        std::env::current_dir().unwrap_or_default().join(p)
    };
    for comp in abs.components() {
        if let std::path::Component::Prefix(prefix) = comp {
            return Some(format!("{:?}", prefix.kind()).to_ascii_uppercase());
        }
    }
    None
}

/// Queries volume serial number for a physical file using Win32 `GetFileInformationByHandle`.
#[cfg(windows)]
pub fn get_file_volume_serial_number(path: &Path) -> Option<u32> {
    use std::os::windows::io::AsRawHandle;
    use windows_sys::Win32::Storage::FileSystem::{GetFileInformationByHandle, BY_HANDLE_FILE_INFORMATION};

    let file = fs::File::open(path).ok()?;
    let handle = file.as_raw_handle() as windows_sys::Win32::Foundation::HANDLE;
    unsafe {
        let mut info: BY_HANDLE_FILE_INFORMATION = std::mem::zeroed();
        if GetFileInformationByHandle(handle, &mut info) != 0 {
            Some(info.dwVolumeSerialNumber)
        } else {
            None
        }
    }
}

/// Queries hard link reference count for a file using Win32 `GetFileInformationByHandle`.
#[cfg(windows)]
pub fn get_file_hard_link_count(path: &Path) -> Option<u32> {
    use std::os::windows::io::AsRawHandle;
    use windows_sys::Win32::Storage::FileSystem::{GetFileInformationByHandle, BY_HANDLE_FILE_INFORMATION};

    let file = fs::File::open(path).ok()?;
    let handle = file.as_raw_handle() as windows_sys::Win32::Foundation::HANDLE;
    unsafe {
        let mut info: BY_HANDLE_FILE_INFORMATION = std::mem::zeroed();
        if GetFileInformationByHandle(handle, &mut info) != 0 {
            Some(info.nNumberOfLinks)
        } else {
            None
        }
    }
}

/// Checks whether source and target paths reside on the same filesystem volume.
/// Hard links on Windows NTFS and Unix require source and target to be on the same volume.
pub fn is_same_volume(source: &Path, target: &Path) -> std::io::Result<bool> {
    #[cfg(windows)]
    {
        // 1. Compare drive letter / prefix components
        if let (Some(p1), Some(p2)) = (get_volume_prefix(source), get_volume_prefix(target)) {
            if p1 != p2 {
                return Ok(false);
            }
        }

        // 2. Query volume serial numbers via Win32 GetFileInformationByHandle if both files exist
        if source.is_file() && target.is_file() {
            if let (Some(s1), Some(s2)) = (
                get_file_volume_serial_number(source),
                get_file_volume_serial_number(target),
            ) {
                return Ok(s1 == s2);
            }
        }

        Ok(true)
    }

    #[cfg(not(windows))]
    {
        use std::os::unix::fs::MetadataExt;
        if let Ok(m1) = fs::metadata(source) {
            let mut curr = target.to_path_buf();
            while !curr.as_os_str().is_empty() {
                if curr.exists() {
                    if let Ok(m2) = fs::metadata(&curr) {
                        return Ok(m1.dev() == m2.dev());
                    }
                }
                if !curr.pop() {
                    break;
                }
            }
        }
        Ok(true)
    }
}

/// Creates a hard link pointing `target` to `source`.
/// Uses Windows Native `CreateHardLinkW` on Windows with `std::fs::hard_link` fallback,
/// ensuring parent directories exist and replacing any pre-existing file at `target`.
pub fn create_hard_link(source: &Path, target: &Path) -> std::io::Result<()> {
    if let Some(parent) = target.parent() {
        fs::create_dir_all(parent)?;
    }

    if target.exists() || target.symlink_metadata().is_ok() {
        let _ = fs::remove_file(target);
    }

    #[cfg(windows)]
    {
        use std::os::windows::ffi::OsStrExt;
        use windows_sys::Win32::Foundation::GetLastError;
        use windows_sys::Win32::Storage::FileSystem::CreateHardLinkW;

        let mut target_wide: Vec<u16> = target.as_os_str().encode_wide().collect();
        target_wide.push(0);
        let mut source_wide: Vec<u16> = source.as_os_str().encode_wide().collect();
        source_wide.push(0);

        let res = unsafe {
            CreateHardLinkW(
                target_wide.as_ptr(),
                source_wide.as_ptr(),
                std::ptr::null(),
            )
        };

        if res == 0 {
            let err_code = unsafe { GetLastError() };
            // Fall back to standard library hard_link if path normalization/prefix differs
            std::fs::hard_link(source, target).map_err(|_| {
                std::io::Error::from_raw_os_error(err_code as i32)
            })
        } else {
            Ok(())
        }
    }

    #[cfg(not(windows))]
    {
        std::fs::hard_link(source, target)
    }
}

/// Resolves the physical destination path for a deployed asset.
/// If `target_dir` is the game's `mods` subdirectory (e.g. `<game_dir>/mods`),
/// core loader assets (`dinput8.dll` and `modengine.ini`) are projected into the game root
/// (`<game_dir>/`), where the Sekiro executable and DirectX hook can load them.
/// All other game assets are projected inside `target_dir` (`<game_dir>/mods/...`).
pub fn resolve_destination_path(target_dir: &Path, rel_path: &str) -> PathBuf {
    let is_loader = rel_path.eq_ignore_ascii_case("dinput8.dll")
        || rel_path.eq_ignore_ascii_case("modengine.ini");
    let is_in_mods_sub = target_dir
        .file_name()
        .map(|n| n.to_string_lossy().eq_ignore_ascii_case("mods"))
        .unwrap_or(false);

    if is_loader && is_in_mods_sub {
        if let Some(parent) = target_dir.parent() {
            return parent.join(rel_path);
        }
    }
    target_dir.join(rel_path)
}

/// Recursively removes empty directories within `dir` (bottom-up), without deleting `dir` itself.
fn remove_empty_subdirs(dir: &Path) -> usize {
    let mut removed = 0;
    if !dir.exists() || !dir.is_dir() {
        return removed;
    }

    // Collect all directories
    let mut dirs = Vec::new();
    for entry in walkdir::WalkDir::new(dir).min_depth(1).into_iter().filter_map(|e| e.ok()) {
        if entry.file_type().is_dir() {
            dirs.push(entry.into_path());
        }
    }

    // Sort by depth descending (longest paths first)
    dirs.sort_by(|a, b| b.components().count().cmp(&a.components().count()));

    for d in dirs {
        if let Ok(mut read) = fs::read_dir(&d) {
            if read.next().is_none() {
                if fs::remove_dir(&d).is_ok() {
                    removed += 1;
                }
            }
        }
    }

    removed
}

/// Executes a deployment plan into `target_mods_dir`.
///
/// Features:
/// - Pre-validates target directory against root paths.
/// - Performs atomic cleanup of old deployment links (via `.smm_manifest.json`).
/// - Checks volume consistency: uses NTFS hard links for same-volume; downgrades to physical copies across volumes.
/// - Automatically creates parent directory structures.
/// - Saves `.smm_manifest.json` for deterministic tracking and fast zero-residual restoration.
/// - Returns a structured deployment report.
pub fn execute_deploy(plan: &DeployPlan, target_mods_dir: impl AsRef<Path>) -> Result<DeployResult> {
    let target_dir = target_mods_dir.as_ref();
    validate_target_dir(target_dir)?;

    let start_time = Instant::now();
    fs::create_dir_all(target_dir)?;

    let manifest_path = target_dir.join(".smm_manifest.json");

    // Atomic cleanup of previously deployed files not present in the current plan
    let new_target_paths: HashSet<&str> = plan
        .mappings
        .iter()
        .map(|m| m.target_relative_path.as_str())
        .collect();

    if manifest_path.exists() {
        if let Ok(content) = fs::read_to_string(&manifest_path) {
            if let Ok(old_manifest) = serde_json::from_str::<DeployManifest>(&content) {
                for old_file in old_manifest.files {
                    if !new_target_paths.contains(old_file.as_str()) {
                        let obsolete_path = resolve_destination_path(target_dir, &old_file);
                        if obsolete_path.exists() || obsolete_path.symlink_metadata().is_ok() {
                            let _ = fs::remove_file(&obsolete_path);
                        }
                    }
                }
            }
        }
    }

    let mut hard_links_created = 0;
    let mut copied_files = 0;
    let skipped_files = 0;
    let mut failed_files = 0;
    let mut bytes_saved = 0u64;
    let mut warnings = Vec::new();
    let mut errors = Vec::new();
    let mut successfully_deployed_files = Vec::new();

    for mapping in &plan.mappings {
        let dest_path = resolve_destination_path(target_dir, &mapping.target_relative_path);

        if !mapping.source_path.exists() {
            let msg = format!("Source file does not exist: {}", mapping.source_path.display());
            warnings.push(msg.clone());
            errors.push((mapping.target_relative_path.clone(), msg));
            failed_files += 1;
            continue;
        }

        let file_size = fs::metadata(&mapping.source_path).map(|m| m.len()).unwrap_or(0);
        let same_volume = is_same_volume(&mapping.source_path, target_dir).unwrap_or(true);

        if same_volume {
            match create_hard_link(&mapping.source_path, &dest_path) {
                Ok(()) => {
                    hard_links_created += 1;
                    bytes_saved += file_size;
                    successfully_deployed_files.push(mapping.target_relative_path.clone());
                }
                Err(err) => {
                    // Downgrade to copy on unexpected filesystem error
                    let warn_msg = format!(
                        "Hard link failed for '{}' ({}); falling back to copy",
                        mapping.target_relative_path, err
                    );
                    warnings.push(warn_msg);

                    if let Some(parent) = dest_path.parent() {
                        let _ = fs::create_dir_all(parent);
                    }
                    if dest_path.exists() || dest_path.symlink_metadata().is_ok() {
                        let _ = fs::remove_file(&dest_path);
                    }

                    match fs::copy(&mapping.source_path, &dest_path) {
                        Ok(_) => {
                            copied_files += 1;
                            successfully_deployed_files.push(mapping.target_relative_path.clone());
                        }
                        Err(copy_err) => {
                            errors.push((mapping.target_relative_path.clone(), copy_err.to_string()));
                            failed_files += 1;
                        }
                    }
                }
            }
        } else {
            let warn_msg = format!(
                "Cross-volume detected for '{}'; falling back to physical copy",
                mapping.target_relative_path
            );
            warnings.push(warn_msg);

            if let Some(parent) = dest_path.parent() {
                let _ = fs::create_dir_all(parent);
            }
            if dest_path.exists() || dest_path.symlink_metadata().is_ok() {
                let _ = fs::remove_file(&dest_path);
            }

            match fs::copy(&mapping.source_path, &dest_path) {
                Ok(_) => {
                    copied_files += 1;
                    successfully_deployed_files.push(mapping.target_relative_path.clone());
                }
                Err(copy_err) => {
                    errors.push((mapping.target_relative_path.clone(), copy_err.to_string()));
                    failed_files += 1;
                }
            }
        }
    }

    // Clean any empty directories left over from cleanup
    remove_empty_subdirs(target_dir);

    // Save deploy manifest
    let now_epoch = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0);

    let manifest = DeployManifest {
        version: 1,
        profile: plan.active_profile.clone(),
        deployed_at: now_epoch,
        files: successfully_deployed_files,
        total_bytes: bytes_saved,
    };

    if let Ok(manifest_json) = serde_json::to_string_pretty(&manifest) {
        let _ = fs::write(&manifest_path, manifest_json);
    }

    let duration_ms = start_time.elapsed().as_millis();

    Ok(DeployResult {
        target_dir: target_dir.to_path_buf(),
        total_files: plan.mappings.len(),
        hard_links_created,
        copied_files,
        skipped_files,
        failed_files,
        bytes_saved,
        duration_ms,
        warnings,
        errors,
    })
}

/// Safely removes all deployed mod links and files from `target_mods_dir`.
///
/// Features:
/// - If `.smm_manifest.json` is present, accurately deletes tracked deployed files and the manifest.
/// - If no manifest is found, recursively purges files in `target_mods_dir`.
/// - Prunes all empty subdirectories while leaving `target_mods_dir` clean.
/// - Returns a structured restoration report.
pub fn restore_deploy(target_mods_dir: impl AsRef<Path>) -> Result<RestoreResult> {
    let target_dir = target_mods_dir.as_ref();
    validate_target_dir(target_dir)?;

    let start_time = Instant::now();

    if !target_dir.exists() {
        return Ok(RestoreResult {
            target_dir: target_dir.to_path_buf(),
            removed_files: 0,
            removed_dirs: 0,
            duration_ms: 0,
            success: true,
            warnings: vec!["Target directory does not exist; nothing to restore".into()],
        });
    }

    let manifest_path = target_dir.join(".smm_manifest.json");
    let mut removed_files = 0;
    let mut warnings = Vec::new();

    if manifest_path.exists() {
        if let Ok(content) = fs::read_to_string(&manifest_path) {
            if let Ok(manifest) = serde_json::from_str::<DeployManifest>(&content) {
                for file_rel in manifest.files {
                    let file_path = resolve_destination_path(target_dir, &file_rel);
                    if file_path.exists() || file_path.symlink_metadata().is_ok() {
                        if fs::remove_file(&file_path).is_ok() {
                            removed_files += 1;
                        }
                    }
                }
            } else {
                warnings.push("Corrupted .smm_manifest.json; cleaning all files in target".into());
            }
        }
        let _ = fs::remove_file(&manifest_path);
    }

    // Clean any remaining unmanaged files if no valid manifest or uncleaned files exist
    for entry in walkdir::WalkDir::new(target_dir)
        .min_depth(1)
        .into_iter()
        .filter_map(|e| e.ok())
    {
        if entry.file_type().is_file() || entry.file_type().is_symlink() {
            let path = entry.path();
            if path.file_name().map(|n| n == ".smm_manifest.json").unwrap_or(false) {
                let _ = fs::remove_file(path);
                continue;
            }
            if fs::remove_file(path).is_ok() {
                removed_files += 1;
            }
        }
    }

    // Clean empty subdirectories bottom-up
    let removed_dirs = remove_empty_subdirs(target_dir);

    let duration_ms = start_time.elapsed().as_millis();

    Ok(RestoreResult {
        target_dir: target_dir.to_path_buf(),
        removed_files,
        removed_dirs,
        duration_ms,
        success: true,
        warnings,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::types::{AssetCategory, AssetEntry, DeployMapping, ModInfo};
    use crate::deploy::DeploymentPlanner;

    #[test]
    fn test_is_same_volume_on_tempdir() {
        let temp = tempfile::tempdir().unwrap();
        let src = temp.path().join("source.txt");
        let tgt = temp.path().join("target.txt");
        fs::write(&src, "test").unwrap();

        assert!(is_same_volume(&src, &tgt).unwrap());
    }

    #[test]
    fn test_prevent_root_target_dir() {
        assert!(validate_target_dir(Path::new("")).is_err());
        assert!(validate_target_dir(Path::new("/")).is_err());
        #[cfg(windows)]
        {
            assert!(validate_target_dir(Path::new("C:\\")).is_err());
        }
    }

    #[test]
    fn test_real_hard_link_bidirectional_sync_and_restore() {
        let temp = tempfile::tempdir().unwrap();
        let staging_dir = temp.path().join("staging");
        let target_dir = temp.path().join("game_mods");

        // 1. Create a mock mod in staging
        let mod_dir = staging_dir.join("kusabimaru");
        let wp_source = mod_dir.join("parts/wp_a_0300.partsbnd.dcx");
        fs::create_dir_all(wp_source.parent().unwrap()).unwrap();
        fs::write(&wp_source, b"Initial Katana Mesh Data v1.0").unwrap();

        let mod_info = ModInfo::new(
            "kusabimaru",
            "Kusabimaru",
            "1.0.0",
            "Author",
            "weapon_skin",
        );
        let assets = vec![AssetEntry::new(
            "parts/wp_a_0300.partsbnd.dcx",
            &wp_source,
            wp_source.metadata().unwrap().len(),
            AssetCategory::Parts,
        )];

        let plan = DeploymentPlanner::build_plan("default", &[(mod_info, assets)]).unwrap();
        assert_eq!(plan.mappings.len(), 1);

        // 2. Execute deployment
        let deploy_res = execute_deploy(&plan, &target_dir).expect("Deployment failed");
        assert_eq!(deploy_res.total_files, 1);
        assert_eq!(deploy_res.hard_links_created, 1);
        assert_eq!(deploy_res.failed_files, 0);
        assert_eq!(deploy_res.bytes_saved, b"Initial Katana Mesh Data v1.0".len() as u64);

        let deployed_file = target_dir.join("parts/wp_a_0300.partsbnd.dcx");
        assert!(deployed_file.exists());
        assert_eq!(
            fs::read_to_string(&deployed_file).unwrap(),
            "Initial Katana Mesh Data v1.0"
        );

        // Verify hard link properties
        #[cfg(windows)]
        {
            let links = get_file_hard_link_count(&deployed_file);
            assert_eq!(links, Some(2));
        }

        // 3. Verify bidirectional synchronization (modifying target modifies source)
        fs::write(&deployed_file, b"Patched in-place by external tool").unwrap();
        assert_eq!(
            fs::read_to_string(&wp_source).unwrap(),
            "Patched in-place by external tool",
            "Hard link modification must sync back to source!"
        );

        // 4. Verify manifest exists
        let manifest_path = target_dir.join(".smm_manifest.json");
        assert!(manifest_path.exists());

        // 5. Restore deployment
        let restore_res = restore_deploy(&target_dir).expect("Restore failed");
        assert_eq!(restore_res.removed_files, 1);
        assert_eq!(restore_res.removed_dirs, 1); // "parts" subdirectory removed
        assert!(!deployed_file.exists(), "Deployed file must be removed");
        assert!(!manifest_path.exists(), "Manifest must be removed");

        // Source file must still exist unharmed!
        assert!(wp_source.exists());
        assert_eq!(
            fs::read_to_string(&wp_source).unwrap(),
            "Patched in-place by external tool"
        );
    }

    #[test]
    fn test_atomic_redeployment_removes_obsolete_files() {
        let temp = tempfile::tempdir().unwrap();
        let staging_dir = temp.path().join("staging");
        let target_dir = temp.path().join("game_mods");

        let file_1 = staging_dir.join("f1.txt");
        let file_2 = staging_dir.join("f2.txt");
        fs::create_dir_all(&staging_dir).unwrap();
        fs::write(&file_1, "111").unwrap();
        fs::write(&file_2, "222").unwrap();

        let plan_v1 = DeployPlan {
            active_profile: "default".into(),
            timestamp: 100,
            mappings: vec![
                DeployMapping {
                    target_relative_path: "f1.txt".into(),
                    source_path: file_1.clone(),
                    owner_mod_id: "m1".into(),
                    priority: 1,
                    shadowed_mods: vec![],
                },
                DeployMapping {
                    target_relative_path: "sub/f2.txt".into(),
                    source_path: file_2.clone(),
                    owner_mod_id: "m1".into(),
                    priority: 1,
                    shadowed_mods: vec![],
                },
            ],
            conflict_report: Default::default(),
        };

        execute_deploy(&plan_v1, &target_dir).unwrap();
        assert!(target_dir.join("f1.txt").exists());
        assert!(target_dir.join("sub/f2.txt").exists());

        // Plan v2 removes sub/f2.txt
        let plan_v2 = DeployPlan {
            active_profile: "default".into(),
            timestamp: 200,
            mappings: vec![DeployMapping {
                target_relative_path: "f1.txt".into(),
                source_path: file_1,
                owner_mod_id: "m1".into(),
                priority: 1,
                shadowed_mods: vec![],
            }],
            conflict_report: Default::default(),
        };

        execute_deploy(&plan_v2, &target_dir).unwrap();
        assert!(target_dir.join("f1.txt").exists());
        assert!(
            !target_dir.join("sub/f2.txt").exists(),
            "Obsolete file must be cleaned automatically"
        );
        assert!(
            !target_dir.join("sub").exists(),
            "Empty directory should be cleaned"
        );
    }
}
