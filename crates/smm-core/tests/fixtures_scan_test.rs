use std::path::Path;
use smm_core::{
    execute_deploy, restore_deploy, AssetCategory, ConflictEngine, ConflictSeverity,
    DeploymentPlanner, ModLoader,
};


fn get_fixtures_dir() -> std::path::PathBuf {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    manifest_dir.join("../../fixtures/mods")
}

#[test]
fn test_scan_all_fixtures() {
    let fixtures_dir = get_fixtures_dir();
    assert!(
        fixtures_dir.exists(),
        "Fixtures directory not found at {}",
        fixtures_dir.display()
    );

    let loaded_mods = ModLoader::scan_mods_directory(&fixtures_dir)
        .expect("Failed to scan fixtures directory");

    // We should have at least 5 standard fixtures loaded
    assert_eq!(loaded_mods.len(), 5, "Expected exactly 5 fixture mods");

    let mod_ids: Vec<&str> = loaded_mods.iter().map(|(m, _)| m.id.as_str()).collect();
    assert!(mod_ids.contains(&"mod-engine-0.1.16"));
    assert!(mod_ids.contains(&"dream-of-the-damned"));
    assert!(mod_ids.contains(&"native-ps4-buttons"));
    assert!(mod_ids.contains(&"kusabimaru-reaper"));
    assert!(mod_ids.contains(&"unnormalized-nested-sample"));
}

#[test]
fn test_mod_engine_fixture() {
    let mod_dir = get_fixtures_dir().join("mod-engine-0.1.16");
    let (info, assets) = ModLoader::scan_mod(&mod_dir).expect("Failed to scan mod-engine-0.1.16");

    assert_eq!(info.id, "mod-engine-0.1.16");
    assert_eq!(info.author, "katalash");
    assert_eq!(info.license.as_deref(), Some("GPL-3.0-or-later"));

    let rel_paths: Vec<&str> = assets.iter().map(|a| a.relative_path.as_str()).collect();
    assert!(rel_paths.contains(&"dinput8.dll"));
    assert!(rel_paths.contains(&"modengine.ini"));

    for asset in &assets {
        assert_eq!(asset.category, AssetCategory::Loader);
    }
}

#[test]
fn test_dream_of_the_damned_fixture() {
    let mod_dir = get_fixtures_dir().join("dream-of-the-damned");
    let (info, assets) = ModLoader::scan_mod(&mod_dir).expect("Failed to scan dream-of-the-damned");

    assert_eq!(info.id, "dream-of-the-damned");
    assert_eq!(info.author, "Nuffly");
    assert_eq!(info.license.as_deref(), Some("Apache-2.0"));

    let rel_paths: Vec<&str> = assets.iter().map(|a| a.relative_path.as_str()).collect();
    assert!(rel_paths.contains(&"param/gameparam/gameparam.parambnd.dcx"));
    assert!(rel_paths.contains(&"chr/c5110.chrbnd.dcx"));
    assert!(rel_paths.contains(&"chr/c0000.chrbnd.dcx"));
    assert!(rel_paths.contains(&"event/common.emevd.dcx"));
    assert!(rel_paths.contains(&"map/m10_00_00_00.msb.dcx"));
    assert!(rel_paths.contains(&"msg/engus/item.msgbnd.dcx"));
    assert!(rel_paths.contains(&"script/talk.common.lua"));

    let param_asset = assets
        .iter()
        .find(|a| a.relative_path == "param/gameparam/gameparam.parambnd.dcx")
        .expect("gameparam.parambnd.dcx missing");
    assert!(param_asset.is_critical, "gameparam must be marked critical");
    assert_eq!(param_asset.category, AssetCategory::Param);
}

#[test]
fn test_native_ps4_buttons_fixture() {
    let mod_dir = get_fixtures_dir().join("native-ps4-buttons");
    let (info, assets) = ModLoader::scan_mod(&mod_dir).expect("Failed to scan native-ps4-buttons");

    assert_eq!(info.id, "native-ps4-buttons");
    assert_eq!(info.author, "katalash");

    let rel_paths: Vec<&str> = assets.iter().map(|a| a.relative_path.as_str()).collect();
    assert!(rel_paths.contains(&"menu/hi/01_common.tpf.dcx"));
    assert!(rel_paths.contains(&"menu/menu.menubnd.dcx"));
    assert!(rel_paths.contains(&"font/font_ps4.gfx"));

    let font_asset = assets
        .iter()
        .find(|a| a.relative_path == "font/font_ps4.gfx")
        .expect("font_ps4.gfx missing");
    assert_eq!(font_asset.category, AssetCategory::Font);
}

#[test]
fn test_kusabimaru_reaper_fixture() {
    let mod_dir = get_fixtures_dir().join("kusabimaru-reaper");
    let (info, assets) = ModLoader::scan_mod(&mod_dir).expect("Failed to scan kusabimaru-reaper");

    assert_eq!(info.id, "kusabimaru-reaper");
    assert_eq!(assets.len(), 2);

    let rel_paths: Vec<&str> = assets.iter().map(|a| a.relative_path.as_str()).collect();
    assert!(rel_paths.contains(&"parts/wp_a_0300.partsbnd.dcx"));
    assert!(rel_paths.contains(&"parts/am_m_9000.partsbnd.dcx"));

    for a in &assets {
        assert_eq!(a.category, AssetCategory::Parts);
        assert!(a.is_exclusive_slot, "Weapon and arm must be exclusive slots");
    }
}

#[test]
fn test_unnormalized_nested_sample_heuristic_recovery() {
    let mod_dir = get_fixtures_dir().join("unnormalized-nested-sample");
    let (info, assets) =
        ModLoader::scan_mod(&mod_dir).expect("Failed to normalize unnormalized-nested-sample");

    assert_eq!(info.id, "unnormalized-nested-sample");

    let rel_paths: Vec<&str> = assets.iter().map(|a| a.relative_path.as_str()).collect();

    // The heuristic normalizer must strip SomeMod_v1.0/Sekiro_Patch/mods/
    assert!(
        rel_paths.contains(&"parts/wp_a_0300.partsbnd.dcx"),
        "wp_a_0300 must be normalized into parts/wp_a_0300.partsbnd.dcx"
    );
    assert!(
        rel_paths.contains(&"parts/bd_m_9000.partsbnd.dcx"),
        "bd_m_9000 must be normalized into parts/bd_m_9000.partsbnd.dcx"
    );

    // Non-game files must NOT be in assets
    assert!(
        !rel_paths.iter().any(|p| p.contains("HowToInstall")),
        "Install notes should be ignored"
    );
    assert!(
        !rel_paths.iter().any(|p| p.contains("preview.jpg")),
        "Screenshots should be ignored"
    );
}

#[test]
fn test_cross_fixture_conflicts_and_deploy_plan() {
    let fixtures_dir = get_fixtures_dir();
    let loaded_mods = ModLoader::scan_mods_directory(&fixtures_dir).unwrap();

    // Detect conflicts across all fixtures
    let conflict_report = ConflictEngine::scan_conflicts(&loaded_mods);

    // Both kusabimaru-reaper and unnormalized-nested-sample provide parts/wp_a_0300.partsbnd.dcx
    assert!(
        conflict_report.total_conflicts >= 1,
        "Should have detected at least one conflict"
    );

    let wp_conflict = conflict_report
        .records
        .iter()
        .find(|r| r.relative_path == "parts/wp_a_0300.partsbnd.dcx")
        .expect("Expected conflict on parts/wp_a_0300.partsbnd.dcx");

    assert_eq!(wp_conflict.severity, ConflictSeverity::Warning);
    // kusabimaru-reaper priority = 30, unnormalized priority = 40 => reaper wins
    assert_eq!(wp_conflict.winner_mod_id, "kusabimaru-reaper");
    assert_eq!(
        wp_conflict.shadowed_mod_ids,
        vec!["unnormalized-nested-sample".to_string()]
    );

    // Generate Deploy Plan
    let plan = DeploymentPlanner::build_plan("test_profile", &loaded_mods).unwrap();
    assert_eq!(plan.active_profile, "test_profile");

    // Ensure winning mapping for wp_a_0300 is kusabimaru-reaper
    let wp_mapping = plan
        .mappings
        .iter()
        .find(|m| m.target_relative_path == "parts/wp_a_0300.partsbnd.dcx")
        .expect("wp_a_0300 must exist in deploy mappings");

    assert_eq!(wp_mapping.owner_mod_id, "kusabimaru-reaper");
    assert_eq!(
        wp_mapping.shadowed_mods,
        vec!["unnormalized-nested-sample".to_string()]
    );
}

#[test]
fn test_deploy_and_restore_fixtures() {
    let fixtures_dir = get_fixtures_dir();
    let loaded_mods = ModLoader::scan_mods_directory(&fixtures_dir).unwrap();
    let plan = DeploymentPlanner::build_plan("fixtures_profile", &loaded_mods).unwrap();

    let temp_target = tempfile::tempdir().unwrap();
    let deploy_res = execute_deploy(&plan, temp_target.path()).expect("Fixture deploy failed");

    assert!(deploy_res.is_success());
    assert_eq!(deploy_res.total_files, plan.mappings.len());
    assert_eq!(deploy_res.hard_links_created, plan.mappings.len());
    assert_eq!(deploy_res.failed_files, 0);
    assert!(deploy_res.bytes_saved > 0);

    // Verify key deployed files exist in target
    assert!(temp_target.path().join("dinput8.dll").exists());
    assert!(temp_target.path().join("modengine.ini").exists());
    assert!(temp_target.path().join("parts/wp_a_0300.partsbnd.dcx").exists());
    assert!(temp_target.path().join("param/gameparam/gameparam.parambnd.dcx").exists());

    // Verify manifest
    assert!(temp_target.path().join(".smm_manifest.json").exists());

    // Restore
    let restore_res = restore_deploy(temp_target.path()).expect("Fixture restore failed");
    assert_eq!(restore_res.removed_files, plan.mappings.len());
    assert!(!temp_target.path().join(".smm_manifest.json").exists());
    assert!(!temp_target.path().join("dinput8.dll").exists());
    assert!(!temp_target.path().join("parts/wp_a_0300.partsbnd.dcx").exists());
}

