use serde::{Deserialize, Serialize};
use std::path::PathBuf;

fn default_true() -> bool {
    true
}

/// Category of Sekiro mod according to its primary functional scope.
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum ModCategory {
    Loader,
    GameplayOverhaul,
    WeaponSkin,
    CharacterSkin,
    Ui,
    Audio,
    Animation,
    Map,
    Script,
    TestSample,
    #[serde(untagged)]
    Custom(String),
}

/// Metadata describing a Sekiro Mod package (persisted in mod.json or staging cache).
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ModInfo {
    /// Unique identifier (slug-like, e.g. "dream-of-the-damned")
    pub id: String,
    /// Human-readable display name
    pub name: String,
    /// Semantic or author version
    pub version: String,
    /// Author / Team name
    pub author: String,
    /// Mod category string or enum
    pub category: String,
    /// Detailed description
    #[serde(default)]
    pub description: Option<String>,
    /// Homepage / NexusMods / GitHub repository URL
    #[serde(default)]
    pub homepage: Option<String>,
    /// Upstream source URL / package origin
    #[serde(default)]
    pub source_url: Option<String>,
    /// SPDX license identifier or custom license name
    #[serde(default)]
    pub license: Option<String>,
    /// Whether the mod is actively enabled for deployment
    #[serde(default = "default_true")]
    pub enabled: bool,
    /// Numerical deployment priority (lower number = higher precedence; 1 wins over 10)
    #[serde(default)]
    pub priority: u32,
    /// Categorical tags
    #[serde(default)]
    pub tags: Vec<String>,
    /// Physical location of the mod root directory on disk
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub root_path: Option<PathBuf>,
}

impl ModInfo {
    pub fn new(
        id: impl Into<String>,
        name: impl Into<String>,
        version: impl Into<String>,
        author: impl Into<String>,
        category: impl Into<String>,
    ) -> Self {
        Self {
            id: id.into(),
            name: name.into(),
            version: version.into(),
            author: author.into(),
            category: category.into(),
            description: None,
            homepage: None,
            source_url: None,
            license: None,
            enabled: true,
            priority: 100,
            tags: Vec::new(),
            root_path: None,
        }
    }
}

/// Sekiro asset subsystem category.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum AssetCategory {
    Parts,
    Chr,
    Param,
    Sound,
    Msg,
    Menu,
    Font,
    Mtd,
    Event,
    Map,
    Obj,
    Script,
    Loader,
    Cutscene,
    Sfx,
    Other,
}

/// A single normalized game asset ready for deployment or collision analysis.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AssetEntry {
    /// Normalized relative path in Sekiro mods/ directory (forward-slash delimited, e.g. "parts/wp_a_0300.partsbnd.dcx")
    pub relative_path: String,
    /// Absolute or disk source path of the physical file
    pub source_path: PathBuf,
    /// Size of the asset file in bytes
    pub file_size: u64,
    /// Game subsystem category
    pub category: AssetCategory,
    /// Whether this asset represents a critical engine file (e.g. gameparam.parambnd.dcx)
    pub is_critical: bool,
    /// Whether this asset binds to an exclusive player character/weapon slot (e.g. am_m_9000, wp_a_0300)
    pub is_exclusive_slot: bool,
}

impl AssetEntry {
    pub fn new(
        relative_path: impl Into<String>,
        source_path: impl Into<PathBuf>,
        file_size: u64,
        category: AssetCategory,
    ) -> Self {
        let rel = relative_path.into();
        let is_critical = is_critical_asset(&rel);
        let is_exclusive_slot = is_slot_asset(&rel);
        Self {
            relative_path: rel,
            source_path: source_path.into(),
            file_size,
            category,
            is_critical,
            is_exclusive_slot,
        }
    }
}

/// Checks whether an asset path represents a critical parameter file.
pub fn is_critical_asset(rel_path: &str) -> bool {
    let lower = rel_path.to_ascii_lowercase();
    lower.ends_with("gameparam.parambnd.dcx")
}

/// Checks whether an asset path represents an exclusive player character or weapon slot.
pub fn is_slot_asset(rel_path: &str) -> bool {
    let lower = rel_path.to_ascii_lowercase();
    lower.contains("wp_a_0300") // Default Katana (Kusabimaru)
        || lower.contains("wp_a_0310") // Mortal Blade
        || lower.contains("am_m_9000") // Arm
        || lower.contains("bd_m_9000") // Body
        || lower.contains("hd_m_9000") // Head
        || lower.contains("lg_m_9000") // Legs
        || lower.contains("c0000.chrbnd.dcx") // Player Character Model
}

/// Severity classification for path and semantic conflicts.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum ConflictSeverity {
    /// Normal cosmetic or sound override; higher priority mod cleanly takes over.
    Info,
    /// Weapon or character slot mutual exclusion; only winning mesh/texture is active.
    Warning,
    /// Gameparam.parambnd.dcx or core engine collision; potentially breaks game mechanics.
    Critical,
}

/// A conflict instance on a specific normalized asset target path.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ConflictRecord {
    /// Normalized relative target path where collision occurs
    pub relative_path: String,
    /// Severity level of the collision
    pub severity: ConflictSeverity,
    /// The mod ID that won and will be deployed to this path
    pub winner_mod_id: String,
    /// List of mod IDs whose file on this path was shadowed/superseded
    pub shadowed_mod_ids: Vec<String>,
    /// Explanatory message and remediation guidance
    pub message: String,
}

/// Complete report of all conflicts detected across active mods.
#[derive(Debug, Clone, Default, PartialEq, Eq, Serialize, Deserialize)]
pub struct ConflictReport {
    /// Whether any critical gameparam collision was detected
    pub has_critical_conflict: bool,
    /// Whether any warning slot collision was detected
    pub has_warning_conflict: bool,
    /// Total number of colliding paths
    pub total_conflicts: usize,
    /// Detailed list of conflict records
    pub records: Vec<ConflictRecord>,
}

/// Single file mapping instruction in a deployment plan.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct DeployMapping {
    /// Relative destination path inside Sekiro mods/ (e.g. "parts/wp_a_0300.partsbnd.dcx")
    pub target_relative_path: String,
    /// Physical source path from staging/mod directory
    pub source_path: PathBuf,
    /// Mod ID owning this active mapping
    pub owner_mod_id: String,
    /// Priority number of the winning mod
    pub priority: u32,
    /// Mod IDs whose file was shadowed at this path
    pub shadowed_mods: Vec<String>,
}

/// Complete deployment plan ready for hard link or copy orchestration.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct DeployPlan {
    /// Active deployment profile name (e.g. "default")
    pub active_profile: String,
    /// Plan generation timestamp (epoch seconds)
    pub timestamp: u64,
    /// Concrete list of target file mappings
    pub mappings: Vec<DeployMapping>,
    /// Summary conflict report associated with this deployment
    pub conflict_report: ConflictReport,
}

/// Single mod configuration inside a preset.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ModPresetEntry {
    /// Target mod identifier
    pub mod_id: String,
    /// Priority assigned to this mod in this preset
    pub priority: u32,
}

/// A saved user activation preset specifying which mods are enabled and their priority order.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ModPreset {
    /// Unique identifier for this preset
    pub id: String,
    /// Human-friendly display name (e.g. "修罗一心与剑光特效")
    pub name: String,
    /// Optional user description
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub description: Option<String>,
    /// Created epoch timestamp
    pub created_at: u64,
    /// Last updated epoch timestamp
    pub updated_at: u64,
    /// List of mods enabled in this preset with their respective deployment priorities
    pub mods: Vec<ModPresetEntry>,
}
