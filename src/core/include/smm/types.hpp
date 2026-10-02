#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace smm {

namespace fs = std::filesystem;
using json = nlohmann::json;

/// Category of a Sekiro mod according to its primary functional scope.
enum class ModCategory {
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
    Custom
};

std::string to_string(ModCategory category);
ModCategory mod_category_from_string(std::string_view text);

/// Metadata describing a Sekiro mod package.
///
/// Persisted to disk twice - as `.smm_mod.json` (SMM's own, authoritative) and as
/// `mod.json` (widely understood by the modding ecosystem) - so the two must stay in sync.
struct ModInfo {
    std::string id;         ///< Slug-like unique identifier, e.g. "dream-of-the-damned"
    std::string name;       ///< Human readable display name
    std::string version;    ///< Author supplied version string
    std::string author;
    std::string category;   ///< Kept as free text; see ModCategory for the canonical set
    std::optional<std::string> description;
    std::optional<std::string> homepage;
    std::optional<std::string> source_url;
    std::optional<std::string> license;
    bool enabled{true};
    /// Lower number wins. 1 takes precedence over 10.
    uint32_t priority{100};
    std::vector<std::string> tags;
    /// Relative paths of assets manually disabled by the user inside this mod.
    std::vector<std::string> disabled_assets;
    /// Relative path (or filename) of the mod's preview background image inside staging.
    std::optional<std::string> preview_image;
    /// Physical staging directory of the mod. Never persisted to disk.
    std::optional<fs::path> root_path;

    ModInfo() = default;
    ModInfo(std::string id, std::string name, std::string version, std::string author, std::string category);
};

void to_json(json& j, const ModInfo& m);
void from_json(const json& j, ModInfo& m);

/// Sekiro asset subsystem.
enum class AssetCategory {
    Parts, Chr, Param, Sound, Msg, Menu, Font, Mtd, Event, Map, Obj, Script, Loader, Cutscene, Sfx, Other
};

std::string to_string(AssetCategory category);
AssetCategory asset_category_from_string(std::string_view text);

/// True for parameter files that, when overridden, silently change game balance.
bool is_critical_asset(std::string_view rel_path);

/// True for paths bound to the player's exclusive character / weapon slots.
bool is_slot_asset(std::string_view rel_path);

/// One normalised game asset, ready for deployment or collision analysis.
struct AssetEntry {
    std::string relative_path; ///< Normalised, forward-slash path under the game's mods/ dir
    fs::path source_path;      ///< Physical file in staging
    uint64_t file_size{0};
    AssetCategory category{AssetCategory::Other};
    bool is_critical{false};
    bool is_exclusive_slot{false};
    bool enabled{true}; ///< File-level toggle state

    AssetEntry() = default;
    AssetEntry(std::string rel_path, fs::path src_path, uint64_t size, AssetCategory cat, bool is_enabled = true);
};

void to_json(json& j, const AssetEntry& a);
void from_json(const json& j, AssetEntry& a);

/// Severity of a path collision.
enum class ConflictSeverity { Info = 0, Warning = 1, Critical = 2 };

std::string to_string(ConflictSeverity severity);
ConflictSeverity conflict_severity_from_string(std::string_view text);

/// A single colliding target path.
struct ConflictRecord {
    std::string relative_path;
    ConflictSeverity severity{ConflictSeverity::Info};
    std::string winner_mod_id;
    std::vector<std::string> shadowed_mod_ids;
    std::string message; ///< Human readable explanation, already localised by the caller
};

void to_json(json& j, const ConflictRecord& r);
void from_json(const json& j, ConflictRecord& r);

/// Aggregate of every collision across the currently enabled mods.
struct ConflictReport {
    bool has_critical_conflict{false};
    bool has_warning_conflict{false};
    std::size_t total_conflicts{0};
    std::vector<ConflictRecord> records; ///< Sorted: critical first, then warning, then info

    void recompute_flags();
};

void to_json(json& j, const ConflictReport& r);
void from_json(const json& j, ConflictReport& r);

/// One file mapping in a deployment plan.
struct DeployMapping {
    std::string target_relative_path;
    fs::path source_path;
    std::string owner_mod_id;
    uint32_t priority{100};
    std::vector<std::string> shadowed_mods;
};

void to_json(json& j, const DeployMapping& m);
void from_json(const json& j, DeployMapping& m);

/// A complete, executable deployment plan.
struct DeployPlan {
    std::string active_profile{"default"};
    uint64_t timestamp{0};
    std::vector<DeployMapping> mappings; ///< Ordered by target path
    ConflictReport conflict_report;
};

void to_json(json& j, const DeployPlan& p);
void from_json(const json& j, DeployPlan& p);

/// One mod's participation inside a preset.
struct ModPresetEntry {
    std::string mod_id;
    uint32_t priority{100};
};

void to_json(json& j, const ModPresetEntry& e);
void from_json(const json& j, ModPresetEntry& e);

/// A saved activation state: which mods are on, and at which priority.
struct ModPreset {
    std::string id;
    std::string name;
    std::optional<std::string> description;
    std::optional<std::string> name_en;
    std::optional<std::string> description_en;
    uint64_t created_at{0};
    uint64_t updated_at{0};
    std::vector<ModPresetEntry> mods; ///< Sorted by priority ascending
};

void to_json(json& j, const ModPreset& p);
void from_json(const json& j, ModPreset& p);

/// A mod together with its normalised assets - the unit every planner consumes.
struct StagedMod {
    ModInfo info;
    std::vector<AssetEntry> assets;

    StagedMod() = default;
    StagedMod(ModInfo i, std::vector<AssetEntry> a) : info(std::move(i)), assets(std::move(a)) {}

    uint64_t total_bytes() const;
};

/// Whitespace/format helpers shared by the CLI and the core.
std::string format_bytes(uint64_t bytes);
uint64_t now_epoch_seconds();

} // namespace smm
