#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "types.hpp"

namespace smm {

/// Severity of a single environment check.
enum class DiagnosticStatus { Ok, Info, Warning, Error };

std::string to_string(DiagnosticStatus status);
DiagnosticStatus diagnostic_status_from_string(std::string_view text);

/// Overall environment verdict, derived from the worst item.
enum class OverallHealth { Healthy, Degraded, ActionRequired };

std::string to_string(OverallHealth health);
OverallHealth overall_health_from_string(std::string_view text);

/// One line of the environment report.
struct DiagnosticItem {
    std::string id;       ///< Stable key, e.g. "sekiro-exe"
    std::string category; ///< Grouping for the UI: "game", "modengine", "staging", ...
    std::string title;    ///< Human readable label
    std::string detail;   ///< What was actually observed
    DiagnosticStatus status{DiagnosticStatus::Info};
    std::optional<std::string> remediation; ///< What the user should do about it
};

void to_json(json& j, const DiagnosticItem& i);
void from_json(const json& j, DiagnosticItem& i);

/// The full environment report.
struct HealthReport {
    OverallHealth overall{OverallHealth::Healthy};
    fs::path game_dir;
    fs::path staging_dir;
    std::vector<DiagnosticItem> items; ///< Sorted errors -> warnings -> info -> ok
    std::size_t ok_count{0};
    std::size_t info_count{0};
    std::size_t warning_count{0};
    std::size_t error_count{0};

    /// Derives overall health from the worst item and refills the counters.
    void recompute();

    bool is_healthy() const { return overall == OverallHealth::Healthy; }
};

void to_json(json& j, const HealthReport& r);

/// A real ModEngine dinput8.dll is several hundred kilobytes. Anything at or below this is a
/// stub or a placeholder and must never be installed as the hook.
inline constexpr uint64_t MOD_ENGINE_DLL_MIN_SIZE = 10'000;

/// Actionable guidance for obtaining ModEngine.
///
/// SMM deliberately neither embeds nor redistributes ModEngine: upstream publishes no license
/// and reserves all rights while permitting redistribution only of an unmodified copy bundled
/// with a mod. SMM is a mod manager, not a mod, so it cannot rely on that grant.
extern const std::string_view MOD_ENGINE_SOURCE_HINT;

/// Parsed \`modengine.ini\`.
///
/// The whole file is kept verbatim in \`raw_content\` so patching only ever rewrites the keys SMM
/// owns and never destroys a user's comments, ordering or unknown keys.
struct ModEngineConfig {
    fs::path path;
    std::string raw_content;
    std::optional<bool> enabled;
    std::optional<bool> load_uxm_files;
    std::optional<bool> cache_paths;
    std::optional<bool> load_loose_params;
    std::optional<std::string> mod_override_directory;
};

/// Reads and parses \`modengine.ini\`. A missing file yields a default config rather than throwing.
ModEngineConfig parse_modengine_ini(const fs::path& ini_path);

/// Parses ini text directly (exposed for tests and for the patch round-trip).
ModEngineConfig parse_modengine_ini_text(std::string_view content);

/// Renders the canonical SMM-generated ini.
std::string render_default_modengine_ini(std::string_view mod_override_dir);

/// Forces \`enabled=1\`, \`loadLooseParams=1\` and \`modOverrideDirectory\` to \`mods_dir\` while
/// leaving every other line of an existing file untouched. Returns the new file content.
std::string patch_modengine_ini_text(std::optional<std::string_view> existing,
                                     std::string_view mods_dir);

/// Applies patch_modengine_ini_text() to disk, creating the file when missing.
void patch_or_create_modengine_ini(const fs::path& ini_path, const fs::path& mods_dir);

/// Outcome of installing the ModEngine loader into the game directory.
struct EngineProvisionResult {
    bool installed_dinput8{false};
    fs::path dinput8_path;
    fs::path ini_path;
    fs::path source_used; ///< Where the dinput8.dll payload came from
    std::vector<std::string> warnings;
};

void to_json(json& j, const EngineProvisionResult& r);

/// Copies a user-supplied ModEngine payload (dinput8.dll) into the game root, writes
/// modengine.ini, and ensures the mods directory exists.
///
/// A genuine DLL already in the game directory is kept, which makes this double as a repair
/// path. Throws SmmError(ModEngineSourceMissing) when no payload can be found - SMM never
/// fabricates or embeds a hook DLL.
EngineProvisionResult provision_mod_engine(const fs::path& game_dir,
                                           const fs::path& source_or_staging = {});

HealthReport diagnose_environment(const fs::path& game_dir, const fs::path& staging_dir);

/// Best-effort location of the Sekiro installation.
///
/// Order: \`hint\` when it contains sekiro.exe, then the directory of the running executable,
/// then every Steam library declared in libraryfolders.vdf. Returns an empty path when nothing
/// was found; never throws.
fs::path detect_game_dir(const fs::path& hint = {});

/// Best-effort staging directory: env SMM_STAGING, then ./staging found by walking up from the
/// working directory, then %LOCALAPPDATA%/smm/staging.
fs::path detect_staging_dir(const fs::path& hint = {});

} // namespace smm
