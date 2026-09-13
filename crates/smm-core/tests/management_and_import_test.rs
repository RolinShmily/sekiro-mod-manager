use std::fs::File;
use std::io::Write;
use tempfile::tempdir;
use zip::write::SimpleFileOptions;
use zip::ZipWriter;

use smm_core::{
    delete_mod, diagnose_environment, execute_deploy, find_mod_dir, get_mod_details, import_mod,
    install_mod_engine, restore_deploy, set_mod_enabled, set_mod_priority, AssetCategory,
    DeploymentPlanner, DiagnosticStatus, ImportOptions, ModLoader, OverallHealth,
};

#[test]
fn test_end_to_end_zip_import_and_lifecycle() {
    let tmp = tempdir().expect("Failed to create tempdir");
    let base = tmp.path();
    let zip_path = base.join("MortalBlade_Fire_v3.0.zip");
    let staging_dir = base.join("staging");
    let game_mods_dir = base.join("game_mods");

    // 1. Dynamically package a messy, non-canonical zip archive
    let file = File::create(&zip_path).expect("Failed to create zip file");
    let mut zip = ZipWriter::new(file);
    let options = SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);

    zip.start_file(
        "MortalBlade_Fire_v3.0/Sekiro/mods/parts/wp_a_0310.partsbnd.dcx",
        options,
    )
    .unwrap();
    zip.write_all(b"mortal blade red flame mesh data").unwrap();

    zip.start_file("MortalBlade_Fire_v3.0/Documentation/README.md", options)
        .unwrap();
    zip.write_all(b"# Mortal Blade Fire FX\nSpecial fiery katana effects.")
        .unwrap();

    zip.start_file("MortalBlade_Fire_v3.0/LICENSE.txt", options)
        .unwrap();
    zip.write_all(b"Apache 2.0 License").unwrap();

    zip.finish().expect("Failed to finish zip");

    // 2. Import mod into staging
    let imported = import_mod(&zip_path, &staging_dir, &ImportOptions::default())
        .expect("Import failed");

    assert_eq!(imported.id, "mortalblade-fire-v3-0");
    assert_eq!(imported.version, "3.0");
    assert_eq!(imported.category, "weapon_skin");
    assert!(imported.enabled);
    assert_eq!(imported.priority, 100);

    // Verify disk files
    let mod_dir = staging_dir.join("mortalblade-fire-v3-0");
    assert!(mod_dir.exists());
    assert!(mod_dir.join("mod.json").exists());
    assert!(mod_dir.join("parts/wp_a_0310.partsbnd.dcx").exists());
    assert!(mod_dir.join("README.md").exists());
    assert!(mod_dir.join("LICENSE.txt").exists());

    // 3. Test get_mod_details
    let (details, assets) =
        get_mod_details(&staging_dir, "mortalblade-fire-v3-0").expect("Failed to get details");
    assert_eq!(details.id, "mortalblade-fire-v3-0");
    assert_eq!(assets.len(), 1);
    assert_eq!(assets[0].relative_path, "parts/wp_a_0310.partsbnd.dcx");
    assert_eq!(assets[0].category, AssetCategory::Parts);
    assert!(assets[0].is_exclusive_slot);

    // 4. Test state mutation: priority modification & persistence
    let updated_pri = set_mod_priority(&staging_dir, "mortalblade-fire-v3-0", 15)
        .expect("Failed to update priority");
    assert_eq!(updated_pri.priority, 15);

    let reloaded_info =
        ModLoader::load_mod_info(&mod_dir).expect("Failed to reload mod.json");
    assert_eq!(reloaded_info.priority, 15);

    // 5. Test state mutation: toggle enabled / disabled
    let disabled = set_mod_enabled(&staging_dir, "mortalblade-fire-v3-0", false)
        .expect("Failed to disable mod");
    assert!(!disabled.enabled);

    let reloaded_info2 =
        ModLoader::load_mod_info(&mod_dir).expect("Failed to reload mod.json");
    assert!(!reloaded_info2.enabled);

    // 6. Test deployment planner honors disabled status
    let staged_mods =
        ModLoader::scan_mods_directory(&staging_dir).expect("Failed to scan staging");
    let plan_disabled =
        DeploymentPlanner::build_plan("test_profile", &staged_mods).expect("Build plan failed");
    assert!(
        plan_disabled.mappings.is_empty(),
        "Disabled mod should produce 0 active mappings"
    );

    // Re-enable mod
    set_mod_enabled(&staging_dir, "mortalblade-fire-v3-0", true).expect("Failed to re-enable");
    let staged_mods2 =
        ModLoader::scan_mods_directory(&staging_dir).expect("Failed to scan staging");
    let plan_enabled =
        DeploymentPlanner::build_plan("test_profile", &staged_mods2).expect("Build plan failed");
    assert_eq!(plan_enabled.mappings.len(), 1);
    assert_eq!(
        plan_enabled.mappings[0].target_relative_path,
        "parts/wp_a_0310.partsbnd.dcx"
    );

    // 7. Test physical deployment
    let deploy_res = execute_deploy(&plan_enabled, &game_mods_dir).expect("Deploy failed");
    assert_eq!(deploy_res.total_files, 1);
    assert!(game_mods_dir.join("parts/wp_a_0310.partsbnd.dcx").exists());

    // 8. Test restore game mods directory
    let restore_res = restore_deploy(&game_mods_dir).expect("Restore failed");
    assert_eq!(restore_res.removed_files, 1);
    assert!(!game_mods_dir.join("parts/wp_a_0310.partsbnd.dcx").exists());

    // 9. Test delete mod from staging
    delete_mod(&staging_dir, "mortalblade-fire-v3-0").expect("Delete mod failed");
    assert!(!mod_dir.exists());
    assert!(find_mod_dir(&staging_dir, "mortalblade-fire-v3-0").is_err());
}

#[test]
fn test_import_with_loose_signature_files() {
    let tmp = tempdir().expect("Failed to create tempdir");
    let base = tmp.path();
    let zip_path = base.join("LooseSkin_v1.0.zip");
    let staging_dir = base.join("staging");

    // Create zip with loose signature files without canonical directory wrappers
    let file = File::create(&zip_path).unwrap();
    let mut zip = ZipWriter::new(file);
    let options = SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);

    // Loose weapon mesh
    zip.start_file("wp_a_0300.partsbnd.dcx", options).unwrap();
    zip.write_all(b"loose katana mesh").unwrap();

    // Loose body mesh
    zip.start_file("bd_m_9000.partsbnd.dcx", options).unwrap();
    zip.write_all(b"loose body mesh").unwrap();

    zip.finish().unwrap();

    let imported = import_mod(&zip_path, &staging_dir, &ImportOptions::default())
        .expect("Import loose mod failed");

    assert_eq!(imported.id, "looseskin-v1-0");
    let mod_dir = staging_dir.join("looseskin-v1-0");

    // The normalizer must have automatically organized them into parts/
    assert!(mod_dir.join("parts/wp_a_0300.partsbnd.dcx").exists());
    assert!(mod_dir.join("parts/bd_m_9000.partsbnd.dcx").exists());

    let (_, assets) = ModLoader::scan_mod(&mod_dir).unwrap();
    assert_eq!(assets.len(), 2);
    assert_eq!(assets[0].relative_path, "parts/bd_m_9000.partsbnd.dcx");
    assert_eq!(assets[1].relative_path, "parts/wp_a_0300.partsbnd.dcx");
}

#[test]
fn test_import_directory_with_overrides() {
    let tmp = tempdir().expect("Failed to create tempdir");
    let base = tmp.path();
    let source_dir = base.join("RawModFolder");
    let staging_dir = base.join("staging");

    let sound_dir = source_dir.join("sound");
    std::fs::create_dir_all(&sound_dir).unwrap();
    std::fs::write(sound_dir.join("fdp_main.fsb"), b"mock sound fsb").unwrap();

    let opts = ImportOptions {
        custom_id: Some("epic-soundtrack".to_string()),
        custom_name: Some("Epic Soundtrack Replacement".to_string()),
        priority: Some(45),
        overwrite: false,
        source_url: None,
    };

    let imported = import_mod(&source_dir, &staging_dir, &opts).expect("Import dir failed");
    assert_eq!(imported.id, "epic-soundtrack");
    assert_eq!(imported.name, "Epic Soundtrack Replacement");
    assert_eq!(imported.category, "audio");
    assert_eq!(imported.priority, 45);

    let target = staging_dir.join("epic-soundtrack");
    assert!(target.exists());
    assert!(target.join("sound/fdp_main.fsb").exists());
}

#[test]
fn test_end_to_end_7z_import_and_lifecycle() {
    let tmp = tempdir().expect("Failed to create tempdir");
    let base = tmp.path();
    let staging_dir = base.join("staging");
    let game_mods_dir = base.join("game_mods");

    // 1. Create a messy directory tree and compress to .7z using sevenz-rust
    let source_dir = base.join("raw_7z_source");
    let nested_parts = source_dir.join("NestedArchive_v1.5/Sekiro_Mod/mods/parts");
    std::fs::create_dir_all(&nested_parts).unwrap();
    std::fs::write(
        nested_parts.join("am_m_9000.partsbnd.dcx"),
        b"mock prosthetic arm 7z data",
    )
    .unwrap();
    std::fs::write(
        source_dir.join("NestedArchive_v1.5/README.md"),
        b"# 7z Arm Mod\nCustom arm replacement in 7z format.",
    )
    .unwrap();

    let archive_7z = base.join("ShinobiArm_v1.5.7z");
    sevenz_rust::compress_to_path(&source_dir, &archive_7z).expect("Failed to create 7z archive");
    assert!(archive_7z.exists());

    // 2. Import .7z archive into staging
    let imported = import_mod(&archive_7z, &staging_dir, &ImportOptions::default())
        .expect("Failed to import 7z package");

    assert_eq!(imported.id, "shinobiarm-v1-5");
    assert_eq!(imported.version, "1.5");
    assert_eq!(imported.category, "character_skin");
    assert!(imported.enabled);

    let staged_mod = staging_dir.join("shinobiarm-v1-5");
    assert!(staged_mod.exists());
    assert!(staged_mod.join("parts/am_m_9000.partsbnd.dcx").exists());
    assert!(staged_mod.join("README.md").exists());
    assert!(staged_mod.join("mod.json").exists());

    // 3. Verify deployment and clean restoration
    let staged_mods = ModLoader::scan_mods_directory(&staging_dir).unwrap();
    let plan = DeploymentPlanner::build_plan("test_7z", &staged_mods).unwrap();
    assert_eq!(plan.mappings.len(), 1);

    let deploy_res = execute_deploy(&plan, &game_mods_dir).unwrap();
    assert_eq!(deploy_res.total_files, 1);
    assert!(game_mods_dir.join("parts/am_m_9000.partsbnd.dcx").exists());

    let restore_res = restore_deploy(&game_mods_dir).unwrap();
    assert_eq!(restore_res.removed_files, 1);
    assert!(!game_mods_dir.join("parts/am_m_9000.partsbnd.dcx").exists());
}

#[test]
fn test_doctor_diagnostics_and_setup_engine() {
    let tmp = tempdir().expect("Failed to create tempdir");
    let base = tmp.path();
    let game_dir = base.join("SekiroGame");
    let staging_dir = base.join("staging");
    std::fs::create_dir_all(&game_dir).unwrap();
    std::fs::create_dir_all(&staging_dir).unwrap();

    // 1. Scenario: Empty game directory -> ActionRequired (missing sekiro.exe, dinput8.dll, modengine.ini)
    let report1 = diagnose_environment(&game_dir, Some(&staging_dir));
    assert_eq!(report1.overall_status, OverallHealth::ActionRequired);
    assert!(report1.fail_count() >= 3);

    // 2. Scenario: Add sekiro.exe -> sekiro.exe passes, but ModEngine files still fail
    std::fs::write(game_dir.join("sekiro.exe"), b"mock binary").unwrap();
    let report2 = diagnose_environment(&game_dir, Some(&staging_dir));
    assert_eq!(report2.overall_status, OverallHealth::ActionRequired);
    let exe_check = report2.items.iter().find(|i| i.name == "sekiro.exe").unwrap();
    assert_eq!(exe_check.status, DiagnosticStatus::Pass);

    // 3. Scenario: Use setup-engine to deploy ModEngine hook and configuration
    install_mod_engine(&game_dir, None).expect("install_mod_engine failed");
    assert!(game_dir.join("dinput8.dll").exists());
    assert!(game_dir.join("modengine.ini").exists());
    assert!(game_dir.join("mods").exists());

    // Verify modengine.ini contents
    let ini_content = std::fs::read_to_string(game_dir.join("modengine.ini")).unwrap();
    assert!(ini_content.contains("enabled=1"));
    assert!(ini_content.contains(r#"modOverrideDirectory="\mods""#));
    assert!(ini_content.contains("loadLooseParams=1"));

    // 4. Scenario: Re-run diagnosis -> Now overall Healthy!
    let report3 = diagnose_environment(&game_dir, Some(&staging_dir));
    assert_eq!(report3.overall_status, OverallHealth::Healthy);
    assert_eq!(report3.fail_count(), 0);
    assert_eq!(report3.warning_count(), 0);
    assert_eq!(report3.pass_count(), 6);

    // 5. Scenario: Disable modengine in ini -> ActionRequired
    let disabled_ini = ini_content.replace("enabled=1", "enabled=0");
    std::fs::write(game_dir.join("modengine.ini"), disabled_ini).unwrap();
    let report4 = diagnose_environment(&game_dir, Some(&staging_dir));
    assert_eq!(report4.overall_status, OverallHealth::ActionRequired);
    let enabled_check = report4.items.iter().find(|i| i.name == "enabled").unwrap();
    assert_eq!(enabled_check.status, DiagnosticStatus::Fail);

    // 6. Repair via install_mod_engine -> Restores enabled=1
    install_mod_engine(&game_dir, None).expect("repair failed");
    let report5 = diagnose_environment(&game_dir, Some(&staging_dir));
    assert_eq!(report5.overall_status, OverallHealth::Healthy);
}

#[test]
fn test_nested_archive_and_sfx_and_loose_texbnd_import() {
    let tmp = tempdir().expect("Failed to create tempdir");
    let base = tmp.path();
    let staging_dir = base.join("staging");

    // 1. Test Loose texbnd without chr/ directory (e.g. Shura Isshin #2204)
    let isshin_zip = base.join("IsshinSkin_v1.0.zip");
    {
        let file = File::create(&isshin_zip).unwrap();
        let mut zip = ZipWriter::new(file);
        let opt = SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);
        zip.start_file("Custom Isshin/c5409.texbnd.dcx", opt).unwrap();
        zip.write_all(b"MOCK_ISSHIN_TEXTURE").unwrap();
        zip.finish().unwrap();
    }
    let imported_isshin = import_mod(&isshin_zip, &staging_dir, &ImportOptions::default())
        .expect("Failed to import loose texbnd mod");
    let (_, isshin_assets) = ModLoader::scan_mod(&staging_dir.join(&imported_isshin.id)).unwrap();
    assert_eq!(isshin_assets.len(), 1);
    assert_eq!(isshin_assets[0].relative_path, "chr/c5409.texbnd.dcx");
    assert_eq!(isshin_assets[0].category, AssetCategory::Chr);

    // 2. Test Sfx directory package (e.g. Blue Effect #2243)
    let blue_effect_zip = base.join("BlueLazulite_v1.6.zip");
    {
        let file = File::create(&blue_effect_zip).unwrap();
        let mut zip = ZipWriter::new(file);
        let opt = SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);
        zip.start_file("Blue Flame/sfx/sfxbnd_commoneffects.ffxbnd.dcx", opt).unwrap();
        zip.write_all(b"MOCK_SFX_BUNDLE").unwrap();
        zip.start_file("Blue Flame/Fxr files/f000300235.fxr", opt).unwrap();
        zip.write_all(b"MOCK_FXR").unwrap();
        zip.finish().unwrap();
    }
    let imported_blue = import_mod(&blue_effect_zip, &staging_dir, &ImportOptions::default())
        .expect("Failed to import sfx mod");
    assert_eq!(imported_blue.category, "vfx");
    let (_, blue_assets) = ModLoader::scan_mod(&staging_dir.join(&imported_blue.id)).unwrap();
    assert!(blue_assets.iter().any(|a| a.relative_path == "sfx/sfxbnd_commoneffects.ffxbnd.dcx"));

    // 3. Test Nested archive with integration zip (e.g. Lamia #1715)
    let nested_outer_zip = base.join("LamiaBundle_v1.1.zip");
    {
        // Inner integration zip
        let inner_int_path = base.join("Lamia(integration).zip");
        {
            let file = File::create(&inner_int_path).unwrap();
            let mut zip = ZipWriter::new(file);
            let opt = SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);
            zip.start_file("am_m_9000.partsbnd.dcx", opt).unwrap();
            zip.write_all(b"ARM").unwrap();
            zip.start_file("wp_a_0310.partsbnd.dcx", opt).unwrap();
            zip.write_all(b"MORTAL_BLADE").unwrap();
            zip.finish().unwrap();
        }

        // Inner secondary zip
        let inner_char_path = base.join("Lamia(character).zip");
        {
            let file = File::create(&inner_char_path).unwrap();
            let mut zip = ZipWriter::new(file);
            let opt = SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);
            zip.start_file("am_m_9000.partsbnd.dcx", opt).unwrap();
            zip.write_all(b"ARM_ONLY").unwrap();
            zip.finish().unwrap();
        }

        // Outer zip wrapping both
        let file = File::create(&nested_outer_zip).unwrap();
        let mut zip = ZipWriter::new(file);
        let opt = SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);
        zip.start_file("Lamia(integration).zip", opt).unwrap();
        zip.write_all(&std::fs::read(&inner_int_path).unwrap()).unwrap();
        zip.start_file("Lamia(character).zip", opt).unwrap();
        zip.write_all(&std::fs::read(&inner_char_path).unwrap()).unwrap();
        zip.finish().unwrap();
    }

    let imported_lamia = import_mod(&nested_outer_zip, &staging_dir, &ImportOptions::default())
        .expect("Failed to import nested archive mod");
    let (_, lamia_assets) = ModLoader::scan_mod(&staging_dir.join(&imported_lamia.id)).unwrap();
    assert_eq!(lamia_assets.len(), 2);
    assert!(lamia_assets.iter().any(|a| a.relative_path == "parts/am_m_9000.partsbnd.dcx"));
    assert!(lamia_assets.iter().any(|a| a.relative_path == "parts/wp_a_0310.partsbnd.dcx"));
}

