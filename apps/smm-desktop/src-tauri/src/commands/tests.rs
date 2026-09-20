use std::path::Path;
use std::path::PathBuf;

use smm_core::{ModInfo, ModLoader};
use tempfile::tempdir;

use super::*;

fn create_test_staging(dir: &Path) {
    let kr = dir.join("kusabimaru-reaper");
    let wp = kr.join("parts/wp_a_0300.partsbnd.dcx");
    std::fs::create_dir_all(wp.parent().unwrap()).unwrap();
    std::fs::write(&wp, b"KATANA_MESH_MOCK").unwrap();
    let kr_info = ModInfo {
        id: "kusabimaru-reaper".to_string(),
        name: "Kusabimaru Reaper".to_string(),
        version: "1.0.0".to_string(),
        author: "Author".to_string(),
        description: None,
        category: "weapon_skin".to_string(),
        license: None,
        enabled: true,
        priority: 10,
        tags: Vec::new(),
        root_path: None,
        source_url: None,
        homepage: None,
    };
    std::fs::write(kr.join("mod.json"), serde_json::to_string(&kr_info).unwrap()).unwrap();

    let ps4 = dir.join("native-ps4-buttons");
    let btn = ps4.join("menu/menu.menubnd.dcx");
    std::fs::create_dir_all(btn.parent().unwrap()).unwrap();
    std::fs::write(&btn, b"PS4_BUTTON_MOCK").unwrap();
    let ps4_info = ModInfo {
        id: "native-ps4-buttons".to_string(),
        name: "Native PS4 Buttons".to_string(),
        version: "1.0.0".to_string(),
        author: "Author".to_string(),
        description: None,
        category: "ui".to_string(),
        license: None,
        enabled: true,
        priority: 50,
        tags: Vec::new(),
        root_path: None,
        source_url: None,
        homepage: None,
    };
    std::fs::write(ps4.join("mod.json"), serde_json::to_string(&ps4_info).unwrap()).unwrap();

    // Conflict mod for scan_conflicts test
    let conflict = dir.join("conflict-reaper");
    let c_wp = conflict.join("parts/wp_a_0300.partsbnd.dcx");
    std::fs::create_dir_all(c_wp.parent().unwrap()).unwrap();
    std::fs::write(&c_wp, b"CONFLICTING_KATANA_MESH").unwrap();
    let c_info = ModInfo {
        id: "conflict-reaper".to_string(),
        name: "Conflict Reaper".to_string(),
        version: "1.0.0".to_string(),
        author: "Author".to_string(),
        description: None,
        category: "weapon_skin".to_string(),
        license: None,
        enabled: true,
        priority: 20,
        tags: Vec::new(),
        root_path: None,
        source_url: None,
        homepage: None,
    };
    std::fs::write(conflict.join("mod.json"), serde_json::to_string(&c_info).unwrap()).unwrap();
}

#[test]
fn test_get_settings_probe() {
    let settings = get_settings().expect("Failed to get settings");
    assert!(!settings.staging_dir.is_empty());
}

#[test]
fn test_list_mods_from_fixtures() {
    let tmp = tempdir().unwrap();
    create_test_staging(tmp.path());
    let mods = list_mods(tmp.path().to_string_lossy().to_string()).expect("list_mods failed");
    assert!(!mods.is_empty());
    let ids: Vec<&str> = mods.iter().map(|m| m.info.id.as_str()).collect();
    assert!(ids.contains(&"kusabimaru-reaper"));
    assert!(ids.contains(&"native-ps4-buttons"));
}

#[test]
fn test_get_mod_details() {
    let tmp = tempdir().unwrap();
    create_test_staging(tmp.path());
    let details = get_mod_details(
        tmp.path().to_string_lossy().to_string(),
        "kusabimaru-reaper".to_string(),
    )
    .expect("get_mod_details failed");

    assert_eq!(details.info.id, "kusabimaru-reaper");
    assert!(!details.assets.is_empty());
    assert!(details.total_size > 0);
}

#[test]
fn test_scan_conflicts() {
    let tmp = tempdir().unwrap();
    create_test_staging(tmp.path());
    let report =
        scan_conflicts(tmp.path().to_string_lossy().to_string()).expect("scan_conflicts failed");
    assert!(report.total_conflicts > 0);
}

#[test]
fn test_deploy_and_restore_cycle() {
    let tmp = tempdir().unwrap();
    create_test_staging(tmp.path());
    let mock_game = tmp.path().join("SekiroGame");
    std::fs::create_dir_all(&mock_game).unwrap();

    let deploy_res = deploy_mods(
        mock_game.to_string_lossy().to_string(),
        tmp.path().to_string_lossy().to_string(),
    )
    .expect("deploy_mods failed");

    assert!(deploy_res.is_success());
    assert!(mock_game.join("mods").exists());
    assert!(mock_game.join("mods").join(".smm_manifest.json").exists());

    let restore_res =
        restore_mods(mock_game.to_string_lossy().to_string()).expect("restore_mods failed");
    assert!(restore_res.success);
    assert!(!mock_game.join("mods").join(".smm_manifest.json").exists());
}

#[test]
fn test_diagnose_and_setup_engine() {
    let tmp = tempdir().unwrap();
    create_test_staging(tmp.path());
    let mock_game = tmp.path().join("SekiroGame");
    std::fs::create_dir_all(&mock_game).unwrap();

    let _ = provision_engine_mod(tmp.path().to_string_lossy().to_string());

    let report = diagnose_env(
        mock_game.to_string_lossy().to_string(),
        tmp.path().to_string_lossy().to_string(),
    )
    .expect("diagnose_env failed");

    assert!(!report.items.is_empty());
}

#[test]
fn test_update_mod_info_ipc() {
    let tmp = tempdir().unwrap();
    let staging = tmp.path().join("staging");
    create_test_staging(&staging);

    let dst_mod = staging.join("kusabimaru-reaper");
    let mut patch = ModLoader::load_mod_info(&dst_mod).unwrap();
    patch.name = "Reaper Katana Overhaul".to_string();
    patch.source_url = Some("https://nexusmods.com/sekiro/mods/999".to_string());
    patch.homepage = Some("https://github.com/example/reaper".to_string());

    let res = update_mod_info(
        staging.to_string_lossy().to_string(),
        "kusabimaru-reaper".to_string(),
        patch,
    )
    .expect("update_mod_info IPC failed");

    assert_eq!(res.name, "Reaper Katana Overhaul");
    assert_eq!(
        res.source_url.as_deref(),
        Some("https://nexusmods.com/sekiro/mods/999")
    );
    assert_eq!(
        res.homepage.as_deref(),
        Some("https://github.com/example/reaper")
    );
}

#[test]
fn test_export_and_import_modpack_ipc() {
    let tmp = tempdir().unwrap();
    let staging_src = tmp.path().join("staging_src");
    let staging_dst = tmp.path().join("staging_dst");
    let out_dir = tmp.path().join("exports");
    std::fs::create_dir_all(&staging_src).unwrap();
    std::fs::create_dir_all(&staging_dst).unwrap();
    std::fs::create_dir_all(&out_dir).unwrap();

    create_test_staging(&staging_src);

    // 1. Single mod export IPC
    let single_zip = out_dir.join("single_reaper.zip").to_string_lossy().to_string();
    let exported_zip = export_single_mod(
        staging_src.to_string_lossy().to_string(),
        "kusabimaru-reaper".to_string(),
        single_zip.clone(),
        true,
    )
    .expect("export_single_mod IPC failed");
    assert!(PathBuf::from(&exported_zip).exists());

    // 2. Modpack export IPC
    let pack_path = out_dir.join("test_pack.smmpack").to_string_lossy().to_string();
    let exported_pack = export_modpack(
        staging_src.to_string_lossy().to_string(),
        vec!["kusabimaru-reaper".to_string(), "native-ps4-buttons".to_string()],
        "Test Pack".to_string(),
        "1.0.0".to_string(),
        Some("Author".to_string()),
        Some("Test Modpack Description".to_string()),
        pack_path.clone(),
        true,
    )
    .expect("export_modpack IPC failed");
    assert!(PathBuf::from(&exported_pack).exists());

    // 3. Modpack import IPC
    let manifest = import_modpack(
        staging_dst.to_string_lossy().to_string(),
        exported_pack,
        false,
    )
    .expect("import_modpack IPC failed");

    assert_eq!(manifest.name, "Test Pack");
    assert_eq!(manifest.mods.len(), 2);
    assert!(staging_dst.join("kusabimaru-reaper").exists());
    assert!(staging_dst.join("native-ps4-buttons").exists());
}

#[test]
fn test_preset_ipc_cycle() {
    let tmp = tempdir().unwrap();
    let staging = tmp.path().to_string_lossy().to_string();
    create_test_staging(tmp.path());

    // 1. Initially 0 presets
    let list1 = list_presets(staging.clone()).expect("list_presets failed");
    assert!(list1.is_empty());

    // 2. Create preset from current state
    let preset = create_preset_from_current(
        staging.clone(),
        "Combat Build".to_string(),
        Some("Active mods preset".to_string()),
    )
    .expect("create_preset failed");

    assert_eq!(preset.name, "Combat Build");
    assert!(!preset.mods.is_empty());

    // 3. Apply preset
    let applied = apply_preset(staging.clone(), preset.id.clone()).expect("apply_preset failed");
    assert_eq!(applied.id, preset.id);

    // 4. Delete preset
    delete_preset(staging.clone(), preset.id).expect("delete_preset failed");
    let list2 = list_presets(staging).expect("list_presets after delete failed");
    assert!(list2.is_empty());
}