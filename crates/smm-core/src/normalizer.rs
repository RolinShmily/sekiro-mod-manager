use std::path::{Path, PathBuf};
use walkdir::WalkDir;

use crate::error::{Result, SmmError};
use crate::types::{AssetCategory, AssetEntry};

/// Canonical top-level asset directories in Sekiro Mod Engine.
pub const CANONICAL_DIRS: &[&str] = &[
    "parts", "chr", "param", "sfx", "sound", "msg", "menu", "font", "mtd", "event", "map", "action",
    "cutscene", "script",
];

/// Canonical root files that belong in the game root (alongside sekiro.exe).
pub const CANONICAL_ROOT_FILES: &[&str] = &["dinput8.dll", "modengine.ini"];

/// Characteristic file signatures and their canonical directory mapping.
pub const FILE_SIGNATURES: &[(&str, &str, AssetCategory)] = &[
    (".partsbnd.dcx", "parts", AssetCategory::Parts),
    (".chrbnd.dcx", "chr", AssetCategory::Chr),
    (".anibnd.dcx", "chr", AssetCategory::Chr),
    (".texbnd.dcx", "chr", AssetCategory::Chr),
    (".ffxbnd.dcx", "sfx", AssetCategory::Sfx),
    (".parambnd.dcx", "param/gameparam", AssetCategory::Param),
    (".fsb", "sound", AssetCategory::Sound),
    (".bank", "sound", AssetCategory::Sound),
    (".fev", "sound", AssetCategory::Sound),
    (".emevd.dcx", "event", AssetCategory::Event),
    (".tpf.dcx", "menu", AssetCategory::Menu),
    (".menubnd.dcx", "menu", AssetCategory::Menu),
    (".gfx", "font", AssetCategory::Font),
    (".mtd", "mtd", AssetCategory::Mtd),
    (".mtdbnd.dcx", "mtd", AssetCategory::Mtd),
    (".msb.dcx", "map", AssetCategory::Map),
    (".msgbnd.dcx", "msg", AssetCategory::Msg),
    (".lua", "script", AssetCategory::Script),
    (".fxr", "sfx", AssetCategory::Sfx),
    (".hkx", "action", AssetCategory::Other),
    (".bk2", "cutscene", AssetCategory::Cutscene),
];

/// Result of directory normalization.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct NormalizationResult {
    /// Identified canonical asset root directory
    pub canonical_root: PathBuf,
    /// Extracted game asset entries
    pub assets: Vec<AssetEntry>,
    /// Files detected but safely excluded (e.g. README, mod.json, install notes)
    pub ignored_files: Vec<PathBuf>,
}

/// Heuristic normalizer for unpacked mod packages.
pub struct Normalizer;

impl Normalizer {
    /// Inspects an unpacked mod directory, identifies its canonical root,
    /// and collects all valid Sekiro game assets with normalized paths.
    pub fn normalize_directory(base_path: &Path) -> Result<NormalizationResult> {
        if !base_path.exists() {
            return Err(SmmError::NormalizationError {
                path: base_path.to_path_buf(),
                message: "Specified path does not exist".to_string(),
            });
        }

        let canonical_root = Self::find_canonical_root(base_path)?;
        let mut assets = Vec::new();
        let mut ignored_files = Vec::new();

        for entry in WalkDir::new(&canonical_root).into_iter().filter_map(|e| e.ok()) {
            let path = entry.path();
            if path.is_dir() {
                continue;
            }

            let file_name = match path.file_name().and_then(|f| f.to_str()) {
                Some(name) => name,
                None => continue,
            };

            // Check if file should be ignored (metadata, docs, screenshots)
            if Self::is_ignored_file(file_name, path, &canonical_root) {
                ignored_files.push(path.to_path_buf());
                continue;
            }

            let rel_path = match path.strip_prefix(&canonical_root) {
                Ok(p) => p,
                Err(_) => continue,
            };

            let mut normalized_rel_str = rel_path
                .components()
                .map(|c| c.as_os_str().to_string_lossy())
                .collect::<Vec<_>>()
                .join("/");

            // Heuristic fix for loose files matching signatures: place into canonical dir
            if !normalized_rel_str.contains('/')
                && !CANONICAL_ROOT_FILES.contains(&normalized_rel_str.as_str())
            {
                let lower = normalized_rel_str.to_ascii_lowercase();
                let mut mapped = false;
                for (sig, canon_dir, _) in FILE_SIGNATURES {
                    if lower.ends_with(sig) {
                        normalized_rel_str = format!("{canon_dir}/{normalized_rel_str}");
                        mapped = true;
                        break;
                    }
                }
                if !mapped {
                    if (lower.starts_with("wp_") || lower.starts_with("am_") || lower.starts_with("bd_") || lower.starts_with("fc_") || lower.starts_with("lg_"))
                        && (lower.ends_with(".dcx") || lower.ends_with(".partsbnd") || lower.ends_with(".tpf")) {
                        normalized_rel_str = format!("parts/{normalized_rel_str}");
                    } else if lower.starts_with('c') && lower.chars().skip(1).take(4).all(|c| c.is_ascii_digit())
                        && (lower.ends_with(".dcx") || lower.ends_with(".chrbnd") || lower.ends_with(".texbnd") || lower.ends_with(".anibnd")) {
                        normalized_rel_str = format!("chr/{normalized_rel_str}");
                    } else if (lower.starts_with("sfx") || lower.starts_with("f000"))
                        && (lower.ends_with(".dcx") || lower.ends_with(".ffxbnd") || lower.ends_with(".fxr")) {
                        normalized_rel_str = format!("sfx/{normalized_rel_str}");
                    }
                }
            }

            let category = Self::classify_asset(&normalized_rel_str);
            let metadata = std::fs::metadata(path)?;

            assets.push(AssetEntry::new(
                normalized_rel_str,
                path.to_path_buf(),
                metadata.len(),
                category,
            ));
        }

        // Sort assets deterministically by relative path
        assets.sort_by(|a, b| a.relative_path.cmp(&b.relative_path));

        if assets.is_empty() {
            return Err(SmmError::NoAssetsFound(base_path.to_path_buf()));
        }

        Ok(NormalizationResult {
            canonical_root,
            assets,
            ignored_files,
        })
    }

    /// Finds the canonical asset root by inspecting canonical directories and signatures.
    pub fn find_canonical_root(base_path: &Path) -> Result<PathBuf> {
        // Step 1: Check if base_path directly contains canonical dirs or root loader files
        if Self::contains_canonical_anchor(base_path) {
            return Ok(base_path.to_path_buf());
        }

        // Step 2: Search subdirectories for canonical anchors
        let mut best_candidate: Option<(PathBuf, usize, usize)> = None; // (path, anchor_count, depth)

        for entry in WalkDir::new(base_path)
            .min_depth(1)
            .max_depth(6)
            .into_iter()
            .filter_map(|e| e.ok())
        {
            if !entry.file_type().is_dir() {
                continue;
            }

            let count = Self::count_canonical_anchors(entry.path());
            if count > 0 {
                let depth = entry.depth();
                match &best_candidate {
                    None => {
                        best_candidate = Some((entry.path().to_path_buf(), count, depth));
                    }
                    Some((_, best_count, best_depth)) => {
                        // Prefer candidate with more anchors, or shallower if equal
                        if count > *best_count || (count == *best_count && depth < *best_depth) {
                            best_candidate = Some((entry.path().to_path_buf(), count, depth));
                        }
                    }
                }
            }
        }

        if let Some((candidate, _, _)) = best_candidate {
            return Ok(candidate);
        }

        // Step 3: Check if there is a directory containing loose files matching Sekiro signatures
        for entry in WalkDir::new(base_path)
            .min_depth(0)
            .max_depth(4)
            .into_iter()
            .filter_map(|e| e.ok())
        {
            if entry.file_type().is_dir() && Self::contains_signature_files(entry.path()) {
                return Ok(entry.path().to_path_buf());
            }
        }

        Err(SmmError::NormalizationError {
            path: base_path.to_path_buf(),
            message: "No canonical Sekiro directories (parts/, chr/, etc.) or signature files found"
                .to_string(),
        })
    }

    /// Checks if a directory directly contains any canonical subdirectory or loader file.
    fn contains_canonical_anchor(dir: &Path) -> bool {
        Self::count_canonical_anchors(dir) > 0
    }

    /// Counts how many canonical directories or root loader files exist directly under `dir`.
    fn count_canonical_anchors(dir: &Path) -> usize {
        let entries = match std::fs::read_dir(dir) {
            Ok(iter) => iter,
            Err(_) => return 0,
        };

        let mut count = 0;
        for item in entries.filter_map(|e| e.ok()) {
            let name = item.file_name().to_string_lossy().to_ascii_lowercase();
            let is_dir = item.file_type().map(|t| t.is_dir()).unwrap_or(false);

            if is_dir {
                if CANONICAL_DIRS.iter().any(|&c| c == name) {
                    count += 1;
                }
            } else {
                if CANONICAL_ROOT_FILES.iter().any(|&c| c == name) {
                    count += 1;
                }
            }
        }
        count
    }

    /// Checks if a directory directly contains loose files matching Sekiro signatures.
    fn contains_signature_files(dir: &Path) -> bool {
        let entries = match std::fs::read_dir(dir) {
            Ok(iter) => iter,
            Err(_) => return false,
        };

        for item in entries.filter_map(|e| e.ok()) {
            if item.file_type().map(|t| t.is_file()).unwrap_or(false) {
                let name = item.file_name().to_string_lossy().to_ascii_lowercase();
                if FILE_SIGNATURES.iter().any(|(sig, _, _)| name.ends_with(sig)) {
                    return true;
                }
                if (name.starts_with("wp_") || name.starts_with("am_") || name.starts_with("bd_") || name.starts_with("fc_") || name.starts_with("lg_"))
                    && (name.ends_with(".dcx") || name.ends_with(".partsbnd") || name.ends_with(".tpf")) {
                    return true;
                }
                if name.starts_with('c') && name.chars().skip(1).take(4).all(|c| c.is_ascii_digit())
                    && (name.ends_with(".dcx") || name.ends_with(".chrbnd") || name.ends_with(".texbnd") || name.ends_with(".anibnd")) {
                    return true;
                }
                if (name.starts_with("sfx") || name.starts_with("f000"))
                    && (name.ends_with(".dcx") || name.ends_with(".ffxbnd") || name.ends_with(".fxr")) {
                    return true;
                }
            }
        }
        false
    }

    /// Classifies an asset by its normalized relative path.
    pub fn classify_asset(rel_path: &str) -> AssetCategory {
        let lower = rel_path.to_ascii_lowercase();

        if lower == "dinput8.dll" || lower == "modengine.ini" {
            return AssetCategory::Loader;
        }
        if lower.starts_with("parts/") {
            return AssetCategory::Parts;
        }
        if lower.starts_with("chr/") {
            return AssetCategory::Chr;
        }
        if lower.starts_with("param/") {
            return AssetCategory::Param;
        }
        if lower.starts_with("sfx/") || lower.ends_with(".ffxbnd.dcx") || lower.ends_with(".fxr") {
            return AssetCategory::Sfx;
        }
        if lower.starts_with("sound/") {
            return AssetCategory::Sound;
        }
        if lower.starts_with("msg/") {
            return AssetCategory::Msg;
        }
        if lower.starts_with("menu/") {
            return AssetCategory::Menu;
        }
        if lower.starts_with("font/") {
            return AssetCategory::Font;
        }
        if lower.starts_with("mtd/") {
            return AssetCategory::Mtd;
        }
        if lower.starts_with("event/") {
            return AssetCategory::Event;
        }
        if lower.starts_with("map/") {
            return AssetCategory::Map;
        }
        if lower.starts_with("action/") {
            return AssetCategory::Other;
        }
        if lower.starts_with("cutscene/") {
            return AssetCategory::Cutscene;
        }
        if lower.starts_with("script/") {
            return AssetCategory::Script;
        }

        // Extension-based fallback
        for (sig, _, cat) in FILE_SIGNATURES {
            if lower.ends_with(sig) {
                return *cat;
            }
        }

        AssetCategory::Other
    }

    /// Checks if a file is an auxiliary/documentation file rather than a game asset.
    fn is_ignored_file(file_name: &str, path: &Path, root: &Path) -> bool {
        let lower = file_name.to_ascii_lowercase();

        // Hidden files or files in hidden folders relative to root (e.g. .smm_source/, .smm_mod.json)
        if file_name.starts_with('.') {
            return true;
        }

        // Archive files should not be deployed as game assets
        if lower.ends_with(".zip")
            || lower.ends_with(".7z")
            || lower.ends_with(".rar")
            || lower.ends_with(".tar")
            || lower.ends_with(".gz")
        {
            return true;
        }

        if let Ok(rel) = path.strip_prefix(root) {
            if rel.components().any(|c| {
                let s = c.as_os_str().to_string_lossy();
                s.starts_with('.') && s != "." && s != ".."
            }) {
                return true;
            }
        }

        // Exact metadata and license files
        if lower == "mod.json"
            || lower == ".smm_mod.json"
            || lower == "desktop.ini"
            || lower == ".ds_store"
            || lower == "thumbs.db"
        {
            return true;
        }

        // Markdown or text documentation
        if lower.starts_with("readme")
            || lower.starts_with("license")
            || lower.starts_with("changelog")
            || lower.ends_with(".md")
            || lower.ends_with(".txt")
        {
            return true;
        }

        // Images placed directly in root (previews/screenshots)
        if (lower.ends_with(".jpg") || lower.ends_with(".png") || lower.ends_with(".jpeg"))
            && path.parent() == Some(root)
        {
            return true;
        }

        false
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs::{create_dir_all, File};
    use std::io::Write;
    use tempfile::tempdir;

    #[test]
    fn test_classify_asset() {
        assert_eq!(Normalizer::classify_asset("parts/wp_a_0300.partsbnd.dcx"), AssetCategory::Parts);
        assert_eq!(Normalizer::classify_asset("chr/c5110.chrbnd.dcx"), AssetCategory::Chr);
        assert_eq!(Normalizer::classify_asset("param/gameparam/gameparam.parambnd.dcx"), AssetCategory::Param);
        assert_eq!(Normalizer::classify_asset("sound/fdp_main.fsb"), AssetCategory::Sound);
        assert_eq!(Normalizer::classify_asset("msg/engus/item.msgbnd.dcx"), AssetCategory::Msg);
        assert_eq!(Normalizer::classify_asset("menu/hi/01_common.tpf.dcx"), AssetCategory::Menu);
        assert_eq!(Normalizer::classify_asset("font/font_ps4.gfx"), AssetCategory::Font);
        assert_eq!(Normalizer::classify_asset("dinput8.dll"), AssetCategory::Loader);
        assert_eq!(Normalizer::classify_asset("modengine.ini"), AssetCategory::Loader);
    }

    #[test]
    fn test_nested_directory_normalization() {
        let tmp = tempdir().unwrap();
        let base = tmp.path();

        // Create a messy nested folder: base/NestedMod_v1/Sekiro/mods/parts/wp_a_0300.partsbnd.dcx
        let nested_parts = base.join("NestedMod_v1").join("Sekiro").join("mods").join("parts");
        create_dir_all(&nested_parts).unwrap();
        File::create(nested_parts.join("wp_a_0300.partsbnd.dcx"))
            .unwrap()
            .write_all(b"mock dcx")
            .unwrap();

        // Extra non-game docs
        let doc_dir = base.join("NestedMod_v1").join("Doc");
        create_dir_all(&doc_dir).unwrap();
        File::create(doc_dir.join("ReadMe.txt"))
            .unwrap()
            .write_all(b"how to install")
            .unwrap();

        let norm = Normalizer::normalize_directory(base).unwrap();
        assert_eq!(norm.assets.len(), 1);
        assert_eq!(norm.assets[0].relative_path, "parts/wp_a_0300.partsbnd.dcx");
        assert_eq!(norm.assets[0].category, AssetCategory::Parts);
        assert!(norm.assets[0].is_exclusive_slot);
    }
}

