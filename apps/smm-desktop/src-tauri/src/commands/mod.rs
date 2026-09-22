pub mod dialogs;
pub mod environment;
pub mod import_export;
pub mod mods;
pub mod presets;
pub mod settings;
pub mod shell;
pub mod util;

pub use dialogs::{pick_file, pick_files, pick_folder};
pub use environment::{
    deploy_mods, diagnose_env, launch_game, provision_engine_mod, restore_mods, scan_conflicts,
    setup_mod_engine,
};
pub use import_export::{
    export_modpack, export_single_mod, import_merged_mod_files, import_mod_file, import_modpack,
};
pub use mods::{
    delete_mod, get_mod_details, list_mods, reorder_mods, set_mod_priority, toggle_mod,
    update_mod_info,
};
pub use presets::{
    apply_preset, create_preset_from_current, delete_preset, list_presets, save_preset,
};
pub use settings::{get_settings, save_settings, AppSettings};
pub use shell::{open_external_url, open_path_in_explorer};

#[cfg(test)]
mod tests;
