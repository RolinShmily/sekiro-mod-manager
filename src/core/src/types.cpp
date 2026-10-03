#include "smm/types.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace smm {

std::string path_to_utf8(const fs::path& p) {
#ifdef _WIN32
    const std::wstring ws = p.native();
    if (ws.empty()) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, ws.data(), static_cast<int>(ws.size()), NULL, 0, NULL, NULL);
    if (size_needed <= 0) return {};
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.data(), static_cast<int>(ws.size()), &str[0], size_needed, NULL, NULL);
    return str;
#else
    return p.string();
#endif
}

fs::path utf8_to_path(const std::string& str) {
#ifdef _WIN32
    if (str.empty()) return {};
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0);
    if (size_needed <= 0) return {};
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &wstr[0], size_needed);
    return fs::path(wstr);
#else
    return fs::path(str);
#endif
}
namespace {

/// Field helper: read an optional string member, tolerating both absent and null.
void read_optional(const json& j, const char* key, std::optional<std::string>& target) {
    const auto it = j.find(key);
    if (it == j.end() || it->is_null()) {
        target.reset();
        return;
    }
    if (it->is_string()) {
        target = it->get<std::string>();
    }
}

void write_optional(json& j, const char* key, const std::optional<std::string>& value) {
    if (value.has_value()) {
        j[key] = *value;
    }
}

bool read_bool(const json& j, const char* key, bool fallback) {
    const auto it = j.find(key);
    if (it == j.end() || !it->is_boolean()) {
        return fallback;
    }
    return it->get<bool>();
}

uint32_t read_priority(const json& j) {
    const auto it = j.find("priority");
    if (it == j.end() || !it->is_number_unsigned()) {
        // Metadata written by hand or by other tools may omit priority; 100 keeps the
        // documented default and matches the Rust implementation.
        return 100;
    }
    return it->get<uint32_t>();
}

} // namespace

// ---------------------------------------------------------------------------
// ModCategory
// ---------------------------------------------------------------------------

std::string to_string(ModCategory category) {
    switch (category) {
        case ModCategory::Loader: return "loader";
        case ModCategory::GameplayOverhaul: return "gameplay_overhaul";
        case ModCategory::WeaponSkin: return "weapon_skin";
        case ModCategory::CharacterSkin: return "character_skin";
        case ModCategory::Ui: return "ui";
        case ModCategory::Audio: return "audio";
        case ModCategory::Animation: return "animation";
        case ModCategory::Map: return "map";
        case ModCategory::Script: return "script";
        case ModCategory::TestSample: return "test_sample";
        case ModCategory::Custom: return "custom";
    }
    return "custom";
}

ModCategory mod_category_from_string(std::string_view text) {
    if (text == "loader") return ModCategory::Loader;
    if (text == "gameplay_overhaul") return ModCategory::GameplayOverhaul;
    if (text == "weapon_skin") return ModCategory::WeaponSkin;
    if (text == "character_skin") return ModCategory::CharacterSkin;
    if (text == "ui") return ModCategory::Ui;
    if (text == "audio") return ModCategory::Audio;
    if (text == "animation") return ModCategory::Animation;
    if (text == "map") return ModCategory::Map;
    if (text == "script") return ModCategory::Script;
    if (text == "test_sample") return ModCategory::TestSample;
    return ModCategory::Custom;
}

// ---------------------------------------------------------------------------
// AssetCategory
// ---------------------------------------------------------------------------

std::string to_string(AssetCategory category) {
    switch (category) {
        case AssetCategory::Parts: return "parts";
        case AssetCategory::Chr: return "chr";
        case AssetCategory::Param: return "param";
        case AssetCategory::Sound: return "sound";
        case AssetCategory::Msg: return "msg";
        case AssetCategory::Menu: return "menu";
        case AssetCategory::Font: return "font";
        case AssetCategory::Mtd: return "mtd";
        case AssetCategory::Event: return "event";
        case AssetCategory::Map: return "map";
        case AssetCategory::Obj: return "obj";
        case AssetCategory::Script: return "script";
        case AssetCategory::Loader: return "loader";
        case AssetCategory::Cutscene: return "cutscene";
        case AssetCategory::Sfx: return "sfx";
        case AssetCategory::Other: return "other";
    }
    return "other";
}

AssetCategory asset_category_from_string(std::string_view text) {
    if (text == "parts") return AssetCategory::Parts;
    if (text == "chr") return AssetCategory::Chr;
    if (text == "param") return AssetCategory::Param;
    if (text == "sound") return AssetCategory::Sound;
    if (text == "msg") return AssetCategory::Msg;
    if (text == "menu") return AssetCategory::Menu;
    if (text == "font") return AssetCategory::Font;
    if (text == "mtd") return AssetCategory::Mtd;
    if (text == "event") return AssetCategory::Event;
    if (text == "map") return AssetCategory::Map;
    if (text == "obj") return AssetCategory::Obj;
    if (text == "script") return AssetCategory::Script;
    if (text == "loader") return AssetCategory::Loader;
    if (text == "cutscene") return AssetCategory::Cutscene;
    if (text == "sfx") return AssetCategory::Sfx;
    return AssetCategory::Other;
}

bool is_critical_asset(std::string_view rel_path) {
    // gameparam.parambnd.dcx carries global balance data: overriding it silently discards
    // every parameter change made by the mods it shadows, so it is flagged everywhere.
    std::string lower(rel_path);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    constexpr std::string_view kSuffix = "gameparam.parambnd.dcx";
    return lower.size() >= kSuffix.size() &&
           lower.compare(lower.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0;
}

bool is_slot_asset(std::string_view rel_path) {
    std::string lower(rel_path);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    constexpr std::string_view slots[] = {
        "wp_a_0300", // Kusabimaru, the default katana
        "wp_a_0310", // Mortal Blade
        "am_m_9000", // Left arm
        "bd_m_9000", // Body
        "hd_m_9000", // Head
        "lg_m_9000", // Legs
        "c0000.chrbnd.dcx", // Player character model
    };
    for (const auto slot : slots) {
        if (lower.find(slot) != std::string::npos) {
            return true;
        }
    }
    return false;
}

AssetEntry::AssetEntry(std::string rel_path, fs::path src_path, uint64_t size, AssetCategory cat, bool is_enabled)
    : relative_path(std::move(rel_path)),
      source_path(std::move(src_path)),
      file_size(size),
      category(cat),
      is_critical(is_critical_asset(relative_path)),
      is_exclusive_slot(is_slot_asset(relative_path)),
      enabled(is_enabled) {}

// ---------------------------------------------------------------------------
// ModInfo
// ---------------------------------------------------------------------------

ModInfo::ModInfo(std::string mod_id, std::string mod_name, std::string mod_version,
                 std::string mod_author, std::string mod_category)
    : id(std::move(mod_id)),
      name(std::move(mod_name)),
      version(std::move(mod_version)),
      author(std::move(mod_author)),
      category(std::move(mod_category)) {}

void to_json(json& j, const ModInfo& m) {
    j = json{
        {"id", m.id},
        {"name", m.name},
        {"version", m.version},
        {"author", m.author},
        {"category", m.category},
        {"enabled", m.enabled},
        {"priority", m.priority},
        {"tags", m.tags},
        {"disabled_assets", m.disabled_assets},
    };
    write_optional(j, "description", m.description);
    write_optional(j, "homepage", m.homepage);
    write_optional(j, "source_url", m.source_url);
    write_optional(j, "license", m.license);
    write_optional(j, "preview_image", m.preview_image);
    // root_path is machine specific, so it is deliberately never persisted.
}

void from_json(const json& j, ModInfo& m) {
    m.id = j.value("id", std::string{});
    m.name = j.value("name", std::string{});
    m.version = j.value("version", std::string{});
    m.author = j.value("author", std::string{});
    m.category = j.value("category", std::string{"custom"});
    read_optional(j, "description", m.description);
    read_optional(j, "homepage", m.homepage);
    read_optional(j, "source_url", m.source_url);
    read_optional(j, "license", m.license);
    read_optional(j, "preview_image", m.preview_image);
    if (!m.preview_image.has_value() || m.preview_image->empty()) {
        read_optional(j, "preview", m.preview_image);
    }
    m.enabled = read_bool(j, "enabled", true);
    m.priority = read_priority(j);
    if (const auto it = j.find("tags"); it != j.end() && it->is_array()) {
        m.tags = it->get<std::vector<std::string>>();
    } else {
        m.tags.clear();
    }
    if (const auto it = j.find("disabled_assets"); it != j.end() && it->is_array()) {
        m.disabled_assets = it->get<std::vector<std::string>>();
    } else {
        m.disabled_assets.clear();
    }
    m.root_path.reset();
}

// ---------------------------------------------------------------------------
// AssetEntry
// ---------------------------------------------------------------------------

void to_json(json& j, const AssetEntry& a) {
    j = json{
        {"relative_path", a.relative_path},
        {"source_path", path_to_utf8(a.source_path)},
        {"file_size", a.file_size},
        {"category", to_string(a.category)},
        {"is_critical", a.is_critical},
        {"is_exclusive_slot", a.is_exclusive_slot},
        {"enabled", a.enabled},
    };
}

void from_json(const json& j, AssetEntry& a) {
    a.relative_path = j.value("relative_path", std::string{});
    a.source_path = utf8_to_path(j.value("source_path", std::string{}));
    a.file_size = j.value("file_size", uint64_t{0});
    a.category = asset_category_from_string(j.value("category", std::string{"other"}));
    a.is_critical = j.value("is_critical", is_critical_asset(a.relative_path));
    a.is_exclusive_slot = j.value("is_exclusive_slot", is_slot_asset(a.relative_path));
    a.enabled = read_bool(j, "enabled", true);
}

// ---------------------------------------------------------------------------
// ConflictSeverity / ConflictRecord / ConflictReport
// ---------------------------------------------------------------------------

std::string to_string(ConflictSeverity severity) {
    switch (severity) {
        case ConflictSeverity::Info: return "info";
        case ConflictSeverity::Warning: return "warning";
        case ConflictSeverity::Critical: return "critical";
    }
    return "info";
}

ConflictSeverity conflict_severity_from_string(std::string_view text) {
    if (text == "critical") return ConflictSeverity::Critical;
    if (text == "warning") return ConflictSeverity::Warning;
    return ConflictSeverity::Info;
}

void to_json(json& j, const ConflictRecord& r) {
    j = json{
        {"relative_path", r.relative_path},
        {"severity", to_string(r.severity)},
        {"winner_mod_id", r.winner_mod_id},
        {"shadowed_mod_ids", r.shadowed_mod_ids},
        {"message", r.message},
    };
}

void from_json(const json& j, ConflictRecord& r) {
    r.relative_path = j.value("relative_path", std::string{});
    r.severity = conflict_severity_from_string(j.value("severity", std::string{"info"}));
    r.winner_mod_id = j.value("winner_mod_id", std::string{});
    r.shadowed_mod_ids = j.value("shadowed_mod_ids", std::vector<std::string>{});
    r.message = j.value("message", std::string{});
}

void ConflictReport::recompute_flags() {
    has_critical_conflict = false;
    has_warning_conflict = false;
    total_conflicts = records.size();
    for (const auto& record : records) {
        if (record.severity == ConflictSeverity::Critical) {
            has_critical_conflict = true;
        } else if (record.severity == ConflictSeverity::Warning) {
            has_warning_conflict = true;
        }
    }
}

void to_json(json& j, const ConflictReport& r) {
    j = json{
        {"has_critical_conflict", r.has_critical_conflict},
        {"has_warning_conflict", r.has_warning_conflict},
        {"total_conflicts", r.total_conflicts},
        {"records", r.records},
    };
}

void from_json(const json& j, ConflictReport& r) {
    r.has_critical_conflict = j.value("has_critical_conflict", false);
    r.has_warning_conflict = j.value("has_warning_conflict", false);
    r.total_conflicts = j.value("total_conflicts", std::size_t{0});
    r.records = j.value("records", std::vector<ConflictRecord>{});
}

// ---------------------------------------------------------------------------
// DeployMapping / DeployPlan
// ---------------------------------------------------------------------------

void to_json(json& j, const DeployMapping& m) {
    j = json{
        {"target_relative_path", m.target_relative_path},
        {"source_path", m.source_path.string()},
        {"owner_mod_id", m.owner_mod_id},
        {"priority", m.priority},
        {"shadowed_mods", m.shadowed_mods},
    };
}

void from_json(const json& j, DeployMapping& m) {
    m.target_relative_path = j.value("target_relative_path", std::string{});
    m.source_path = fs::path(j.value("source_path", std::string{}));
    m.owner_mod_id = j.value("owner_mod_id", std::string{});
    m.priority = read_priority(j);
    m.shadowed_mods = j.value("shadowed_mods", std::vector<std::string>{});
}

void to_json(json& j, const DeployPlan& p) {
    j = json{
        {"active_profile", p.active_profile},
        {"timestamp", p.timestamp},
        {"mappings", p.mappings},
        {"conflict_report", p.conflict_report},
    };
}

void from_json(const json& j, DeployPlan& p) {
    p.active_profile = j.value("active_profile", std::string{"default"});
    p.timestamp = j.value("timestamp", uint64_t{0});
    p.mappings = j.value("mappings", std::vector<DeployMapping>{});
    if (const auto it = j.find("conflict_report"); it != j.end()) {
        p.conflict_report = it->get<ConflictReport>();
    } else {
        p.conflict_report = ConflictReport{};
    }
}

// ---------------------------------------------------------------------------
// Presets
// ---------------------------------------------------------------------------

void to_json(json& j, const ModPresetEntry& e) {
    j = json{{"mod_id", e.mod_id}, {"priority", e.priority}};
}

void from_json(const json& j, ModPresetEntry& e) {
    e.mod_id = j.value("mod_id", std::string{});
    e.priority = read_priority(j);
}

void to_json(json& j, const ModPreset& p) {
    j = json{
        {"id", p.id},
        {"name", p.name},
        {"created_at", p.created_at},
        {"updated_at", p.updated_at},
        {"mods", p.mods},
    };
    write_optional(j, "description", p.description);
    write_optional(j, "name_en", p.name_en);
    write_optional(j, "description_en", p.description_en);
}

void from_json(const json& j, ModPreset& p) {
    p.id = j.value("id", std::string{});
    p.name = j.value("name", std::string{});
    read_optional(j, "description", p.description);
    read_optional(j, "name_en", p.name_en);
    read_optional(j, "description_en", p.description_en);
    p.created_at = j.value("created_at", uint64_t{0});
    p.updated_at = j.value("updated_at", uint64_t{0});
    p.mods = j.value("mods", std::vector<ModPresetEntry>{});
}

// ---------------------------------------------------------------------------
// StagedMod and shared helpers
// ---------------------------------------------------------------------------

uint64_t StagedMod::total_bytes() const {
    uint64_t total = 0;
    for (const auto& asset : assets) {
        total += asset.file_size;
    }
    return total;
}

std::string format_bytes(uint64_t bytes) {
    constexpr uint64_t kKb = 1024ULL;
    constexpr uint64_t kMb = kKb * 1024ULL;
    constexpr uint64_t kGb = kMb * 1024ULL;

    char buffer[64] = {};
    if (bytes >= kGb) {
        std::snprintf(buffer, sizeof(buffer), "%.2f GB", static_cast<double>(bytes) / static_cast<double>(kGb));
    } else if (bytes >= kMb) {
        std::snprintf(buffer, sizeof(buffer), "%.2f MB", static_cast<double>(bytes) / static_cast<double>(kMb));
    } else if (bytes >= kKb) {
        std::snprintf(buffer, sizeof(buffer), "%.2f KB", static_cast<double>(bytes) / static_cast<double>(kKb));
    } else {
        std::snprintf(buffer, sizeof(buffer), "%llu B", static_cast<unsigned long long>(bytes));
    }
    return std::string(buffer);
}

uint64_t now_epoch_seconds() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(now).count());
}

} // namespace smm
