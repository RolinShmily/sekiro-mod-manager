use std::collections::HashMap;
use std::fs;
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::error::{Result, SmmError};
use crate::types::ModInfo;
use crate::executor::is_same_volume;

/// Minimum plausible size, in bytes, of a genuine ModEngine `dinput8.dll`.
///
/// The real hook library is several hundred kilobytes; anything at or below this
/// threshold is a stub/placeholder and must never be deployed as a hook.
const MOD_ENGINE_DLL_MIN_SIZE: u64 = 10_000;

/// Actionable guidance for obtaining ModEngine, used in diagnostics and errors.
///
/// SMM deliberately does **not** embed or redistribute ModEngine: upstream publishes no
/// license (`katalash/ModEngine` has no LICENSE file and GitHub reports `license: null`),
/// and its own readme states "All rights reserved" while permitting redistribution only of
/// an *unmodified* copy *bundled with a mod*. SMM is a mod manager, not a mod, so it cannot
/// rely on that grant — the user must supply ModEngine themselves.
pub const MOD_ENGINE_SOURCE_HINT: &str =
    "Sekiro Mod Engine (dinput8.dll) is third-party software by katalash that SMM does not \
     redistribute. Download it from https://www.nexusmods.com/sekiro/mods/6 (or \
     https://github.com/katalash/ModEngine), then place a genuine dinput8.dll (>10 KB) in the \
     ModEngine source folder or in staging/mod-engine/";

/// True when `path` points at a file large enough to be a genuine ModEngine DLL.
fn is_valid_mod_engine_dll(path: &Path) -> bool {
    fs::metadata(path)
        .map(|m| m.is_file() && m.len() > MOD_ENGINE_DLL_MIN_SIZE)
        .unwrap_or(false)
}

/// Returns the first candidate holding a genuine, user-supplied ModEngine `dinput8.dll`.
fn find_valid_mod_engine_dll<I>(candidates: I) -> Option<PathBuf>
where
    I: IntoIterator<Item = PathBuf>,
{
    candidates.into_iter().find(|c| is_valid_mod_engine_dll(c))
}

/// Candidate locations for a user-supplied ModEngine `dinput8.dll`.
fn dinput8_candidates(source_or_staging: Option<&Path>) -> Vec<PathBuf> {
    let mut candidates = Vec::new();

    if let Some(src) = source_or_staging {
        candidates.push(src.join("dinput8.dll"));
        candidates.push(src.join("mod-engine").join("dinput8.dll"));
        candidates.push(src.join("mod-engine-0.1.16").join("dinput8.dll"));
    }

    candidates.push(PathBuf::from("staging/mod-engine/dinput8.dll"));
    candidates.push(PathBuf::from("staging/mod-engine-0.1.16/dinput8.dll"));
    candidates.push(PathBuf::from("dinput8.dll"));

    candidates
}

/// Diagnostic status for an individual health check.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum DiagnosticStatus {
    Pass,
    Warning,
    Fail,
}

impl DiagnosticStatus {
    pub fn is_pass(&self) -> bool {
        matches!(self, DiagnosticStatus::Pass)
    }

    pub fn is_warning(&self) -> bool {
        matches!(self, DiagnosticStatus::Warning)
    }

    pub fn is_fail(&self) -> bool {
        matches!(self, DiagnosticStatus::Fail)
    }
}

/// Overall system health rating.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum OverallHealth {
    /// All critical components and configurations are optimal.
    Healthy,
    /// System is operational, but potential degradation or suboptimal settings exist.
    Degraded,
    /// Critical files missing or disabled; mods will not function.
    ActionRequired,
}

/// Result of an individual diagnostic check.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DiagnosticItem {
    pub category: String,
    pub name: String,
    pub status: DiagnosticStatus,
    pub message: String,
    pub remediation: Option<String>,
}

/// Comprehensive health inspection report for the game installation and modding environment.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct HealthReport {
    pub game_dir: PathBuf,
    pub staging_dir: Option<PathBuf>,
    pub items: Vec<DiagnosticItem>,
    pub overall_status: OverallHealth,
}

impl HealthReport {
    pub fn is_healthy(&self) -> bool {
        self.overall_status == OverallHealth::Healthy
    }

    pub fn pass_count(&self) -> usize {
        self.items.iter().filter(|i| i.status.is_pass()).count()
    }

    pub fn warning_count(&self) -> usize {
        self.items.iter().filter(|i| i.status.is_warning()).count()
    }

    pub fn fail_count(&self) -> usize {
        self.items.iter().filter(|i| i.status.is_fail()).count()
    }
}

/// Parsed representation of `modengine.ini`.
#[derive(Debug, Clone, Default)]
pub struct ModEngineConfig {
    pub enabled: Option<bool>,
    pub load_uxm_files: Option<bool>,
    pub cache_paths: Option<bool>,
    pub mod_override_directory: Option<String>,
    pub load_loose_params: Option<bool>,
    pub sections: HashMap<String, HashMap<String, String>>,
}

/// Parses standard INI format used by ModEngine.
pub fn parse_modengine_ini(content: &str) -> ModEngineConfig {
    let mut config = ModEngineConfig::default();
    let mut current_section = String::new();

    for line in content.lines() {
        let trimmed = line.trim();
        if trimmed.is_empty() || trimmed.starts_with(';') || trimmed.starts_with('#') {
            continue;
        }

        if trimmed.starts_with('[') && trimmed.ends_with(']') {
            current_section = trimmed[1..trimmed.len() - 1].trim().to_ascii_lowercase();
            config
                .sections
                .entry(current_section.clone())
                .or_default();
            continue;
        }

        if let Some((raw_key, raw_val)) = trimmed.split_once('=') {
            let key = raw_key.trim();
            let mut val = raw_val.trim();

            // Strip inline comment if any
            if let Some((v, _)) = val.split_once(';') {
                val = v.trim();
            }

            // Strip quotes
            let cleaned_val = if (val.starts_with('"') && val.ends_with('"'))
                || (val.starts_with('\'') && val.ends_with('\''))
            {
                if val.len() >= 2 {
                    &val[1..val.len() - 1]
                } else {
                    val
                }
            } else {
                val
            };

            config
                .sections
                .entry(current_section.clone())
                .or_default()
                .insert(key.to_ascii_lowercase(), cleaned_val.to_string());

            if current_section == "files" {
                match key.to_ascii_lowercase().as_str() {
                    "enabled" => {
                        config.enabled = Some(cleaned_val == "1" || cleaned_val.eq_ignore_ascii_case("true"));
                    }
                    "loaduxmfiles" => {
                        config.load_uxm_files = Some(cleaned_val == "1" || cleaned_val.eq_ignore_ascii_case("true"));
                    }
                    "cachepaths" => {
                        config.cache_paths = Some(cleaned_val == "1" || cleaned_val.eq_ignore_ascii_case("true"));
                    }
                    "modoverridedirectory" => {
                        config.mod_override_directory = Some(cleaned_val.to_string());
                    }
                    "loadlooseparams" => {
                        config.load_loose_params = Some(cleaned_val == "1" || cleaned_val.eq_ignore_ascii_case("true"));
                    }
                    _ => {}
                }
            }
        }
    }

    config
}

/// Generates or patches `modengine.ini` content ensuring:
/// - `enabled=1`
/// - `modOverrideDirectory="\mods"` (or designated folder)
/// - `loadLooseParams=1`
pub fn patch_or_create_modengine_ini(
    existing_content: Option<&str>,
    mod_override_dir: &str,
) -> String {
    let override_val = if mod_override_dir.starts_with('\\') || mod_override_dir.starts_with('/') {
        format!("\"{}\"", mod_override_dir)
    } else {
        format!("\"\\{}\"", mod_override_dir)
    };

    if let Some(content) = existing_content {
        let mut lines: Vec<String> = Vec::new();
        let mut in_files_section = false;
        let mut has_files_section = false;
        let mut seen_enabled = false;
        let mut seen_override = false;
        let mut seen_loose_params = false;

        for line in content.lines() {
            let trimmed = line.trim();
            if trimmed.starts_with('[') && trimmed.ends_with(']') {
                let sec = trimmed[1..trimmed.len() - 1].trim();
                if in_files_section {
                    // Flush missing keys before leaving [files] section
                    if !seen_enabled {
                        lines.push("enabled=1".to_string());
                    }
                    if !seen_override {
                        lines.push(format!("modOverrideDirectory={}", override_val));
                    }
                    if !seen_loose_params {
                        lines.push("loadLooseParams=1".to_string());
                    }
                }
                in_files_section = sec.eq_ignore_ascii_case("files");
                if in_files_section {
                    has_files_section = true;
                }
                lines.push(line.to_string());
                continue;
            }

            if in_files_section {
                if let Some((k, _)) = trimmed.split_once('=') {
                    let key = k.trim().to_ascii_lowercase();
                    if key == "enabled" {
                        lines.push("enabled=1".to_string());
                        seen_enabled = true;
                        continue;
                    } else if key == "modoverridedirectory" {
                        lines.push(format!("modOverrideDirectory={}", override_val));
                        seen_override = true;
                        continue;
                    } else if key == "loadlooseparams" {
                        lines.push("loadLooseParams=1".to_string());
                        seen_loose_params = true;
                        continue;
                    }
                }
            }

            lines.push(line.to_string());
        }

        if in_files_section {
            if !seen_enabled {
                lines.push("enabled=1".to_string());
            }
            if !seen_override {
                lines.push(format!("modOverrideDirectory={}", override_val));
            }
            if !seen_loose_params {
                lines.push("loadLooseParams=1".to_string());
            }
        } else if !has_files_section {
            lines.push(String::new());
            lines.push("[files]".to_string());
            lines.push("enabled=1".to_string());
            lines.push("loadUXMFiles=0".to_string());
            lines.push("cachePaths=1".to_string());
            lines.push(format!("modOverrideDirectory={}", override_val));
            lines.push("loadLooseParams=1".to_string());
        }

        lines.join("\r\n")
    } else {
        format!(
            "; Sekiro Mod Engine configuration file\r\n\
             ; Generated & maintained by Sekiro Mod Manager (SMM)\r\n\
             \r\n\
             [files]\r\n\
             enabled=1\r\n\
             loadUXMFiles=0\r\n\
             cachePaths=1\r\n\
             modOverrideDirectory={}\r\n\
             loadLooseParams=1\r\n\
             \r\n\
             [debug]\r\n\
             showDebugConsole=0\r\n\
             logFile=\"modengine.log\"\r\n",
            override_val
        )
    }
}

/// Diagnoses the Sekiro game environment and ModEngine setup.
pub fn diagnose_environment(game_dir: &Path, staging_dir: Option<&Path>) -> HealthReport {
    let mut items = Vec::new();

    // 1. Game Root Directory & sekiro.exe
    if !game_dir.exists() {
        items.push(DiagnosticItem {
            category: "Game Binary".to_string(),
            name: "Game Directory".to_string(),
            status: DiagnosticStatus::Fail,
            message: format!("Directory does not exist: {}", game_dir.display()),
            remediation: Some("Verify your Sekiro installation folder path.".to_string()),
        });
    } else if !game_dir.is_dir() {
        items.push(DiagnosticItem {
            category: "Game Binary".to_string(),
            name: "Game Directory".to_string(),
            status: DiagnosticStatus::Fail,
            message: format!("Specified path is not a directory: {}", game_dir.display()),
            remediation: Some("Specify the root folder containing sekiro.exe.".to_string()),
        });
    } else {
        // Look for sekiro.exe (case-insensitive check on directory contents)
        let exe_found = fs::read_dir(game_dir)
            .map(|entries| {
                entries.filter_map(|e| e.ok()).any(|e| {
                    e.file_name()
                        .to_string_lossy()
                        .eq_ignore_ascii_case("sekiro.exe")
                })
            })
            .unwrap_or(false);

        if exe_found {
            items.push(DiagnosticItem {
                category: "Game Binary".to_string(),
                name: "sekiro.exe".to_string(),
                status: DiagnosticStatus::Pass,
                message: "Sekiro executable (sekiro.exe) found and accessible.".to_string(),
                remediation: None,
            });
        } else {
            items.push(DiagnosticItem {
                category: "Game Binary".to_string(),
                name: "sekiro.exe".to_string(),
                status: DiagnosticStatus::Fail,
                message: "sekiro.exe not found in specified game directory.".to_string(),
                remediation: Some(
                    "Ensure --game-dir points to the folder containing sekiro.exe \
                     (e.g., steamapps/common/Sekiro)."
                        .to_string(),
                ),
            });
        }
    }

    // 2. ModEngine Hook DLL (dinput8.dll)
    let dinput8_path = game_dir.join("dinput8.dll");
    if dinput8_path.exists() && dinput8_path.is_file() {
        let size = fs::metadata(&dinput8_path).map(|m| m.len()).unwrap_or(0);
        if size > MOD_ENGINE_DLL_MIN_SIZE {
            items.push(DiagnosticItem {
                category: "ModEngine Hook".to_string(),
                name: "dinput8.dll".to_string(),
                status: DiagnosticStatus::Pass,
                message: format!(
                    "ModEngine entry hook (dinput8.dll) is installed ({} bytes).",
                    size
                ),
                remediation: None,
            });
        } else {
            items.push(DiagnosticItem {
                category: "ModEngine Hook".to_string(),
                name: "dinput8.dll".to_string(),
                status: DiagnosticStatus::Warning,
                message: format!("dinput8.dll is only {} bytes (stub/placeholder).", size),
                remediation: Some(MOD_ENGINE_SOURCE_HINT.to_string()),
            });
        }
    } else {
        items.push(DiagnosticItem {
            category: "ModEngine Hook".to_string(),
            name: "dinput8.dll".to_string(),
            status: DiagnosticStatus::Fail,
            message: "dinput8.dll hook library is missing from the game directory.".to_string(),
            remediation: Some(MOD_ENGINE_SOURCE_HINT.to_string()),
        });
    }

    // 3. ModEngine Configuration (modengine.ini)
    let ini_path = game_dir.join("modengine.ini");
    if ini_path.exists() && ini_path.is_file() {
        match fs::read_to_string(&ini_path) {
            Ok(content) => {
                items.push(DiagnosticItem {
                    category: "ModEngine Config".to_string(),
                    name: "modengine.ini".to_string(),
                    status: DiagnosticStatus::Pass,
                    message: "ModEngine configuration file (modengine.ini) found and valid.".to_string(),
                    remediation: None,
                });

                let config = parse_modengine_ini(&content);

                // 3a. enabled
                match config.enabled {
                    Some(true) => {
                        items.push(DiagnosticItem {
                            category: "ModEngine Config".to_string(),
                            name: "enabled".to_string(),
                            status: DiagnosticStatus::Pass,
                            message: "ModEngine hook execution is enabled (enabled=1).".to_string(),
                            remediation: None,
                        });
                    }
                    Some(false) => {
                        items.push(DiagnosticItem {
                            category: "ModEngine Config".to_string(),
                            name: "enabled".to_string(),
                            status: DiagnosticStatus::Fail,
                            message: "ModEngine is disabled in config (enabled=0). Loose mods will be ignored.".to_string(),
                            remediation: Some("Set 'enabled=1' under [files] in modengine.ini or run 'smm setup-engine'.".to_string()),
                        });
                    }
                    None => {
                        items.push(DiagnosticItem {
                            category: "ModEngine Config".to_string(),
                            name: "enabled".to_string(),
                            status: DiagnosticStatus::Warning,
                            message: "'enabled' setting is missing under [files]. Defaults may apply.".to_string(),
                            remediation: Some("Add 'enabled=1' under [files] in modengine.ini.".to_string()),
                        });
                    }
                }

                // 3b. modOverrideDirectory
                match &config.mod_override_directory {
                    Some(override_dir) if !override_dir.trim().is_empty() => {
                        let clean_sub = override_dir.trim().trim_start_matches('\\').trim_start_matches('/');
                        let target_sub_dir = game_dir.join(clean_sub);

                        if target_sub_dir.exists() {
                            items.push(DiagnosticItem {
                                category: "ModEngine Config".to_string(),
                                name: "modOverrideDirectory".to_string(),
                                status: DiagnosticStatus::Pass,
                                message: format!(
                                    "Override directory is set to '{}' and exists on disk.",
                                    override_dir
                                ),
                                remediation: None,
                            });
                        } else {
                            items.push(DiagnosticItem {
                                category: "ModEngine Config".to_string(),
                                name: "modOverrideDirectory".to_string(),
                                status: DiagnosticStatus::Warning,
                                message: format!(
                                    "Override directory is set to '{}', but folder does not exist yet.",
                                    override_dir
                                ),
                                remediation: Some(format!(
                                    "Folder will be created automatically on first mod deployment via 'smm deploy'."
                                )),
                            });
                        }
                    }
                    Some(_) | None => {
                        items.push(DiagnosticItem {
                            category: "ModEngine Config".to_string(),
                            name: "modOverrideDirectory".to_string(),
                            status: DiagnosticStatus::Fail,
                            message: "'modOverrideDirectory' is missing or empty in modengine.ini.".to_string(),
                            remediation: Some("Set modOverrideDirectory=\"\\mods\" in modengine.ini.".to_string()),
                        });
                    }
                }

                // 3c. loadLooseParams
                match config.load_loose_params {
                    Some(true) => {
                        items.push(DiagnosticItem {
                            category: "ModEngine Config".to_string(),
                            name: "loadLooseParams".to_string(),
                            status: DiagnosticStatus::Pass,
                            message: "Loose parameter override is enabled (loadLooseParams=1).".to_string(),
                            remediation: None,
                        });
                    }
                    Some(false) | None => {
                        items.push(DiagnosticItem {
                            category: "ModEngine Config".to_string(),
                            name: "loadLooseParams".to_string(),
                            status: DiagnosticStatus::Warning,
                            message: "loadLooseParams is disabled or unset. Loose param files may not load without gameparam.parambnd.dcx.".to_string(),
                            remediation: Some("Set 'loadLooseParams=1' in modengine.ini to allow loose param modifications.".to_string()),
                        });
                    }
                }
            }
            Err(e) => {
                items.push(DiagnosticItem {
                    category: "ModEngine Config".to_string(),
                    name: "modengine.ini".to_string(),
                    status: DiagnosticStatus::Fail,
                    message: format!("Failed to read modengine.ini: {}", e),
                    remediation: Some("Verify file read permissions for modengine.ini.".to_string()),
                });
            }
        }
    } else {
        items.push(DiagnosticItem {
            category: "ModEngine Config".to_string(),
            name: "modengine.ini".to_string(),
            status: DiagnosticStatus::Fail,
            message: "modengine.ini configuration file is missing from game directory.".to_string(),
            remediation: Some("Run 'smm setup-engine --game-dir <path>' to generate standard configuration.".to_string()),
        });
    }

    // 4. Filesystem & Hard Link Volume Compatibility
    if let Some(staging) = staging_dir {
        if staging.exists() {
            match is_same_volume(game_dir, staging) {
                Ok(true) => {
                    items.push(DiagnosticItem {
                        category: "Filesystem".to_string(),
                        name: "NTFS Hard Link Volume".to_string(),
                        status: DiagnosticStatus::Pass,
                        message: "Game and staging directories share the same volume. Instant zero-copy hard links supported.".to_string(),
                        remediation: None,
                    });
                }
                Ok(false) => {
                    items.push(DiagnosticItem {
                        category: "Filesystem".to_string(),
                        name: "NTFS Hard Link Volume".to_string(),
                        status: DiagnosticStatus::Warning,
                        message: "Game and staging directories are on different drive volumes. Deployments will fall back to physical copying.".to_string(),
                        remediation: Some(
                            "Relocate staging directory to the same drive partition as Sekiro to enable instant hard links."
                                .to_string(),
                        ),
                    });
                }
                Err(e) => {
                    items.push(DiagnosticItem {
                        category: "Filesystem".to_string(),
                        name: "NTFS Hard Link Volume".to_string(),
                        status: DiagnosticStatus::Warning,
                        message: format!("Could not inspect volume serial numbers: {}", e),
                        remediation: None,
                    });
                }
            }
        } else {
            items.push(DiagnosticItem {
                category: "Filesystem".to_string(),
                name: "Staging Directory".to_string(),
                status: DiagnosticStatus::Warning,
                message: format!("Specified staging directory does not exist: {}", staging.display()),
                remediation: Some("Initialize staging directory or run 'smm import'.".to_string()),
            });
        }
    } else {
        items.push(DiagnosticItem {
            category: "Filesystem".to_string(),
            name: "Staging Directory".to_string(),
            status: DiagnosticStatus::Warning,
            message: "No staging directory specified for volume compatibility analysis.".to_string(),
            remediation: Some("Specify --staging <path> to verify NTFS hard link compatibility.".to_string()),
        });
    }

    // Compute Overall Health Rating
    let overall_status = if items.iter().any(|i| i.status.is_fail()) {
        OverallHealth::ActionRequired
    } else if items.iter().any(|i| i.status.is_warning()) {
        OverallHealth::Degraded
    } else {
        OverallHealth::Healthy
    };

    HealthReport {
        game_dir: game_dir.to_path_buf(),
        staging_dir: staging_dir.map(|p| p.to_path_buf()),
        items,
        overall_status,
    }
}

/// Installs or repairs ModEngine in the designated game directory.
///
/// 1. Ensures `game_dir` exists.
/// 2. Deploys a **user-supplied** `dinput8.dll` into `game_dir`. A valid DLL already present in
///    `game_dir` is preserved, so this doubles as a repair path.
/// 3. Generates or patches `modengine.ini` ensuring `enabled=1`, `modOverrideDirectory="\mods"`,
///    and `loadLooseParams=1`.
/// 4. Ensures the `mods/` directory exists inside `game_dir`.
///
/// Returns [`SmmError::ModEngineSourceMissing`] when no genuine ModEngine payload is available;
/// see [`MOD_ENGINE_SOURCE_HINT`]. SMM never fabricates or embeds a hook DLL.
pub fn install_mod_engine(
    game_dir: &Path,
    source_files_or_staging: Option<&Path>,
) -> Result<()> {
    fs::create_dir_all(game_dir)?;

    let target_dll = game_dir.join("dinput8.dll");
    let target_ini = game_dir.join("modengine.ini");
    let target_mods_dir = game_dir.join("mods");

    // 1. Deploy dinput8.dll, sourced only from a user-supplied ModEngine payload.
    if !is_valid_mod_engine_dll(&target_dll) {
        match find_valid_mod_engine_dll(dinput8_candidates(source_files_or_staging)) {
            Some(src_dll) => {
                fs::copy(&src_dll, &target_dll)?;
            }
            None => return Err(SmmError::ModEngineSourceMissing(MOD_ENGINE_SOURCE_HINT.to_string())),
        }
    }

    // 2. Deploy or patch modengine.ini
    let existing_content = if target_ini.exists() {
        fs::read_to_string(&target_ini).ok()
    } else {
        None
    };

    let patched_ini = patch_or_create_modengine_ini(existing_content.as_deref(), "\\mods");
    fs::write(&target_ini, patched_ini)?;

    // 3. Ensure mods folder exists
    fs::create_dir_all(&target_mods_dir)?;

    Ok(())
}

/// Provisions Sekiro Mod Engine into the staging directory as a managed Mod.
///
/// Creates `staging/mod-engine/` with a user-supplied `dinput8.dll`, an SMM-generated
/// `modengine.ini`, and standard `mod.json`. No upstream ModEngine file is copied: the DLL must
/// come from a user-supplied payload, and the INI is generated by
/// [`patch_or_create_modengine_ini`].
///
/// Returns [`SmmError::ModEngineSourceMissing`] when no genuine ModEngine payload is available;
/// see [`MOD_ENGINE_SOURCE_HINT`].
pub fn provision_mod_engine(staging_dir: &Path) -> Result<ModInfo> {
    fs::create_dir_all(staging_dir)?;
    let engine_dir = staging_dir.join("mod-engine");
    fs::create_dir_all(&engine_dir)?;

    let dll_path = engine_dir.join("dinput8.dll");
    let ini_path = engine_dir.join("modengine.ini");
    let json_path = engine_dir.join("mod.json");

    // 1. Materialize dinput8.dll from a user-supplied payload only.
    if !is_valid_mod_engine_dll(&dll_path) {
        let candidates = dinput8_candidates(Some(staging_dir))
            .into_iter()
            .filter(|c| c != &dll_path);
        match find_valid_mod_engine_dll(candidates) {
            Some(src_dll) => {
                fs::copy(&src_dll, &dll_path)?;
            }
            None => return Err(SmmError::ModEngineSourceMissing(MOD_ENGINE_SOURCE_HINT.to_string())),
        }
    }

    // 2. Generate modengine.ini from SMM's own defaults (never copied from upstream).
    if !ini_path.exists() {
        fs::write(&ini_path, patch_or_create_modengine_ini(None, "\\mods"))?;
    }

    let mut info = ModInfo::new(
        "mod-engine",
        "Sekiro Mod Engine",
        "0.1.16",
        "katalash",
        "loader",
    );
    info.description = Some("Official runtime file injection & DirectX input hook for Sekiro: Shadows Die Twice".to_string());
    info.priority = 0;
    info.source_url = Some("https://www.nexusmods.com/sekiro/mods/6".to_string());
    info.homepage = Some("https://www.nexusmods.com/sekiro/mods/6".to_string());
    // ModEngine publishes no open-source license; do not assert one on its behalf.
    info.license = None;

    let json_str = serde_json::to_string_pretty(&info)?;
    fs::write(&json_path, json_str)?;

    Ok(info)
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    #[test]
    fn test_parse_and_patch_modengine_ini() {
        let raw = r#"
[files]
enabled = 0
modOverrideDirectory = "\custom_mods"
"#;
        let config = parse_modengine_ini(raw);
        assert_eq!(config.enabled, Some(false));
        assert_eq!(config.mod_override_directory, Some(r"\custom_mods".to_string()));
        assert_eq!(config.load_loose_params, None);

        let patched = patch_or_create_modengine_ini(Some(raw), "\\mods");
        let patched_config = parse_modengine_ini(&patched);
        assert_eq!(patched_config.enabled, Some(true));
        assert_eq!(patched_config.mod_override_directory, Some(r"\mods".to_string()));
        assert_eq!(patched_config.load_loose_params, Some(true));
    }

    #[test]
    fn test_diagnose_environment_scenarios() {
        let tmp = tempdir().unwrap();
        let game_dir = tmp.path().join("Sekiro");
        let staging_dir = tmp.path().join("staging");
        fs::create_dir_all(&game_dir).unwrap();
        fs::create_dir_all(&staging_dir).unwrap();

        // Scenario 1: Empty game dir -> Fail on sekiro.exe, dinput8.dll, modengine.ini
        let report1 = diagnose_environment(&game_dir, Some(&staging_dir));
        assert_eq!(report1.overall_status, OverallHealth::ActionRequired);
        assert!(report1.fail_count() >= 3);

        // Scenario 2: Add sekiro.exe
        fs::write(game_dir.join("sekiro.exe"), b"mock exe binary").unwrap();
        let report2 = diagnose_environment(&game_dir, Some(&staging_dir));
        assert_eq!(report2.overall_status, OverallHealth::ActionRequired);
        assert!(report2.items.iter().any(|i| i.name == "sekiro.exe" && i.status.is_pass()));

        // Scenario 3: Add dummy dinput8.dll
        fs::write(game_dir.join("dinput8.dll"), b"mock dll hook").unwrap();

        // Scenario 4: Supply a user-provided ModEngine payload, then install via
        // install_mod_engine. SMM embeds no DLL, so a source is mandatory here.
        let engine_src = staging_dir.join("mod-engine");
        fs::create_dir_all(&engine_src).unwrap();
        fs::write(engine_src.join("dinput8.dll"), vec![0u8; 20_000]).unwrap();
        install_mod_engine(&game_dir, Some(&staging_dir)).expect("install_mod_engine should succeed");

        let report3 = diagnose_environment(&game_dir, Some(&staging_dir));
        assert_eq!(report3.overall_status, OverallHealth::Healthy);
        assert_eq!(report3.fail_count(), 0);
        assert_eq!(report3.warning_count(), 0);
    }

    #[test]
    fn test_install_mod_engine_refuses_without_user_supplied_dll() {
        let tmp = tempdir().unwrap();
        let game_dir = tmp.path().join("Sekiro");
        fs::create_dir_all(&game_dir).unwrap();

        // No ModEngine payload anywhere -> must fail loudly instead of fabricating a hook DLL.
        let err = install_mod_engine(&game_dir, Some(tmp.path()))
            .expect_err("must refuse to install ModEngine without a user-supplied payload");
        assert!(matches!(err, SmmError::ModEngineSourceMissing(_)));
        assert!(
            !game_dir.join("dinput8.dll").exists(),
            "SMM must not fabricate or embed a hook DLL"
        );

        // A stub below the size threshold must be rejected as well.
        let engine_src = tmp.path().join("mod-engine");
        fs::create_dir_all(&engine_src).unwrap();
        fs::write(engine_src.join("dinput8.dll"), b"stub").unwrap();
        let err = install_mod_engine(&game_dir, Some(tmp.path()))
            .expect_err("a stub payload must not satisfy the requirement");
        assert!(matches!(err, SmmError::ModEngineSourceMissing(_)));
        assert!(!game_dir.join("dinput8.dll").exists());

        // An installed valid DLL keeps the repair path working without a source.
        fs::write(game_dir.join("dinput8.dll"), vec![0u8; 20_000]).unwrap();
        install_mod_engine(&game_dir, None).expect("repair with an installed DLL should succeed");
        assert!(game_dir.join("modengine.ini").exists());
    }

    #[test]
    fn test_provision_mod_engine_requires_user_supplied_dll() {
        let tmp = tempdir().unwrap();
        let staging = tmp.path().join("staging");
        fs::create_dir_all(&staging).unwrap();

        let err = provision_mod_engine(&staging)
            .expect_err("must refuse to provision ModEngine without a user-supplied payload");
        assert!(matches!(err, SmmError::ModEngineSourceMissing(_)));
        assert!(!staging.join("mod-engine/dinput8.dll").exists());

        // With a genuine payload present, provisioning succeeds and records no license.
        fs::create_dir_all(staging.join("mod-engine")).unwrap();
        fs::write(staging.join("mod-engine/dinput8.dll"), vec![0u8; 20_000]).unwrap();
        let info = provision_mod_engine(&staging).expect("provision should succeed");
        assert_eq!(info.id, "mod-engine");
        assert_eq!(info.license, None, "ModEngine publishes no license to assert");
        assert!(staging.join("mod-engine/modengine.ini").exists());
        assert!(
            !staging.join("mod-engine/readme.txt").exists(),
            "must not redistribute upstream documentation"
        );
    }
}
