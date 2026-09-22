use smm_core::{
    execute_deploy, restore_deploy, AssetCategory, ConflictEngine, ConflictSeverity,
    DeploymentPlanner, ModLoader,
};
use std::fs;
use std::path::Path;

fn create_file(path: &Path, content: &[u8]) {
    if let Some(parent) = path.parent() {
        let _ = fs::create_dir_all(parent);
    }
    fs::write(path, content).unwrap();
}

fn setup_test_suite_in(base: &Path) {
    // 1. mod-engine
    let me = base.join("mod-engine-0.1.16");
    create_file(&me.join("dinput8.dll"), b"MZ_MOCK_DINPUT8_DLL");
    create_file(
        &me.join("modengine.ini"),
        b"[files]\nenabled=1\nloadUXMFiles=0\ncachePaths=1\nmodOverrideDirectory=\"\\mods\"\nloadLooseParams=1\n",
    );
    create_file(
        &me.join("mod.json"),
        br#"{
  "id": "mod-engine-0.1.16",
  "name": "Sekiro Mod Engine",
  "version": "0.1.16",
  "author": "katalash",
  "description": "Core runtime file injection & DirectX input hook for Sekiro: Shadows Die Twice",
  "category": "loader",
  "license": "Proprietary",
  "enabled": true,
  "priority": 0
}"#,
    );

    // 2. dream-of-the-damned
    let dotd = base.join("dream-of-the-damned");
    create_file(
        &dotd.join("param/gameparam/gameparam.parambnd.dcx"),
        b"DCX_PARAM_DATA",
    );
    create_file(&dotd.join("chr/c5110.chrbnd.dcx"), b"DCX_CHR_5110_DATA");
    create_file(&dotd.join("chr/c0000.chrbnd.dcx"), b"DCX_CHR_0000_DATA");
    create_file(&dotd.join("event/common.emevd.dcx"), b"DCX_EMEVD_DATA");
    create_file(&dotd.join("map/m10_00_00_00.msb.dcx"), b"DCX_MSB_DATA");
    create_file(&dotd.join("msg/engus/item.msgbnd.dcx"), b"DCX_MSG_DATA");
    create_file(&dotd.join("script/talk.common.lua"), b"-- lua talk script");
    create_file(
        &dotd.join("mod.json"),
        br#"{
  "id": "dream-of-the-damned",
  "name": "Dream of the Damned - Complete Overhaul",
  "version": "1.2.0",
  "author": "Nuffly",
  "description": "Major overhaul with new encounters, overhauled gameparam, and boss behaviors",
  "category": "overhaul",
  "license": "Apache-2.0",
  "enabled": true,
  "priority": 10
}"#,
    );

    // 3. native-ps4-buttons
    let ps4 = base.join("native-ps4-buttons");
    create_file(&ps4.join("menu/hi/01_common.tpf.dcx"), b"DCX_TPF_DATA");
    create_file(&ps4.join("menu/menu.menubnd.dcx"), b"DCX_MENUBND_DATA");
    create_file(&ps4.join("font/font_ps4.gfx"), b"GFX_FONT_DATA");
    create_file(
        &ps4.join("mod.json"),
        br#"{
  "id": "native-ps4-buttons",
  "name": "Native PS4 Buttons",
  "version": "1.0.0",
  "author": "katalash",
  "description": "Replaces Xbox UI prompts with native DualShock 4 prompts",
  "category": "ui",
  "license": "MIT",
  "enabled": true,
  "priority": 50
}"#,
    );

    // 4. kusabimaru-reaper
    let kr = base.join("kusabimaru-reaper");
    create_file(
        &kr.join("parts/wp_a_0300.partsbnd.dcx"),
        b"DCX_REAPER_SWORD_DATA",
    );
    create_file(
        &kr.join("parts/am_m_9000.partsbnd.dcx"),
        b"DCX_PROSTHETIC_ARM_DATA",
    );
    create_file(
        &kr.join("mod.json"),
        br#"{
  "id": "kusabimaru-reaper",
  "name": "Kusabimaru - Grim Reaper Scythe Reskin",
  "version": "2.1.0",
  "author": "ModderWolf",
  "description": "Transforms Wolf's katana into a sleek dark scythe with custom glow effects",
  "category": "weapon_skin",
  "license": "CC-BY-NC-4.0",
  "enabled": true,
  "priority": 50
}"#,
    );

    // 5. unnormalized-nested-sample
    let un = base.join("unnormalized-nested-sample");
    create_file(
        &un.join("SomeMod_v1.0/Sekiro_Patch/mods/parts/wp_a_0300.partsbnd.dcx"),
        b"DCX_NESTED_SWORD_DATA",
    );
    create_file(
        &un.join("SomeMod_v1.0/Sekiro_Patch/mods/parts/bd_m_9000.partsbnd.dcx"),
        b"DCX_BODY_MESH_DATA",
    );
    create_file(
        &un.join("SomeMod_v1.0/Documentation/HowToInstall.txt"),
        b"Drop into mods folder",
    );
    create_file(
        &un.join("SomeMod_v1.0/Extra_Screenshots/preview.jpg"),
        b"JPG_MOCK",
    );
    create_file(
        &un.join("mod.json"),
        br#"{
  "id": "unnormalized-nested-sample",
  "name": "Nested Structure Sample Mod",
  "version": "0.9.0",
  "author": "NexusCommunity",
  "description": "A realistic nested mod packaging example with redundant root wrappers",
  "category": "weapon_skin",
  "license": "Unlicense",
  "enabled": true,
  "priority": 80
}"#,
    );
}

#[test]
fn test_scan_all_fixtures() {
    let temp = tempfile::tempdir_in(std::env::current_dir().unwrap()).unwrap();
    setup_test_suite_in(temp.path());

    let loaded_mods =
        ModLoader::scan_mods_directory(temp.path()).expect("Failed to scan test directory");

    assert_eq!(loaded_mods.len(), 5, "Expected exactly 5 mods");

    let mod_ids: Vec<&str> = loaded_mods.iter().map(|(m, _)| m.id.as_str()).collect();
    assert!(mod_ids.contains(&"mod-engine-0.1.16"));
    assert!(mod_ids.contains(&"dream-of-the-damned"));
    assert!(mod_ids.contains(&"native-ps4-buttons"));
    assert!(mod_ids.contains(&"kusabimaru-reaper"));
    assert!(mod_ids.contains(&"unnormalized-nested-sample"));
}

#[test]
fn test_mod_engine_fixture() {
    let temp = tempfile::tempdir_in(std::env::current_dir().unwrap()).unwrap();
    setup_test_suite_in(temp.path());
    let mod_dir = temp.path().join("mod-engine-0.1.16");
    let (info, assets) = ModLoader::scan_mod(&mod_dir).expect("Failed to scan mod-engine-0.1.16");

    assert_eq!(info.id, "mod-engine-0.1.16");
    assert_eq!(info.author, "katalash");
    assert_eq!(info.license.as_deref(), Some("Proprietary"));

    let rel_paths: Vec<&str> = assets.iter().map(|a| a.relative_path.as_str()).collect();
    assert!(rel_paths.contains(&"dinput8.dll"));
    assert!(rel_paths.contains(&"modengine.ini"));

    for asset in &assets {
        assert_eq!(asset.category, AssetCategory::Loader);
    }
}

#[test]
fn test_dream_of_the_damned_fixture() {
    let temp = tempfile::tempdir_in(std::env::current_dir().unwrap()).unwrap();
    setup_test_suite_in(temp.path());
    let mod_dir = temp.path().join("dream-of-the-damned");
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
    let temp = tempfile::tempdir_in(std::env::current_dir().unwrap()).unwrap();
    setup_test_suite_in(temp.path());
    let mod_dir = temp.path().join("native-ps4-buttons");
    let (info, assets) = ModLoader::scan_mod(&mod_dir).expect("Failed to scan native-ps4-buttons");

    assert_eq!(info.id, "native-ps4-buttons");
    assert_eq!(info.author, "katalash");

    let rel_paths: Vec<&str> = assets.iter().map(|a| a.relative_path.as_str()).collect();
    assert!(rel_paths.contains(&"menu/hi/01_common.tpf.dcx"));
    assert!(rel_paths.contains(&"menu/menu.menubnd.dcx"));
    assert!(rel_paths.contains(&"menu/font/font_ps4.gfx"));

    let font_asset = assets
        .iter()
        .find(|a| a.relative_path == "menu/font/font_ps4.gfx")
        .expect("menu/font/font_ps4.gfx missing");
    assert_eq!(font_asset.category, AssetCategory::Font);
}

#[test]
fn test_kusabimaru_reaper_fixture() {
    let temp = tempfile::tempdir_in(std::env::current_dir().unwrap()).unwrap();
    setup_test_suite_in(temp.path());
    let mod_dir = temp.path().join("kusabimaru-reaper");
    let (info, assets) = ModLoader::scan_mod(&mod_dir).expect("Failed to scan kusabimaru-reaper");

    assert_eq!(info.id, "kusabimaru-reaper");
    assert_eq!(info.priority, 50);

    let rel_paths: Vec<&str> = assets.iter().map(|a| a.relative_path.as_str()).collect();
    assert!(rel_paths.contains(&"parts/wp_a_0300.partsbnd.dcx"));
    assert!(rel_paths.contains(&"parts/am_m_9000.partsbnd.dcx"));

    for asset in &assets {
        assert_eq!(asset.category, AssetCategory::Parts);
    }
}

#[test]
fn test_unnormalized_nested_sample_heuristic_recovery() {
    let temp = tempfile::tempdir_in(std::env::current_dir().unwrap()).unwrap();
    setup_test_suite_in(temp.path());
    let mod_dir = temp.path().join("unnormalized-nested-sample");
    let (info, assets) =
        ModLoader::scan_mod(&mod_dir).expect("Failed to scan unnormalized-nested-sample");

    assert_eq!(info.id, "unnormalized-nested-sample");

    let rel_paths: Vec<&str> = assets.iter().map(|a| a.relative_path.as_str()).collect();
    assert!(
        rel_paths.contains(&"parts/wp_a_0300.partsbnd.dcx"),
        "Nested Sekiro_Patch/mods/parts/ must be stripped down to parts/"
    );
    assert!(rel_paths.contains(&"parts/bd_m_9000.partsbnd.dcx"));

    // Ensure documentation was skipped from actionable asset tree
    assert!(
        !rel_paths.iter().any(|p| p.contains("HowToInstall")),
        "Documentation files should be classified as documentation/skipped"
    );
}

#[test]
fn test_cross_fixture_conflicts_and_deploy_plan() {
    let temp = tempfile::tempdir_in(std::env::current_dir().unwrap()).unwrap();
    setup_test_suite_in(temp.path());
    let loaded_mods = ModLoader::scan_mods_directory(temp.path()).unwrap();

    // 1. Conflict detection
    let report = ConflictEngine::scan_conflicts(&loaded_mods);
    assert_eq!(report.total_conflicts, 1);

    let conflict = &report.records[0];
    assert_eq!(conflict.relative_path, "parts/wp_a_0300.partsbnd.dcx");
    assert_eq!(conflict.severity, ConflictSeverity::Warning);
    assert_eq!(conflict.winner_mod_id, "kusabimaru-reaper");
    assert_eq!(
        conflict.shadowed_mod_ids,
        vec!["unnormalized-nested-sample"]
    );

    // 2. Deployment planner
    let plan = DeploymentPlanner::build_plan("fixtures_profile", &loaded_mods).unwrap();
    assert_eq!(plan.active_profile, "fixtures_profile");

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
    let temp = tempfile::tempdir_in(std::env::current_dir().unwrap()).unwrap();
    setup_test_suite_in(temp.path());
    let loaded_mods = ModLoader::scan_mods_directory(temp.path()).unwrap();
    let plan = DeploymentPlanner::build_plan("fixtures_profile", &loaded_mods).unwrap();

    // Create target in current working directory to guarantee same-volume testing
    let temp_target = tempfile::tempdir_in(std::env::current_dir().unwrap()).unwrap();
    let deploy_res = execute_deploy(&plan, temp_target.path()).expect("Fixture deploy failed");

    assert!(deploy_res.is_success());
    assert_eq!(deploy_res.total_files, plan.mappings.len());
    assert_eq!(
        deploy_res.hard_links_created + deploy_res.copied_files,
        plan.mappings.len()
    );
    assert_eq!(deploy_res.failed_files, 0);
    assert!(deploy_res.bytes_saved > 0 || deploy_res.copied_files > 0);

    // Verify key deployed files exist in target
    assert!(temp_target.path().join("dinput8.dll").exists());
    assert!(temp_target.path().join("modengine.ini").exists());
    assert!(temp_target
        .path()
        .join("parts/wp_a_0300.partsbnd.dcx")
        .exists());
    assert!(temp_target
        .path()
        .join("param/gameparam/gameparam.parambnd.dcx")
        .exists());

    // Verify manifest
    assert!(temp_target.path().join(".smm_manifest.json").exists());

    // Restore
    let restore_res = restore_deploy(temp_target.path()).expect("Fixture restore failed");
    assert_eq!(restore_res.removed_files, plan.mappings.len());
    assert!(!temp_target.path().join(".smm_manifest.json").exists());
    assert!(!temp_target.path().join("dinput8.dll").exists());
    assert!(!temp_target
        .path()
        .join("parts/wp_a_0300.partsbnd.dcx")
        .exists());
}
