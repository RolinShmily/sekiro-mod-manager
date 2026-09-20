use std::path::{Path, PathBuf};

/// Resolves the actual game `mods` directory: accepts either the game root or a
/// path that already points at a `mods` folder and normalizes both to the same target.
pub fn target_mods_dir(game_dir: &Path) -> PathBuf {
    if game_dir
        .file_name()
        .map(|n| n.to_string_lossy().eq_ignore_ascii_case("mods"))
        .unwrap_or(false)
    {
        game_dir.to_path_buf()
    } else {
        game_dir.join("mods")
    }
}

/// Candidate staging directory locations, walking up relative to the working directory.
pub fn staging_candidates() -> [PathBuf; 6] {
    [
        PathBuf::from("staging"),
        PathBuf::from("../staging"),
        PathBuf::from("../../staging"),
        PathBuf::from("../../../staging"),
        PathBuf::from("mods_staging"),
        PathBuf::from("../mods_staging"),
    ]
}

/// Common Steam library paths where Sekiro is typically installed.
pub fn steam_install_candidates() -> [&'static str; 4] {
    [
        r"C:\Program Files (x86)\Steam\steamapps\common\Sekiro",
        r"C:\Program Files\Steam\steamapps\common\Sekiro",
        r"D:\SteamLibrary\steamapps\common\Sekiro",
        r"E:\SteamLibrary\steamapps\common\Sekiro",
    ]
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;

    #[test]
    fn test_target_mods_dir_normalization() {
        assert_eq!(
            target_mods_dir(Path::new(r"C:\Games\Sekiro")),
            PathBuf::from(r"C:\Games\Sekiro\mods")
        );
        assert_eq!(
            target_mods_dir(Path::new(r"C:\Games\Sekiro\mods")),
            PathBuf::from(r"C:\Games\Sekiro\mods")
        );
        assert_eq!(
            target_mods_dir(Path::new(r"C:\Games\Sekiro\MODS")),
            PathBuf::from(r"C:\Games\Sekiro\MODS")
        );
    }
}