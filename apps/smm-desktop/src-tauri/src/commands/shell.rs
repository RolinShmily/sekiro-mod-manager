use std::path::Path;

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

/// Opens a file or directory in the system default file manager (Windows Explorer).
#[tauri::command]
pub async fn open_path_in_explorer(path: String) -> Result<(), String> {
    let trimmed = path.trim();
    if trimmed.is_empty() {
        return Err("路径不能为空".to_string());
    }
    let p = Path::new(trimmed);
    if !p.exists() {
        return Err(format!("目标路径不存在: {}", trimmed));
    }

    open::that_detached(trimmed).map_err(|e| format!("在资源管理器中打开失败: {}", e))
}
