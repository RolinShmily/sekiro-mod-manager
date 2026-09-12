use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

use crate::error::{Result, SmmError};

/// Supported compressed archive formats.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ArchiveFormat {
    Zip,
    SevenZip,
    Rar,
}

impl ArchiveFormat {
    pub fn extension(&self) -> &'static str {
        match self {
            ArchiveFormat::Zip => "zip",
            ArchiveFormat::SevenZip => "7z",
            ArchiveFormat::Rar => "rar",
        }
    }
}

/// Detects archive format from file extension.
pub fn detect_archive_format(path: &Path) -> Option<ArchiveFormat> {
    let ext = path.extension()?.to_str()?.to_ascii_lowercase();
    match ext.as_str() {
        "zip" => Some(ArchiveFormat::Zip),
        "7z" => Some(ArchiveFormat::SevenZip),
        "rar" => Some(ArchiveFormat::Rar),
        _ => None,
    }
}

/// Checks if a file has a supported archive extension (.zip, .7z, .rar).
pub fn is_supported_archive(path: &Path) -> bool {
    detect_archive_format(path).is_some()
}

/// Locates external `7z` or `7za` command line executable in PATH or standard system locations.
pub fn find_external_7z() -> Option<PathBuf> {
    let binary_names = if cfg!(windows) {
        vec!["7z.exe", "7za.exe", "7z", "7za"]
    } else {
        vec!["7z", "7za", "7zr"]
    };

    // 1. Search in PATH
    if let Some(path_var) = std::env::var_os("PATH") {
        for dir in std::env::split_paths(&path_var) {
            for bin in &binary_names {
                let candidate = dir.join(bin);
                if candidate.is_file() {
                    return Some(candidate);
                }
            }
        }
    }

    // 2. Search standard Windows installation locations
    #[cfg(windows)]
    {
        let mut standard_dirs: Vec<PathBuf> = Vec::new();

        if let Ok(pf) = std::env::var("ProgramFiles") {
            standard_dirs.push(PathBuf::from(pf).join("7-Zip"));
        }
        if let Ok(pf86) = std::env::var("ProgramFiles(x86)") {
            standard_dirs.push(PathBuf::from(pf86).join("7-Zip"));
        }
        if let Ok(local_app_data) = std::env::var("LOCALAPPDATA") {
            standard_dirs.push(PathBuf::from(local_app_data).join("Programs").join("7-Zip"));
        }

        // Hardcoded common fallbacks
        standard_dirs.push(PathBuf::from(r"C:\Program Files\7-Zip"));
        standard_dirs.push(PathBuf::from(r"C:\Program Files (x86)\7-Zip"));

        for dir in standard_dirs {
            for bin in &binary_names {
                let candidate = dir.join(bin);
                if candidate.is_file() {
                    return Some(candidate);
                }
            }
        }
    }

    // 3. Search standard Unix/Linux locations
    #[cfg(not(windows))]
    {
        let unix_candidates = [
            "/usr/bin/7z",
            "/usr/local/bin/7z",
            "/usr/bin/7za",
            "/usr/local/bin/7za",
            "/usr/bin/7zr",
        ];
        for candidate in unix_candidates {
            let p = PathBuf::from(candidate);
            if p.is_file() {
                return Some(p);
            }
        }
    }

    None
}

/// Locates external `unrar` executable in PATH or standard system locations.
pub fn find_external_unrar() -> Option<PathBuf> {
    let binary_names = if cfg!(windows) {
        vec!["unrar.exe", "rar.exe", "unrar", "rar"]
    } else {
        vec!["unrar", "rar"]
    };

    // 1. Search in PATH
    if let Some(path_var) = std::env::var_os("PATH") {
        for dir in std::env::split_paths(&path_var) {
            for bin in &binary_names {
                let candidate = dir.join(bin);
                if candidate.is_file() {
                    return Some(candidate);
                }
            }
        }
    }

    // 2. Search standard Windows WinRAR directories
    #[cfg(windows)]
    {
        let standard_paths = [
            r"C:\Program Files\WinRAR\UnRAR.exe",
            r"C:\Program Files\WinRAR\WinRAR.exe",
            r"C:\Program Files (x86)\WinRAR\UnRAR.exe",
            r"C:\Program Files (x86)\WinRAR\WinRAR.exe",
        ];
        for p in standard_paths {
            let pb = PathBuf::from(p);
            if pb.is_file() {
                return Some(pb);
            }
        }
    }

    None
}

/// Safely decompresses a `.zip` archive into the target directory, preventing path traversal.
pub fn extract_zip(zip_path: &Path, dest_dir: &Path) -> Result<()> {
    let file = std::fs::File::open(zip_path)?;
    let mut archive = zip::ZipArchive::new(file)?;

    for i in 0..archive.len() {
        let mut entry = archive.by_index(i)?;
        let enclosed = match entry.enclosed_name() {
            Some(path) => path.to_owned(),
            None => continue, // Discard unsafe path traversal attempts
        };

        let out_path = dest_dir.join(enclosed);
        if entry.is_dir() {
            std::fs::create_dir_all(&out_path)?;
        } else {
            if let Some(parent) = out_path.parent() {
                std::fs::create_dir_all(parent)?;
            }
            let mut outfile = std::fs::File::create(&out_path)?;
            std::io::copy(&mut entry, &mut outfile)?;
        }
    }

    Ok(())
}

/// Pure-Rust decompression of `.7z` archive using `sevenz-rust` with path traversal guards.
pub fn extract_7z_native(archive_path: &Path, dest_dir: &Path) -> Result<()> {
    sevenz_rust::decompress_file_with_extract_fn(archive_path, dest_dir, |entry, reader, _dest| {
        let entry_name = entry.name();

        // Prevent path traversal attacks (leading slash or '..')
        let clean_path = Path::new(entry_name);
        if clean_path.is_absolute()
            || clean_path
                .components()
                .any(|c| matches!(c, std::path::Component::ParentDir))
        {
            return Ok(true);
        }

        let target_path = dest_dir.join(clean_path);
        if entry.is_directory() {
            std::fs::create_dir_all(&target_path).map_err(sevenz_rust::Error::io)?;
        } else {
            if let Some(parent) = target_path.parent() {
                std::fs::create_dir_all(parent).map_err(sevenz_rust::Error::io)?;
            }
            let mut file = std::fs::File::create(&target_path)
                .map_err(|e| sevenz_rust::Error::io_msg(e, target_path.to_string_lossy().to_string()))?;
            if entry.size() > 0 {
                std::io::copy(reader, &mut file).map_err(sevenz_rust::Error::io)?;
            }
        }
        Ok(true)
    })?;

    Ok(())
}

/// Decompresses an archive using an external 7z binary command line.
fn extract_with_external_7z(sevenz_bin: &Path, archive_path: &Path, dest_dir: &Path) -> Result<()> {
    let dest_arg = format!("-o{}", dest_dir.display());
    let output = Command::new(sevenz_bin)
        .arg("x")
        .arg("-y")
        .arg(&dest_arg)
        .arg(archive_path)
        .output()
        .map_err(|e| {
            SmmError::ExtractionError(format!(
                "Failed to execute external 7z ({}): {}",
                sevenz_bin.display(),
                e
            ))
        })?;

    if output.status.success() {
        Ok(())
    } else {
        let stderr = String::from_utf8_lossy(&output.stderr);
        let stdout = String::from_utf8_lossy(&output.stdout);
        let msg = if !stderr.trim().is_empty() {
            stderr.trim().to_string()
        } else {
            stdout.trim().to_string()
        };
        Err(SmmError::ExtractionError(format!(
            "External 7z decompression failed: {}",
            msg
        )))
    }
}

/// Decompresses a `.7z` archive:
/// 1. Attempts pure-Rust decompression via `sevenz-rust`.
/// 2. If pure-Rust fails (e.g. complex password, multivolume, unsupported method),
///    falls back to system external `7z` / `7za` command.
pub fn extract_7z(archive_path: &Path, dest_dir: &Path) -> Result<()> {
    match extract_7z_native(archive_path, dest_dir) {
        Ok(()) => Ok(()),
        Err(native_err) => {
            // Attempt fallback to external 7-Zip
            if let Some(sevenz_bin) = find_external_7z() {
                if extract_with_external_7z(&sevenz_bin, archive_path, dest_dir).is_ok() {
                    return Ok(());
                }
            }
            Err(native_err)
        }
    }
}

/// Decompresses a `.rar` archive:
/// 1. Attempts native decompression via `unrar` crate.
/// 2. If native fails, falls back to external `7z` or `unrar` if installed.
pub fn extract_rar(archive_path: &Path, dest_dir: &Path) -> Result<()> {
    fs::create_dir_all(dest_dir)?;

    // 1. Try native unrar
    if let Some(archive_path_str) = archive_path.to_str() {
        if let Ok(mut archive) = unrar::Archive::new(archive_path_str).open_for_processing() {
            let mut extracted_any = false;
            let mut process_result = Ok(());

            loop {
                match archive.read_header() {
                    Ok(Some(header)) => {
                        let is_file = header.entry().is_file();
                        if is_file {
                            match header.extract_to(dest_dir) {
                                Ok(next_archive) => {
                                    archive = next_archive;
                                    extracted_any = true;
                                }
                                Err(e) => {
                                    process_result = Err(SmmError::ExtractionError(format!("RAR extraction failed: {}", e)));
                                    break;
                                }
                            }
                        } else {
                            match header.skip() {
                                Ok(next_archive) => {
                                    archive = next_archive;
                                }
                                Err(e) => {
                                    process_result = Err(SmmError::ExtractionError(format!("RAR skip error: {}", e)));
                                    break;
                                }
                            }
                        }
                    }
                    Ok(None) => break,
                    Err(e) => {
                        process_result = Err(SmmError::ExtractionError(format!("RAR read error: {}", e)));
                        break;
                    }
                }
            }

            if process_result.is_ok() && extracted_any {
                return Ok(());
            }
        }
    }

    // 2. Try external 7z first
    if let Some(sevenz_bin) = find_external_7z() {
        return extract_with_external_7z(&sevenz_bin, archive_path, dest_dir);
    }

    // 3. Try external unrar
    if let Some(unrar_bin) = find_external_unrar() {
        let output = Command::new(&unrar_bin)
            .arg("x")
            .arg("-y")
            .arg(archive_path)
            .arg(format!("{}/", dest_dir.display()))
            .output()
            .map_err(|e| {
                SmmError::ExtractionError(format!(
                    "Failed to execute unrar ({}): {}",
                    unrar_bin.display(),
                    e
                ))
            })?;

        if output.status.success() {
            return Ok(());
        } else {
            let stderr = String::from_utf8_lossy(&output.stderr);
            return Err(SmmError::ExtractionError(format!(
                "External unrar decompression failed: {}",
                stderr.trim()
            )));
        }
    }

    // 4. Neither tool is available
    Err(SmmError::ExtractionError(
        "Failed to decompress RAR archive. Please check file integrity or install 7-Zip (https://www.7-zip.org).".to_string(),
    ))
}

/// Unified archive decompression router.
/// Supports `.zip`, `.7z`, and `.rar` (via external 7z/unrar).
pub fn extract_archive(archive_path: &Path, dest_dir: &Path) -> Result<()> {
    if !archive_path.exists() {
        return Err(SmmError::Io(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!("Archive file not found: {}", archive_path.display()),
        )));
    }

    let format = detect_archive_format(archive_path).ok_or_else(|| {
        SmmError::UnsupportedArchive(archive_path.to_path_buf())
    })?;

    std::fs::create_dir_all(dest_dir)?;

    match format {
        ArchiveFormat::Zip => extract_zip(archive_path, dest_dir),
        ArchiveFormat::SevenZip => extract_7z(archive_path, dest_dir),
        ArchiveFormat::Rar => extract_rar(archive_path, dest_dir),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    #[test]
    fn test_detect_archive_format() {
        assert_eq!(detect_archive_format(Path::new("mod.zip")), Some(ArchiveFormat::Zip));
        assert_eq!(detect_archive_format(Path::new("MOD.ZIP")), Some(ArchiveFormat::Zip));
        assert_eq!(detect_archive_format(Path::new("mod.7z")), Some(ArchiveFormat::SevenZip));
        assert_eq!(detect_archive_format(Path::new("mod.rar")), Some(ArchiveFormat::Rar));
        assert_eq!(detect_archive_format(Path::new("mod.tar.gz")), None);
        assert_eq!(detect_archive_format(Path::new("mod.txt")), None);
    }

    #[test]
    fn test_extract_7z_native_and_roundtrip() {
        let tmp = tempdir().unwrap();
        let src_dir = tmp.path().join("source");
        std::fs::create_dir_all(src_dir.join("parts")).unwrap();
        std::fs::write(src_dir.join("parts").join("wp_a_0300.partsbnd.dcx"), b"katana 7z data").unwrap();
        std::fs::write(src_dir.join("README.md"), b"# 7z Mod Readme").unwrap();

        let archive_7z = tmp.path().join("test_mod.7z");

        // Compress using sevenz-rust writer
        sevenz_rust::compress_to_path(&src_dir, &archive_7z).expect("compress 7z failed");
        assert!(archive_7z.exists());

        // Now decompress using our extractor
        let dest_dir = tmp.path().join("unpacked");
        extract_archive(&archive_7z, &dest_dir).expect("extract_archive 7z failed");

        // Verify contents
        let extracted_file = dest_dir.join("parts").join("wp_a_0300.partsbnd.dcx");
        assert!(extracted_file.exists());
        assert_eq!(std::fs::read(&extracted_file).unwrap(), b"katana 7z data");

        let extracted_readme = dest_dir.join("README.md");
        assert!(extracted_readme.exists());
        assert_eq!(std::fs::read(&extracted_readme).unwrap(), b"# 7z Mod Readme");
    }

    #[test]
    fn test_find_external_7z_detector() {
        // Just verify it doesn't crash
        let external = find_external_7z();
        if let Some(path) = external {
            assert!(path.is_file());
        }
    }
}
