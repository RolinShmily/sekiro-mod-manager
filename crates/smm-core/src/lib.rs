//! Sekiro Mod Manager Core (`smm-core`)
//!
//! A dedicated library for managing, normalizing, analyzing conflicts, and generating
//! deployment plans for *Sekiro: Shadows Die Twice* mods.

pub mod conflict;
pub mod deploy;
pub mod doctor;
pub mod error;
pub mod executor;
pub mod exporter;
pub mod extractor;
pub mod importer;
pub mod loader;
pub mod manager;
pub mod normalizer;
pub mod types;

// Re-exports of core entities
pub use conflict::ConflictEngine;
pub use deploy::DeploymentPlanner;
pub use doctor::{
    diagnose_environment, install_mod_engine, parse_modengine_ini, patch_or_create_modengine_ini,
    provision_mod_engine, DiagnosticItem, DiagnosticStatus, HealthReport, ModEngineConfig,
    OverallHealth,
};
pub use error::{Result, SmmError};
pub use executor::{
    create_hard_link, execute_deploy, is_same_volume, restore_deploy, DeployManifest, DeployResult,
    RestoreResult,
};
pub use exporter::{
    export_modpack, export_single_mod, import_modpack, ModPackItem, ModPackManifest,
};
pub use extractor::{
    detect_archive_format, extract_7z, extract_archive, extract_rar, extract_zip,
    find_external_7z, find_external_unrar, is_supported_archive, ArchiveFormat,
};
pub use importer::{
    default_priority_for_category, extract_version, humanize_name, import_mod, infer_category,
    slugify, ImportOptions,
};
pub use loader::ModLoader;
pub use manager::{
    delete_mod, find_mod_dir, get_mod_details, save_mod_info, set_mod_enabled, set_mod_priority,
    update_mod_info, ModManager,
};

pub use normalizer::{
    NormalizationResult, Normalizer, CANONICAL_DIRS, CANONICAL_ROOT_FILES, FILE_SIGNATURES,
};
pub use types::{
    is_critical_asset, is_slot_asset, AssetCategory, AssetEntry, ConflictRecord, ConflictReport,
    ConflictSeverity, DeployMapping, DeployPlan, ModCategory, ModInfo,
};


