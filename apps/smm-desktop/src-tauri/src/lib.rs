pub mod commands;

pub use commands::*;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![
            commands::settings::get_settings,
            commands::settings::save_settings,
            commands::mods::list_mods,
            commands::mods::get_mod_details,
            commands::mods::toggle_mod,
            commands::mods::set_mod_priority,
            commands::mods::reorder_mods,
            commands::mods::delete_mod,
            commands::import_export::import_mod_file,
            commands::environment::scan_conflicts,
            commands::environment::deploy_mods,
            commands::environment::restore_mods,
            commands::environment::diagnose_env,
            commands::environment::setup_mod_engine,
            commands::environment::provision_engine_mod,
            commands::mods::update_mod_info,
            commands::import_export::export_single_mod,
            commands::import_export::export_modpack,
            commands::import_export::import_modpack,
            commands::dialogs::pick_folder,
            commands::dialogs::pick_file,
            commands::dialogs::pick_files,
            commands::shell::open_external_url,
            commands::shell::open_path_in_explorer,
            commands::environment::launch_game,
            commands::import_export::import_merged_mod_files,
            commands::presets::list_presets,
            commands::presets::create_preset_from_current,
            commands::presets::save_preset,
            commands::presets::apply_preset,
            commands::presets::delete_preset,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
