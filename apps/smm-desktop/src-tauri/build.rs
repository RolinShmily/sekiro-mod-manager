fn main() {
    let manifest_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap_or_default();
    let manifest_path = std::path::Path::new(&manifest_dir);

    // 1. Ensure dist/index.html exists for tauri::generate_context!() if frontend isn't built yet
    if let Some(desktop_dir) = manifest_path.parent() {
        let dist_dir = desktop_dir.join("dist");
        let index_html = dist_dir.join("index.html");
        if !index_html.exists() {
            let _ = std::fs::create_dir_all(&dist_dir);
            let _ = std::fs::write(&index_html, "<!DOCTYPE html><html><body>SMM</body></html>");
        }
    }

    // 2. Ensure bundled resources exist so tauri-build does not fail during cargo test / debug / fresh builds
    // Go 3 levels up to workspace root: src-tauri -> smm-desktop -> apps -> root
    if let Some(root) = manifest_path.ancestors().nth(3) {
        let release_dir = root.join("target").join("release");
        let smm_exe = release_dir.join("smm.exe");
        if !smm_exe.exists() {
            let _ = std::fs::create_dir_all(&release_dir);
            let _ = std::fs::write(&smm_exe, b"MZ\0\0SMM_STUB");
        }
    }

    tauri_build::build()
}
