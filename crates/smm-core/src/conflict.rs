use std::collections::BTreeMap;

use crate::types::{
    is_critical_asset, is_slot_asset, AssetEntry, ConflictRecord, ConflictReport, ConflictSeverity,
    ModInfo,
};

/// Conflict detection and collision analysis engine.
pub struct ConflictEngine;

impl ConflictEngine {
    /// Evaluates conflicts across a list of enabled mods and their associated asset entries.
    /// Mods should preferably have priorities assigned (lower priority number = higher precedence).
    pub fn scan_conflicts<'a>(
        mods: &'a [(ModInfo, Vec<AssetEntry>)],
    ) -> ConflictReport {
        // Filter enabled mods and sort by priority (ascending: 1 beats 2)
        let mut enabled_mods: Vec<&(ModInfo, Vec<AssetEntry>)> =
            mods.iter().filter(|(info, _)| info.enabled).collect();

        enabled_mods.sort_by(|a, b| {
            a.0.priority
                .cmp(&b.0.priority)
                .then_with(|| a.0.id.cmp(&b.0.id))
        });

        // Group assets by normalized relative path
        // BTreeMap ensures deterministic ordering of paths
        let mut path_map: BTreeMap<String, Vec<(&str, u32, &AssetEntry)>> = BTreeMap::new();

        for (info, assets) in &enabled_mods {
            for asset in assets {
                path_map
                    .entry(asset.relative_path.clone())
                    .or_default()
                    .push((&info.id, info.priority, asset));
            }
        }

        let mut records = Vec::new();
        let mut has_critical = false;
        let mut has_warning = false;

        for (path, providers) in path_map {
            if providers.len() <= 1 {
                continue;
            }

            // The first provider in the sorted list is the winner
            let (winner_id, _, winner_asset) = providers[0];
            let shadowed_ids: Vec<String> = providers[1..]
                .iter()
                .map(|(id, _, _)| id.to_string())
                .collect();

            let severity = if winner_asset.is_critical || is_critical_asset(&path) {
                has_critical = true;
                ConflictSeverity::Critical
            } else if winner_asset.is_exclusive_slot || is_slot_asset(&path) {
                has_warning = true;
                ConflictSeverity::Warning
            } else {
                ConflictSeverity::Info
            };

            let message = match severity {
                ConflictSeverity::Critical => format!(
                    "Critical parameter collision on '{}'! Both '{}' and [{}] modify gameparam.parambnd.dcx. Overwriting will completely discard parameters from lower-priority mods.",
                    path,
                    winner_id,
                    shadowed_ids.join(", ")
                ),
                ConflictSeverity::Warning => format!(
                    "Exclusive model/weapon slot collision on '{}'! Mod '{}' wins; appearance from [{}] will be shadowed.",
                    path,
                    winner_id,
                    shadowed_ids.join(", ")
                ),
                ConflictSeverity::Info => format!(
                    "Asset override on '{}'. Mod '{}' supersedes [{}].",
                    path,
                    winner_id,
                    shadowed_ids.join(", ")
                ),
            };

            records.push(ConflictRecord {
                relative_path: path,
                severity,
                winner_mod_id: winner_id.to_string(),
                shadowed_mod_ids: shadowed_ids,
                message,
            });
        }

        // Sort records: Critical first, then Warning, then Info, then by path
        records.sort_by(|a, b| {
            b.severity
                .cmp(&a.severity)
                .then_with(|| a.relative_path.cmp(&b.relative_path))
        });

        let total_conflicts = records.len();

        ConflictReport {
            has_critical_conflict: has_critical,
            has_warning_conflict: has_warning,
            total_conflicts,
            records,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::types::AssetCategory;
    use std::path::PathBuf;

    #[test]
    fn test_conflict_detection_priorities_and_severity() {
        let mod_a = ModInfo {
            id: "mod_a".to_string(),
            name: "Mod A".to_string(),
            version: "1.0".to_string(),
            author: "Author A".to_string(),
            category: "weapon_skin".to_string(),
            description: None,
            homepage: None,
            source_url: None,
            license: None,
            enabled: true,
            priority: 10, // Higher precedence
            tags: vec![],
            root_path: None,
        };
        let assets_a = vec![
            AssetEntry::new("parts/wp_a_0300.partsbnd.dcx", PathBuf::from("a/wp.dcx"), 100, AssetCategory::Parts),
            AssetEntry::new("param/gameparam/gameparam.parambnd.dcx", PathBuf::from("a/param.dcx"), 500, AssetCategory::Param),
        ];

        let mod_b = ModInfo {
            id: "mod_b".to_string(),
            name: "Mod B".to_string(),
            version: "1.0".to_string(),
            author: "Author B".to_string(),
            category: "overhaul".to_string(),
            description: None,
            homepage: None,
            source_url: None,
            license: None,
            enabled: true,
            priority: 20, // Lower precedence
            tags: vec![],
            root_path: None,
        };
        let assets_b = vec![
            AssetEntry::new("parts/wp_a_0300.partsbnd.dcx", PathBuf::from("b/wp.dcx"), 120, AssetCategory::Parts),
            AssetEntry::new("param/gameparam/gameparam.parambnd.dcx", PathBuf::from("b/param.dcx"), 600, AssetCategory::Param),
            AssetEntry::new("chr/c5110.chrbnd.dcx", PathBuf::from("b/c5110.dcx"), 300, AssetCategory::Chr),
        ];

        let mods = vec![(mod_a, assets_a), (mod_b, assets_b)];
        let report = ConflictEngine::scan_conflicts(&mods);

        assert_eq!(report.total_conflicts, 2);
        assert!(report.has_critical_conflict);
        assert!(report.has_warning_conflict);

        // Find critical record (param)
        let param_rec = report.records.iter().find(|r| r.relative_path == "param/gameparam/gameparam.parambnd.dcx").unwrap();
        assert_eq!(param_rec.severity, ConflictSeverity::Critical);
        assert_eq!(param_rec.winner_mod_id, "mod_a");
        assert_eq!(param_rec.shadowed_mod_ids, vec!["mod_b".to_string()]);

        // Find warning record (weapon slot 0300)
        let wp_rec = report.records.iter().find(|r| r.relative_path == "parts/wp_a_0300.partsbnd.dcx").unwrap();
        assert_eq!(wp_rec.severity, ConflictSeverity::Warning);
        assert_eq!(wp_rec.winner_mod_id, "mod_a");
        assert_eq!(wp_rec.shadowed_mod_ids, vec!["mod_b".to_string()]);
    }
}

