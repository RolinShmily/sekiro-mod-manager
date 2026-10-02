#include "smm/doctor.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <map>
#include <system_error>

#include "smm/error.hpp"
#include "smm/executor.hpp"
#include "smm/loader.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace smm {

const std::string_view MOD_ENGINE_SOURCE_HINT =
    "Sekiro Mod Engine (dinput8.dll) is third-party software by katalash that SMM does not "
    "redistribute. Download it from https://www.nexusmods.com/sekiro/mods/6 (or "
    "https://github.com/katalash/ModEngine), then place a genuine dinput8.dll (larger than "
    "10 KB) in a mod-engine folder inside your staging directory. SMM never fabricates or "
    "embeds a hook DLL.";

namespace {

std::string to_lower_ascii(std::string_view text) {
    std::string lower;
    lower.reserve(text.size());
    for (const unsigned char c : text) {
        lower.push_back(static_cast<char>(std::tolower(c)));
    }
    return lower;
}

bool equals_ignore_case(std::string_view a, std::string_view b) {
    return a.size() == b.size() && to_lower_ascii(a) == to_lower_ascii(b);
}

bool is_truthy(std::string_view value) {
    return value == "1" || equals_ignore_case(value, "true") || equals_ignore_case(value, "yes");
}

/// A genuine ModEngine hook is hundreds of kilobytes; anything smaller is a stub.
bool is_valid_mod_engine_dll(const fs::path& path) {
    std::error_code ec;
    if (!fs::is_regular_file(path, ec) || ec) {
        return false;
    }
    return fs::file_size(path, ec) > MOD_ENGINE_DLL_MIN_SIZE && !ec;
}

/// Every place a user might reasonably have dropped a ModEngine payload.
std::vector<fs::path> dinput8_candidates(const fs::path& source_or_staging) {
    std::vector<fs::path> candidates;
    if (!source_or_staging.empty()) {
        candidates.push_back(source_or_staging / "dinput8.dll");
        candidates.push_back(source_or_staging / "mod-engine" / "dinput8.dll");
        candidates.push_back(source_or_staging / "mod-engine-0.1.16" / "dinput8.dll");
    }
#ifdef _WIN32
    // Beside the running executable is where a portable SMM install keeps its tools.
    wchar_t module_path[MAX_PATH] = {};
    if (::GetModuleFileNameW(nullptr, module_path, MAX_PATH) != 0) {
        const fs::path exe_dir = fs::path(module_path).parent_path();
        candidates.push_back(exe_dir / "tools" / "mod-engine" / "dinput8.dll");
        candidates.push_back(exe_dir / "mod-engine" / "dinput8.dll");
    }
#endif
    candidates.emplace_back("staging/mod-engine/dinput8.dll");
    candidates.emplace_back("staging/mod-engine-0.1.16/dinput8.dll");
    candidates.emplace_back("dinput8.dll");
    return candidates;
}

fs::path first_valid_dll(const std::vector<fs::path>& candidates) {
    const auto it = std::find_if(candidates.begin(), candidates.end(), is_valid_mod_engine_dll);
    return it == candidates.end() ? fs::path{} : *it;
}

/// Reads a REG_SZ value, returning an empty string when absent.
#ifdef _WIN32
std::string read_registry_string(HKEY root, const wchar_t* subkey, const wchar_t* value_name) {
    HKEY key = nullptr;
    if (::RegOpenKeyExW(root, subkey, 0, KEY_READ | KEY_WOW64_32KEY, &key) != ERROR_SUCCESS &&
        ::RegOpenKeyExW(root, subkey, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        return {};
    }

    wchar_t buffer[1024] = {};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    const LONG status = ::RegQueryValueExW(key, value_name, nullptr, &type,
                                           reinterpret_cast<LPBYTE>(buffer), &size);
    ::RegCloseKey(key);
    if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        return {};
    }
    return fs::path(buffer).string();
}
#endif

/// Extracts every `"path" "…"` value from Steam's libraryfolders.vdf.
/// Only the paths are needed; a full VDF parser would be far more machinery than the job needs.
std::vector<fs::path> parse_steam_library_paths(const fs::path& vdf_path) {
    std::vector<fs::path> libraries;
    std::ifstream stream(vdf_path, std::ios::binary);
    if (!stream) {
        return libraries;
    }

    std::string content((std::istreambuf_iterator<char>(stream)),
                        std::istreambuf_iterator<char>());
    std::size_t cursor = 0;
    while (true) {
        const std::size_t key_at = content.find("\"path\"", cursor);
        if (key_at == std::string::npos) {
            break;
        }
        const std::size_t open_quote = content.find('"', key_at + 6);
        if (open_quote == std::string::npos) {
            break;
        }
        const std::size_t close_quote = content.find('"', open_quote + 1);
        if (close_quote == std::string::npos) {
            break;
        }
        std::string value = content.substr(open_quote + 1, close_quote - open_quote - 1);
        // VDF escapes separators as \\, so collapse the doubled backslashes.
        std::string collapsed;
        collapsed.reserve(value.size());
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (value[i] == '\\' && i + 1 < value.size() && value[i + 1] == '\\') {
                collapsed.push_back('\\');
                ++i;
            } else {
                collapsed.push_back(value[i]);
            }
        }
        if (!collapsed.empty()) {
            libraries.emplace_back(collapsed);
        }
        cursor = close_quote + 1;
    }
    return libraries;
}

/// Adds the ModEngine-related items for one ini file.
void diagnose_modengine_ini(const fs::path& game_dir, const fs::path& ini_path,
                            std::vector<DiagnosticItem>& items) {
    std::error_code ec;
    if (!fs::is_regular_file(ini_path, ec)) {
        items.push_back(DiagnosticItem{
            "modengine-ini",
            "modengine",
            "modengine.ini",
            "The ModEngine configuration file is missing from the game directory.",
            DiagnosticStatus::Error,
            "Run 'smm setup-engine --game-dir <path>' to generate a standard configuration."});
        return;
    }

    const ModEngineConfig config = parse_modengine_ini(ini_path);

    items.push_back(DiagnosticItem{"modengine-ini", "modengine", "modengine.ini",
                                   "Configuration file found and readable.",
                                   DiagnosticStatus::Ok, std::nullopt});

    if (!config.enabled.has_value()) {
        items.push_back(DiagnosticItem{
            "modengine-enabled", "modengine", "enabled",
            "'enabled' is missing under [files], so the engine may not load mods at all.",
            DiagnosticStatus::Warning,
            "Add 'enabled=1' under [files] in modengine.ini."});
    } else if (!*config.enabled) {
        items.push_back(DiagnosticItem{
            "modengine-enabled", "modengine", "enabled",
            "ModEngine is switched off (enabled=0); loose mods will be ignored entirely.",
            DiagnosticStatus::Error,
            "Set 'enabled=1' under [files], or run 'smm setup-engine'."});
    } else {
        items.push_back(DiagnosticItem{"modengine-enabled", "modengine", "enabled",
                                       "The engine is enabled (enabled=1).", DiagnosticStatus::Ok,
                                       std::nullopt});
    }

    const std::string override_dir =
        config.mod_override_directory ? *config.mod_override_directory : std::string{};
    if (override_dir.empty()) {
        items.push_back(DiagnosticItem{
            "modengine-override", "modengine", "modOverrideDirectory",
            "'modOverrideDirectory' is missing or empty, so deployed mods would never be found.",
            DiagnosticStatus::Error,
            "Set modOverrideDirectory=\"\\\\mods\" in modengine.ini."});
    } else {
        std::string relative = override_dir;
        while (!relative.empty() && (relative.front() == '\\' || relative.front() == '/')) {
            relative.erase(relative.begin());
        }
        const fs::path target = game_dir / relative;
        const bool exists = fs::exists(target, ec);
        items.push_back(DiagnosticItem{
            "modengine-override", "modengine", "modOverrideDirectory",
            exists ? "Override directory '" + override_dir + "' exists on disk."
                   : "Override directory is set to '" + override_dir +
                         "' but the folder does not exist yet.",
            exists ? DiagnosticStatus::Ok : DiagnosticStatus::Warning,
            exists ? std::nullopt
                   : std::optional<std::string>(
                         "The folder is created automatically on the first 'smm deploy'.")});
    }

    if (!config.load_loose_params.has_value() || !*config.load_loose_params) {
        items.push_back(DiagnosticItem{
            "modengine-loose-params", "modengine", "loadLooseParams",
            "loadLooseParams is off or unset, so loose param files may be ignored.",
            DiagnosticStatus::Warning,
            "Set 'loadLooseParams=1' in modengine.ini to allow loose param modifications."});
    } else {
        items.push_back(DiagnosticItem{"modengine-loose-params", "modengine", "loadLooseParams",
                                       "Loose parameter overrides are enabled.",
                                       DiagnosticStatus::Ok, std::nullopt});
    }
}

} // namespace

std::string to_string(DiagnosticStatus status) {
    switch (status) {
        case DiagnosticStatus::Ok: return "ok";
        case DiagnosticStatus::Info: return "info";
        case DiagnosticStatus::Warning: return "warning";
        case DiagnosticStatus::Error: return "error";
    }
    return "info";
}

DiagnosticStatus diagnostic_status_from_string(std::string_view text) {
    if (text == "ok") return DiagnosticStatus::Ok;
    if (text == "warning") return DiagnosticStatus::Warning;
    if (text == "error") return DiagnosticStatus::Error;
    return DiagnosticStatus::Info;
}

std::string to_string(OverallHealth health) {
    switch (health) {
        case OverallHealth::Healthy: return "healthy";
        case OverallHealth::Degraded: return "degraded";
        case OverallHealth::ActionRequired: return "action_required";
    }
    return "healthy";
}

OverallHealth overall_health_from_string(std::string_view text) {
    if (text == "degraded") return OverallHealth::Degraded;
    if (text == "action_required") return OverallHealth::ActionRequired;
    return OverallHealth::Healthy;
}

void to_json(json& j, const DiagnosticItem& i) {
    j = json{
        {"id", i.id},
        {"category", i.category},
        {"title", i.title},
        {"detail", i.detail},
        {"status", to_string(i.status)},
    };
    if (i.remediation) {
        j["remediation"] = *i.remediation;
    }
}

void from_json(const json& j, DiagnosticItem& i) {
    i.id = j.value("id", std::string{});
    i.category = j.value("category", std::string{});
    i.title = j.value("title", std::string{});
    i.detail = j.value("detail", std::string{});
    i.status = diagnostic_status_from_string(j.value("status", std::string{"info"}));
    if (const auto it = j.find("remediation"); it != j.end() && it->is_string()) {
        i.remediation = it->get<std::string>();
    }
}

void to_json(json& j, const HealthReport& r) {
    j = json{
        {"overall", to_string(r.overall)},
        {"game_dir", r.game_dir.string()},
        {"staging_dir", r.staging_dir.string()},
        {"items", r.items},
        {"ok_count", r.ok_count},
        {"info_count", r.info_count},
        {"warning_count", r.warning_count},
        {"error_count", r.error_count},
    };
}

void HealthReport::recompute() {
    ok_count = 0;
    info_count = 0;
    warning_count = 0;
    error_count = 0;
    bool has_warning = false;
    bool has_error = false;

    for (const auto& item : items) {
        switch (item.status) {
            case DiagnosticStatus::Ok: ++ok_count; break;
            case DiagnosticStatus::Info: ++info_count; break;
            case DiagnosticStatus::Warning:
                ++warning_count;
                has_warning = true;
                break;
            case DiagnosticStatus::Error:
                ++error_count;
                has_error = true;
                break;
        }
    }

    overall = has_error ? OverallHealth::ActionRequired
                        : (has_warning ? OverallHealth::Degraded : OverallHealth::Healthy);

    const auto rank = [](DiagnosticStatus status) {
        switch (status) {
            case DiagnosticStatus::Error: return 0;
            case DiagnosticStatus::Warning: return 1;
            case DiagnosticStatus::Info: return 2;
            case DiagnosticStatus::Ok: return 3;
        }
        return 4;
    };
    std::stable_sort(items.begin(), items.end(), [&rank](const DiagnosticItem& a, const DiagnosticItem& b) {
        return rank(a.status) < rank(b.status);
    });
}

void to_json(json& j, const EngineProvisionResult& r) {
    j = json{
        {"installed_dinput8", r.installed_dinput8},
        {"dinput8_path", r.dinput8_path.string()},
        {"ini_path", r.ini_path.string()},
        {"source_used", r.source_used.string()},
        {"warnings", r.warnings},
    };
}

ModEngineConfig parse_modengine_ini_text(std::string_view content) {
    ModEngineConfig config;
    config.raw_content = std::string(content);

    std::string current_section;
    std::size_t cursor = 0;
    while (cursor <= content.size()) {
        const std::size_t end = content.find('\n', cursor);
        std::string line(content.substr(cursor, end == std::string_view::npos
                                                   ? std::string_view::npos
                                                   : end - cursor));
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        cursor = (end == std::string_view::npos) ? content.size() + 1 : end + 1;

        std::string trimmed = line;
        const std::size_t first = trimmed.find_first_not_of(" \t");
        if (first == std::string::npos) {
            continue;
        }
        const std::size_t last = trimmed.find_last_not_of(" \t");
        trimmed = trimmed.substr(first, last - first + 1);

        if (trimmed.empty() || trimmed.front() == ';' || trimmed.front() == '#') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            current_section = to_lower_ascii(
                std::string_view(trimmed).substr(1, trimmed.size() - 2));
            continue;
        }
        if (current_section != "files") {
            continue;
        }

        const std::size_t separator = trimmed.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = to_lower_ascii(
            std::string_view(trimmed).substr(0, separator));
        std::string value = trimmed.substr(separator + 1);

        // Inline comments and optional quotes are both common in hand-edited files.
        if (const std::size_t comment = value.find(';'); comment != std::string::npos) {
            value = value.substr(0, comment);
        }
        const std::size_t value_first = value.find_first_not_of(" \t");
        const std::size_t value_last = value.find_last_not_of(" \t");
        value = (value_first == std::string::npos)
                    ? std::string{}
                    : value.substr(value_first, value_last - value_first + 1);

        if (value.size() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }

        if (key == "enabled") {
            config.enabled = is_truthy(value);
        } else if (key == "loaduxmfiles") {
            config.load_uxm_files = is_truthy(value);
        } else if (key == "cachepaths") {
            config.cache_paths = is_truthy(value);
        } else if (key == "loadlooseparams") {
            config.load_loose_params = is_truthy(value);
        } else if (key == "modoverridedirectory") {
            config.mod_override_directory = value;
        }
    }

    return config;
}

ModEngineConfig parse_modengine_ini(const fs::path& ini_path) {
    ModEngineConfig config;
    config.path = ini_path;

    std::error_code ec;
    if (!fs::is_regular_file(ini_path, ec)) {
        return config;
    }

    std::ifstream stream(ini_path, std::ios::binary);
    if (!stream) {
        return config;
    }
    const std::string content((std::istreambuf_iterator<char>(stream)),
                              std::istreambuf_iterator<char>());
    config = parse_modengine_ini_text(content);
    config.path = ini_path;
    return config;
}

std::string render_default_modengine_ini(std::string_view mod_override_dir) {
    std::string value(mod_override_dir);
    if (value.empty()) {
        value = "\\mods";
    } else if (value.front() != '\\' && value.front() != '/') {
        value = "\\" + value;
    }

    return "; Sekiro Mod Engine configuration\r\n"
           "; Generated and maintained by Sekiro Mod Manager (SMM)\r\n"
           "\r\n"
           "[files]\r\n"
           "enabled=1\r\n"
           "loadUXMFiles=0\r\n"
           "cachePaths=1\r\n"
           "modOverrideDirectory=\"" +
           value +
           "\"\r\n"
           "loadLooseParams=1\r\n"
           "\r\n"
           "[debug]\r\n"
           "showDebugConsole=0\r\n"
           "logFile=\"modengine.log\"\r\n";
}

std::string patch_modengine_ini_text(std::optional<std::string_view> existing,
                                     std::string_view mods_dir) {
    std::string value(mods_dir);
    if (value.empty()) {
        value = "\\mods";
    } else if (value.front() != '\\' && value.front() != '/') {
        value = "\\" + value;
    }
    const std::string override_line = "modOverrideDirectory=\"" + value + "\"";

    if (!existing.has_value()) {
        return render_default_modengine_ini(value);
    }

    std::vector<std::string> lines;
    bool in_files_section = false;
    bool saw_files_section = false;
    bool wrote_enabled = false;
    bool wrote_override = false;
    bool wrote_loose_params = false;

    const auto flush_missing = [&] {
        if (!wrote_enabled) {
            lines.emplace_back("enabled=1");
            wrote_enabled = true;
        }
        if (!wrote_override) {
            lines.push_back(override_line);
            wrote_override = true;
        }
        if (!wrote_loose_params) {
            lines.emplace_back("loadLooseParams=1");
            wrote_loose_params = true;
        }
    };

    const std::string_view content = *existing;
    std::size_t cursor = 0;
    while (cursor <= content.size()) {
        const std::size_t end = content.find('\n', cursor);
        std::string line(content.substr(cursor, end == std::string_view::npos
                                                   ? std::string_view::npos
                                                   : end - cursor));
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        cursor = (end == std::string_view::npos) ? content.size() + 1 : end + 1;

        std::string trimmed = line;
        const std::size_t first = trimmed.find_first_not_of(" \t");
        const std::size_t last = trimmed.find_last_not_of(" \t");
        trimmed = (first == std::string::npos) ? std::string{} : trimmed.substr(first, last - first + 1);

        if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
            if (in_files_section) {
                // Entering a new section: the keys that never appeared must be appended to the
                // old one, otherwise they would silently land in the wrong section.
                flush_missing();
                in_files_section = false;
            }
            const std::string section = to_lower_ascii(std::string_view(trimmed).substr(1, trimmed.size() - 2));
            if (section == "files") {
                in_files_section = true;
                saw_files_section = true;
            }
            lines.push_back(line);
            continue;
        }

        if (in_files_section && !trimmed.empty() && trimmed.front() != ';' && trimmed.front() != '#') {
            const std::size_t separator = trimmed.find('=');
            if (separator != std::string::npos) {
                const std::string key = to_lower_ascii(std::string_view(trimmed).substr(0, separator));
                if (key == "enabled") {
                    lines.emplace_back("enabled=1");
                    wrote_enabled = true;
                    continue;
                }
                if (key == "modoverridedirectory") {
                    lines.push_back(override_line);
                    wrote_override = true;
                    continue;
                }
                if (key == "loadlooseparams") {
                    lines.emplace_back("loadLooseParams=1");
                    wrote_loose_params = true;
                    continue;
                }
            }
        }

        lines.push_back(line);
    }

    if (in_files_section) {
        flush_missing();
    } else if (!saw_files_section) {
        lines.emplace_back();
        lines.emplace_back("[files]");
        lines.emplace_back("enabled=1");
        lines.emplace_back("loadUXMFiles=0");
        lines.emplace_back("cachePaths=1");
        lines.push_back(override_line);
        lines.emplace_back("loadLooseParams=1");
    }

    std::string result;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) {
            result += "\r\n";
        }
        result += lines[i];
    }
    return result;
}

void patch_or_create_modengine_ini(const fs::path& ini_path, const fs::path& mods_dir) {
    // ModEngine resolves this value relative to the game root, so an absolute path is reduced
    // to the relative form the engine actually understands.
    std::string override_value = "\\mods";
    if (!mods_dir.empty()) {
        std::error_code ec;
        const fs::path ini_dir = ini_path.parent_path();
        if (!ini_dir.empty()) {
            const fs::path relative = fs::relative(mods_dir, ini_dir, ec);
            if (!ec && !relative.empty()) {
                override_value = relative.generic_string();
            } else {
                override_value = mods_dir.filename().string();
            }
        } else {
            override_value = mods_dir.filename().string();
        }
    }

    std::optional<std::string> existing;
    std::error_code ec;
    if (fs::is_regular_file(ini_path, ec)) {
        std::ifstream stream(ini_path, std::ios::binary);
        if (stream) {
            existing = std::string((std::istreambuf_iterator<char>(stream)),
                                   std::istreambuf_iterator<char>());
        }
    }

    const std::string patched = patch_modengine_ini_text(existing, override_value);

    std::ofstream out(ini_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        fail(ErrorCode::Io, "Cannot write modengine.ini", ini_path);
    }
    out << patched;
    out.flush();
    if (!out) {
        fail(ErrorCode::Io, "Failed to write modengine.ini", ini_path);
    }
}

EngineProvisionResult provision_mod_engine(const fs::path& game_dir,
                                           const fs::path& source_or_staging) {
    std::error_code ec;
    if (game_dir.empty()) {
        fail(ErrorCode::InvalidArgument, "A game directory is required to install ModEngine");
    }
    fs::create_directories(game_dir, ec);

    EngineProvisionResult result;
    result.dinput8_path = game_dir / "dinput8.dll";
    result.ini_path = game_dir / "modengine.ini";
    const fs::path mods_dir = game_dir / "mods";

    // A genuine DLL already in place is left alone, which makes this a safe repair command.
    if (is_valid_mod_engine_dll(result.dinput8_path)) {
        result.source_used = result.dinput8_path;
    } else {
        const fs::path source = first_valid_dll(dinput8_candidates(source_or_staging));
        if (source.empty()) {
            fail(ErrorCode::ModEngineSourceMissing, std::string(MOD_ENGINE_SOURCE_HINT),
                 game_dir);
        }
        fs::copy_file(source, result.dinput8_path, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            fail(ErrorCode::Io, "Failed to install dinput8.dll: " + ec.message(),
                 result.dinput8_path);
        }
        result.source_used = source;
        result.installed_dinput8 = true;
    }

    patch_or_create_modengine_ini(result.ini_path, mods_dir);

    fs::create_directories(mods_dir, ec);
    if (ec) {
        result.warnings.push_back("Could not create the mods directory: " + ec.message());
    }

    return result;
}

HealthReport diagnose_environment(const fs::path& game_dir, const fs::path& staging_dir) {
    HealthReport report;
    report.game_dir = game_dir;
    report.staging_dir = staging_dir;
    std::error_code ec;

    if (!fs::exists(game_dir, ec)) {
        report.items.push_back(DiagnosticItem{
            "game-dir", "game", "Game Directory",
            "The game directory does not exist: " + game_dir.string(), DiagnosticStatus::Error,
            "Point SMM at the folder that contains sekiro.exe."});
    } else if (!fs::is_directory(game_dir, ec)) {
        report.items.push_back(DiagnosticItem{
            "game-dir", "game", "Game Directory",
            "The configured game path is not a directory: " + game_dir.string(),
            DiagnosticStatus::Error, "Point SMM at the folder that contains sekiro.exe."});
    } else {
        report.items.push_back(DiagnosticItem{"game-dir", "game", "Game Directory",
                                              "Game directory found.", DiagnosticStatus::Ok,
                                              std::nullopt});

        bool exe_found = false;
        for (const auto& entry : fs::directory_iterator(
                 game_dir, fs::directory_options::skip_permission_denied, ec)) {
            if (equals_ignore_case(entry.path().filename().string(), "sekiro.exe")) {
                exe_found = true;
                break;
            }
        }
        if (exe_found) {
            report.items.push_back(DiagnosticItem{"sekiro-exe", "game", "sekiro.exe",
                                                  "The game executable was found.",
                                                  DiagnosticStatus::Ok, std::nullopt});
        } else {
            report.items.push_back(DiagnosticItem{
                "sekiro-exe", "game", "sekiro.exe",
                "sekiro.exe was not found in the game directory.", DiagnosticStatus::Error,
                "Make sure the path points at steamapps/common/Sekiro, not its parent."});
        }
    }

    const fs::path dinput8 = game_dir / "dinput8.dll";
    if (is_valid_mod_engine_dll(dinput8)) {
        report.items.push_back(DiagnosticItem{
            "dinput8", "modengine", "dinput8.dll",
            "The ModEngine hook is installed (" +
                format_bytes(static_cast<uint64_t>(fs::file_size(dinput8, ec))) + ").",
            DiagnosticStatus::Ok, std::nullopt});
    } else if (fs::exists(dinput8, ec)) {
        report.items.push_back(DiagnosticItem{
            "dinput8", "modengine", "dinput8.dll",
            "dinput8.dll is present but far too small to be the real hook; it looks like a stub.",
            DiagnosticStatus::Warning, std::string(MOD_ENGINE_SOURCE_HINT)});
    } else {
        report.items.push_back(DiagnosticItem{
            "dinput8", "modengine", "dinput8.dll",
            "The ModEngine hook library is missing from the game directory.",
            DiagnosticStatus::Error, std::string(MOD_ENGINE_SOURCE_HINT)});
    }

    diagnose_modengine_ini(game_dir, game_dir / "modengine.ini", report.items);

    if (staging_dir.empty()) {
        report.items.push_back(DiagnosticItem{
            "staging", "staging", "Staging Directory",
            "No staging directory is configured.", DiagnosticStatus::Warning,
            "Choose a staging folder in Settings so SMM knows where your mods live."});
    } else if (!fs::exists(staging_dir, ec)) {
        report.items.push_back(DiagnosticItem{
            "staging", "staging", "Staging Directory",
            "The staging directory does not exist yet: " + staging_dir.string(),
            DiagnosticStatus::Warning,
            "Import a mod, or create the folder, and SMM will use it."});
    } else {
        report.items.push_back(DiagnosticItem{"staging", "staging", "Staging Directory",
                                              "Staging directory found.", DiagnosticStatus::Ok,
                                              std::nullopt});

        if (is_same_volume(game_dir, staging_dir)) {
            report.items.push_back(DiagnosticItem{
                "hardlink-volume", "staging", "Hard Link Volume",
                "Staging and the game share a volume, so deployment is instant and free of "
                "duplicated bytes.",
                DiagnosticStatus::Ok, std::nullopt});
        } else {
            report.items.push_back(DiagnosticItem{
                "hardlink-volume", "staging", "Hard Link Volume",
                "Staging and the game are on different volumes; deployment will physically "
                "copy every file.",
                DiagnosticStatus::Warning,
                "Move the staging directory onto the same drive as the game to get zero-copy "
                "hard links."});
        }

        try {
            const ScanOutcome outcome = ModLoader::scan_mods_directory(staging_dir);
            std::string detail = std::to_string(outcome.mods.size()) + " mod(s) staged";
            std::size_t enabled = 0;
            for (const auto& mod : outcome.mods) {
                if (mod.info.enabled) {
                    ++enabled;
                }
            }
            detail += ", " + std::to_string(enabled) + " enabled";
            if (!outcome.failures.empty()) {
                detail += ", " + std::to_string(outcome.failures.size()) +
                          " unreadable (see the mod list)";
            }
            report.items.push_back(DiagnosticItem{
                "staging-mods", "staging", "Staged Mods", detail,
                outcome.failures.empty() ? DiagnosticStatus::Ok : DiagnosticStatus::Warning,
                outcome.failures.empty()
                    ? std::nullopt
                    : std::optional<std::string>(
                          "Re-import the mods reported as unreadable; their metadata is broken.")});
        } catch (const SmmError& e) {
            report.items.push_back(DiagnosticItem{"staging-mods", "staging", "Staged Mods",
                                                  e.what(), DiagnosticStatus::Warning,
                                                  std::nullopt});
        }
    }

    report.recompute();
    return report;
}

fs::path detect_game_dir(const fs::path& hint) {
    std::error_code ec;

    // A hint is authoritative: the user pointed at a specific folder on purpose.
    if (!hint.empty()) {
        if (equals_ignore_case(hint.filename().string(), "sekiro.exe")) {
            return hint.parent_path();
        }
        if (fs::exists(hint / "sekiro.exe", ec) || fs::exists(hint / "Sekiro.exe", ec)) {
            return hint;
        }
    }

#ifdef _WIN32
    // A portable install usually sits next to or inside the game folder.
    wchar_t module_path[MAX_PATH] = {};
    if (::GetModuleFileNameW(nullptr, module_path, MAX_PATH) != 0) {
        const fs::path exe_dir = fs::path(module_path).parent_path();
        if (fs::exists(exe_dir / "sekiro.exe", ec)) {
            return exe_dir;
        }
        if (const fs::path parent = exe_dir.parent_path();
            !parent.empty() && fs::exists(parent / "sekiro.exe", ec)) {
            return parent;
        }
    }

    std::vector<fs::path> steam_roots;
    const std::string steam_path =
        read_registry_string(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath");
    if (!steam_path.empty()) {
        steam_roots.emplace_back(steam_path);
    }
    const std::string install_path =
        read_registry_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath");
    if (!install_path.empty()) {
        steam_roots.emplace_back(install_path);
    }

    for (const fs::path& root : steam_roots) {
        std::vector<fs::path> libraries{root};
        for (const fs::path& extra : parse_steam_library_paths(root / "steamapps" / "libraryfolders.vdf")) {
            libraries.push_back(extra);
        }
        for (const fs::path& library : libraries) {
            const fs::path candidate = library / "steamapps" / "common" / "Sekiro";
            if (fs::exists(candidate / "sekiro.exe", ec)) {
                return candidate;
            }
        }
    }
#endif

    return {};
}

fs::path detect_staging_dir(const fs::path& hint) {
    std::error_code ec;

    if (!hint.empty() && fs::is_directory(hint, ec)) {
        return hint;
    }

    if (const char* env = std::getenv("SMM_STAGING"); env != nullptr && *env != '\0') {
        const fs::path from_env(env);
        if (fs::is_directory(from_env, ec)) {
            return from_env;
        }
    }

    // Walk up from the working directory so running SMM from a subfolder still finds the
    // staging directory that belongs to the repository or install above it.
    fs::path current = fs::current_path(ec);
    while (!current.empty()) {
        const fs::path candidate = current / "staging";
        if (fs::is_directory(candidate, ec)) {
            return candidate;
        }
        const fs::path parent = current.parent_path();
        if (parent == current) {
            break;
        }
        current = parent;
    }

#ifdef _WIN32
    if (const char* local = std::getenv("LOCALAPPDATA"); local != nullptr && *local != '\0') {
        return fs::path(local) / "smm" / "staging";
    }
#endif

    return fs::current_path(ec) / "staging";
}

} // namespace smm
