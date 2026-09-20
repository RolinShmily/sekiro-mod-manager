use std::path::PathBuf;

use smm_core::{
    diagnose_environment, execute_deploy, install_mod_engine, restore_deploy, ConflictEngine,
    ConflictReport, DeployResult, DeploymentPlanner, HealthReport, ModInfo, ModLoader,
    RestoreResult,
};

use crate::commands::util::target_mods_dir;

/// Scans active mods in staging and returns collision / shadowing matrix report.
#[tauri::command]
pub fn scan_conflicts(staging_dir: String) -> Result<ConflictReport, String> {
    let path = PathBuf::from(&staging_dir);
    if !path.exists() {
        return Err(format!(
            "Staging directory does not exist: {}",
            staging_dir
        ));
    }

    let mods = ModLoader::scan_mods_directory(&path)
        .map_err(|e| format!("Failed to read mods from {}: {}", staging_dir, e))?;

    Ok(ConflictEngine::scan_conflicts(&mods))
}

/// Executes Win32 NTFS hard link deployment of enabled mods into the Sekiro game directory.
#[tauri::command]
pub fn deploy_mods(game_dir: String, staging_dir: String) -> Result<DeployResult, String> {
    let g_path = PathBuf::from(&game_dir);
    let s_path = PathBuf::from(&staging_dir);

    if !s_path.exists() {
        return Err(format!(
            "Staging directory does not exist: {}",
            staging_dir
        ));
    }

    let mods = ModLoader::scan_mods_directory(&s_path)
        .map_err(|e| format!("Failed to read mods from {}: {}", staging_dir, e))?;

    if mods.is_empty() {
        return Err("No mods found in staging directory to deploy.".to_string());
    }

    let plan = DeploymentPlanner::build_plan("default", &mods)
        .map_err(|e| format!("Failed to build deployment plan: {}", e))?;

    execute_deploy(&plan, &target_mods_dir(&g_path))
        .map_err(|e| format!("Deployment execution failed: {}", e))
}

/// Restores game directory to vanilla state by removing deployed hardlinks and files.
#[tauri::command]
pub fn restore_mods(game_dir: String) -> Result<RestoreResult, String> {
    let g_path = PathBuf::from(&game_dir);
    restore_deploy(&target_mods_dir(&g_path))
        .map_err(|e| format!("Restore execution failed: {}", e))
}

/// Performs multi-dimensional health check on the Sekiro game directory and ModEngine setup.
#[tauri::command]
pub fn diagnose_env(game_dir: String, staging_dir: String) -> Result<HealthReport, String> {
    let g_path = PathBuf::from(&game_dir);
    let s_path = optional_dir(&staging_dir);
    Ok(diagnose_environment(&g_path, s_path.as_deref()))
}

/// Installs or patches ModEngine and dinput8.dll into the game directory.
#[tauri::command]
pub fn setup_mod_engine(game_dir: String, staging_dir: String) -> Result<(), String> {
    let g_path = PathBuf::from(&game_dir);
    let s_path = optional_dir(&staging_dir);
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

/// Launches Sekiro: Shadows Die Twice (sekiro.exe) from the configured game directory.
#[tauri::command]
pub async fn launch_game(game_dir: String) -> Result<(), String> {
    let trimmed = game_dir.trim();
    if trimmed.is_empty() {
        return Err("游戏目录尚未配置，请先在设置中指定只狼游戏目录".to_string());
    }
    let dir = PathBuf::from(trimmed);
    let exe = dir.join("sekiro.exe");
    if !exe.exists() {
        return Err(format!(
            "未在指定目录中找到可执行文件 sekiro.exe: {}",
            dir.display()
        ));
    }

    let mut cmd = std::process::Command::new(&exe);
    cmd.current_dir(&dir);

    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        const DETACHED_PROCESS: u32 = 0x00000008;
        cmd.creation_flags(DETACHED_PROCESS);
    }

    cmd.spawn()
        .map_err(|e| format!("启动游戏 sekiro.exe 失败: {}", e))?;

    Ok(())
}

/// Maps a possibly empty string to `None`.
fn optional_dir(dir: &str) -> Option<PathBuf> {
    if dir.trim().is_empty() {
        None
    } else {
        Some(PathBuf::from(dir))
    }
}