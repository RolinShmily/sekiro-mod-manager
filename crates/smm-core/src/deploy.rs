use std::collections::BTreeMap;
use std::time::{SystemTime, UNIX_EPOCH};

use crate::conflict::ConflictEngine;
use crate::error::Result;
use crate::types::{AssetEntry, DeployMapping, DeployPlan, ModInfo};

/// Deployment orchestrator responsible for building executable deployment plans.
pub struct DeploymentPlanner;

impl DeploymentPlanner {
    /// Constructs a deterministic DeployPlan from enabled mods.
    pub fn build_plan(
        profile_name: impl Into<String>,
        mods: &[(ModInfo, Vec<AssetEntry>)],
    ) -> Result<DeployPlan> {
        let profile_str = profile_name.into();
        let conflict_report = ConflictEngine::scan_conflicts(mods);

        // Filter and sort mods by priority (lowest number = highest precedence)
        let mut enabled_mods: Vec<&(ModInfo, Vec<AssetEntry>)> =
            mods.iter().filter(|(info, _)| info.enabled).collect();

        enabled_mods.sort_by(|a, b| {
            a.0.priority
                .cmp(&b.0.priority)
                .then_with(|| a.0.id.cmp(&b.0.id))
        });

        // Build target mapping table: path -> (winner_asset, winner_mod_id, winner_priority, Vec<shadowed_mod_id>)
        let mut target_map: BTreeMap<String, (AssetEntry, String, u32, Vec<String>)> =
            BTreeMap::new();

        for (info, assets) in &enabled_mods {
            for asset in assets {
                if let Some((_, _, _, shadowed)) = target_map.get_mut(&asset.relative_path) {
                    shadowed.push(info.id.clone());
                } else {
                    target_map.insert(
                        asset.relative_path.clone(),
                        (asset.clone(), info.id.clone(), info.priority, Vec::new()),
                    );
                }
            }
        }

        let mappings: Vec<DeployMapping> = target_map
            .into_iter()
            .map(|(path, (asset, owner_id, priority, shadowed))| DeployMapping {
                target_relative_path: path,
                source_path: asset.source_path,
                owner_mod_id: owner_id,
                priority,
                shadowed_mods: shadowed,
            })
            .collect();

        let timestamp = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.as_secs())
            .unwrap_or(0);

        Ok(DeployPlan {
            active_profile: profile_str,
            timestamp,
            mappings,
            conflict_report,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::types::AssetCategory;
    use std::path::PathBuf;

    #[test]
    fn test_deploy_plan_mappings() {
        let mod_1 = ModInfo {
            id: "mod_first".to_string(),
            name: "First".to_string(),
            version: "1.0".to_string(),
            author: "A".to_string(),
            category: "skin".to_string(),
            description: None,
            homepage: None,
            source_url: None,
            license: None,
            enabled: true,
            priority: 5,
            tags: vec![],
            root_path: None,
        };
        let assets_1 = vec![
            AssetEntry::new("parts/wp_a_0300.partsbnd.dcx", PathBuf::from("src1/wp.dcx"), 50, AssetCategory::Parts),
        ];

        let mod_2 = ModInfo {
            id: "mod_second".to_string(),
            name: "Second".to_string(),
            version: "1.0".to_string(),
            author: "B".to_string(),
            category: "skin".to_string(),
            description: None,
            homepage: None,
            source_url: None,
            license: None,
            enabled: true,
            priority: 15,
            tags: vec![],
            root_path: None,
        };
        let assets_2 = vec![
            AssetEntry::new("parts/wp_a_0300.partsbnd.dcx", PathBuf::from("src2/wp.dcx"), 60, AssetCategory::Parts),
            AssetEntry::new("parts/bd_m_9000.partsbnd.dcx", PathBuf::from("src2/bd.dcx"), 80, AssetCategory::Parts),
        ];

        let plan = DeploymentPlanner::build_plan("default", &[(mod_1, assets_1), (mod_2, assets_2)]).unwrap();

        assert_eq!(plan.mappings.len(), 2);

        // wp_a_0300 should belong to mod_first
        let wp_map = plan.mappings.iter().find(|m| m.target_relative_path == "parts/wp_a_0300.partsbnd.dcx").unwrap();
        assert_eq!(wp_map.owner_mod_id, "mod_first");
        assert_eq!(wp_map.shadowed_mods, vec!["mod_second".to_string()]);

        // bd_m_9000 should belong to mod_second
        let bd_map = plan.mappings.iter().find(|m| m.target_relative_path == "parts/bd_m_9000.partsbnd.dcx").unwrap();
        assert_eq!(bd_map.owner_mod_id, "mod_second");
        assert!(bd_map.shadowed_mods.is_empty());
    }
}

