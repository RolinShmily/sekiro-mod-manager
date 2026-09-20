use std::path::PathBuf;

use rfd::AsyncFileDialog;

/// Builds a native file dialog preconfigured with title and default directory.
fn build_dialog(title: Option<String>, default_path: Option<String>) -> AsyncFileDialog {
    let mut dialog = AsyncFileDialog::new();
    if let Some(ref t) = title {
        dialog = dialog.set_title(t);
    }
    if let Some(ref p) = default_path {
        if !p.trim().is_empty() {
            let path = PathBuf::from(p);
            if path.exists() {
                dialog = dialog.set_directory(&path);
            }
        }
    }
    dialog
}

/// Applies the optional filter (name + extensions) to a dialog.
fn apply_filter(dialog: AsyncFileDialog, filter_name: Option<String>, extensions: Option<Vec<String>>) -> AsyncFileDialog {
    if let (Some(name), Some(exts)) = (filter_name, extensions) {
        let ext_refs: Vec<&str> = exts.iter().map(|s| s.as_str()).collect();
        dialog.add_filter(&name, &ext_refs)
    } else {
        dialog
    }
}

/// Opens native Windows file explorer folder selection dialog.
#[tauri::command]
pub async fn pick_folder(
    title: Option<String>,
    default_path: Option<String>,
) -> Result<Option<String>, String> {
    let folder = build_dialog(title, default_path).pick_folder().await;
    Ok(folder.map(|h| h.path().to_string_lossy().to_string()))
}

/// Opens native Windows file explorer file selection dialog.
#[tauri::command]
pub async fn pick_file(
    title: Option<String>,
    default_path: Option<String>,
    filter_name: Option<String>,
    extensions: Option<Vec<String>>,
) -> Result<Option<String>, String> {
    let dialog = apply_filter(build_dialog(title, default_path), filter_name, extensions);
    let file = dialog.pick_file().await;
    Ok(file.map(|h| h.path().to_string_lossy().to_string()))
}

/// Opens native Windows file explorer multiple files selection dialog.
#[tauri::command]
pub async fn pick_files(
    title: Option<String>,
    default_path: Option<String>,
    filter_name: Option<String>,
    extensions: Option<Vec<String>>,
) -> Result<Option<Vec<String>>, String> {
    let dialog = apply_filter(build_dialog(title, default_path), filter_name, extensions);
    let files = dialog.pick_files().await;
    Ok(files.map(|list| {
        list.into_iter()
            .map(|h| h.path().to_string_lossy().to_string())
            .collect()
    }))
}