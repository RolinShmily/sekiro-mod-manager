use std::path::{Path, PathBuf};

/// Resolves the staging directory from a `--staging` argument or detects an existing
/// `staging/` folder walking up the tree. When `create` is true and nothing is found,
/// a default `./staging` directory is created.
pub fn resolve_staging_dir(staging_arg: Option<&Path>, create: bool) -> Result<PathBuf, String> {
    if let Some(path) = staging_arg {
        if create {
            std::fs::create_dir_all(path)
                .map_err(|e| format!("Failed to create staging directory {}: {}", path.display(), e))?;
        } else if !path.exists() {
            return Err(format!(
                "Specified staging directory does not exist: {}",
                path.display()
            ));
        }
        return Ok(path.to_path_buf());
    }

    for candidate in staging_candidates() {
        if candidate.exists() && candidate.is_dir() {
            return Ok(candidate);
        }
    }

    if create {
        let default_dir = PathBuf::from("staging");
        std::fs::create_dir_all(&default_dir)
            .map_err(|e| format!("Failed to create staging directory: {}", e))?;
        Ok(default_dir)
    } else {
        Err(
            "Staging directory not found. Please provide `--staging <path>` or create a `staging/` directory."
                .to_string(),
        )
    }
}

/// Candidate staging directory locations, walking up relative to the current working directory.
fn staging_candidates() -> [PathBuf; 4] {
    [
        PathBuf::from("staging"),
        PathBuf::from("../staging"),
        PathBuf::from("../../staging"),
        PathBuf::from("../../../staging"),
    ]
}

/// Identifies the Sekiro game root: explicit argument, current directory if it contains
/// `sekiro.exe`, or a common Steam library install path. Falls back to the current directory.
pub fn resolve_game_dir(game_dir_arg: Option<&Path>) -> PathBuf {
    if let Some(path) = game_dir_arg {
        return path.to_path_buf();
    }

    if Path::new("sekiro.exe").is_file() {
        return PathBuf::from(".");
    }

    for candidate in steam_install_candidates() {
        let p = PathBuf::from(candidate);
        if p.exists() && p.join("sekiro.exe").is_file() {
            return p;
        }
    }

    PathBuf::from(".")
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

    #[test]
    fn test_resolve_staging_dir_fallback() {
        let temp = tempfile::tempdir().unwrap();
        let custom_staging = temp.path().join("my_staging");
        std::fs::create_dir_all(&custom_staging).unwrap();

        let resolved = resolve_staging_dir(Some(&custom_staging), false);
        assert!(resolved.is_ok());
        assert_eq!(resolved.unwrap(), custom_staging);

        let created = resolve_staging_dir(Some(&custom_staging), true);
        assert!(created.is_ok());
    }

    #[test]
    fn test_resolve_creates_default_when_requested() {
        let temp = tempfile::tempdir().unwrap();
        let old_cwd = std::env::current_dir().unwrap();
        std::env::set_current_dir(temp.path()).unwrap();

        // Nothing exists yet -> non-creating resolution must fail
        let missing = resolve_staging_dir(None, false);
        assert!(missing.is_err());

        // Creating resolution materializes ./staging and succeeds
        let created = resolve_staging_dir(None, true).unwrap();
        assert_eq!(created, PathBuf::from("staging"));
        assert!(created.is_dir());

        std::env::set_current_dir(old_cwd).unwrap();
    }
}