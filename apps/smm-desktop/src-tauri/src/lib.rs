pub mod commands;

pub use commands::*;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![
            commands::get_settings,
            commands::save_settings,
            commands::list_mods,
            commands::get_mod_details,
            commands::toggle_mod,
            commands::set_mod_priority,
            commands::reorder_mods,
            commands::delete_mod,
            commands::import_mod_file,
            commands::scan_conflicts,
            commands::deploy_mods,
            commands::restore_mods,
            commands::diagnose_env,
            commands::setup_mod_engine,
            commands::provision_engine_mod,
            commands::update_mod_info,
            commands::export_single_mod,
            commands::export_modpack,
            commands::import_modpack,
            commands::pick_folder,
            commands::pick_file,
            commands::pick_files,
            commands::open_external_url,
            commands::open_path_in_explorer,
            commands::launch_game,
            commands::import_merged_mod_files,
            commands::list_presets,
            commands::create_preset_from_current,
            commands::save_preset,
            commands::apply_preset,
            commands::delete_preset,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
