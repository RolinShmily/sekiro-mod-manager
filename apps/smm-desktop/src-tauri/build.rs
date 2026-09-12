fn main() {
    // Ensure bundled resources exist so tauri-build does not fail during cargo test / debug / fresh builds
    let manifest_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap_or_default();
    let manifest_path = std::path::Path::new(&manifest_dir);
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
