#include "resolve.hpp"

#include <cstdlib>
#include <fstream>
#include <system_error>

#include "smm/doctor.hpp"
#include "smm/error.hpp"

namespace smm::cli {
namespace {

fs::path default_settings_root() {
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA"); appdata != nullptr && *appdata != '\0') {
        return fs::path(appdata) / "SMM";
    }
    if (const char* local = std::getenv("LOCALAPPDATA"); local != nullptr && *local != '\0') {
        return fs::path(local) / "SMM";
    }
#else
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        return fs::path(home) / ".config" / "smm";
    }
#endif
    std::error_code ec;
    return fs::current_path(ec) / ".smm";
}

} // namespace

fs::path settings_path() {
    if (const char* override_path = std::getenv("SMM_SETTINGS_FILE");
        override_path != nullptr && *override_path != '\0') {
        return fs::path(override_path);
    }
    return default_settings_root() / "settings.json";
}

CliSettings load_settings() {
    CliSettings settings;

    const fs::path path = settings_path();
    std::error_code ec;
    if (!fs::is_regular_file(path, ec)) {
        return settings;
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return settings;
    }

    json parsed;
    try {
        stream >> parsed;
    } catch (const json::exception&) {
        // A corrupt settings file must not make every command fail; the user can simply set the
        // paths again. Losing the file's contents is reported by 'smm env' showing no paths.
        return settings;
    }

    if (parsed.is_object()) {
        if (const auto it = parsed.find("staging_dir"); it != parsed.end() && it->is_string()) {
            settings.staging_dir = it->get<std::string>();
        }
        if (const auto it = parsed.find("game_dir"); it != parsed.end() && it->is_string()) {
            settings.game_dir = it->get<std::string>();
        }
    }
    return settings;
}

void save_settings(const CliSettings& settings) {
    const fs::path path = settings_path();
    std::error_code ec;
    if (!path.parent_path().empty()) {
        fs::create_directories(path.parent_path(), ec);
    }

    const json payload{
        {"staging_dir", settings.staging_dir.string()},
        {"game_dir", settings.game_dir.string()},
    };

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        fail(ErrorCode::Io, "Cannot write the settings file", path);
    }
    stream << payload.dump(2);
    stream.flush();
    if (!stream) {
        fail(ErrorCode::Io, "Failed to write the settings file", path);
    }
}

fs::path resolve_staging_dir(const CliContext& ctx) {
    if (ctx.has_option("--staging")) {
        return fs::path(ctx.option("--staging"));
    }

    const CliSettings settings = load_settings();
    if (!settings.staging_dir.empty()) {
        return settings.staging_dir;
    }

    // detect_staging_dir() always produces something usable, so there is no empty-path case to
    // report here: on a first run SMM creates the folder it names.
    return detect_staging_dir();
}

fs::path resolve_game_dir(const CliContext& ctx) {
    if (ctx.has_option("--game-dir")) {
        return fs::path(ctx.option("--game-dir"));
    }

    const CliSettings settings = load_settings();
    if (!settings.game_dir.empty()) {
        return settings.game_dir;
    }

    return detect_game_dir();
}

fs::path ensure_staging_dir(const CliContext& ctx) {
    const fs::path staging = resolve_staging_dir(ctx);
    if (staging.empty()) {
        fail(ErrorCode::EnvironmentError,
             "No staging directory could be determined. Pass --staging <path> or set it with "
             "'smm config --staging <path>'.");
    }

    std::error_code ec;
    fs::create_directories(staging, ec);
    if (ec) {
        fail(ErrorCode::Io, "Cannot create the staging directory: " + ec.message(), staging);
    }
    return staging;
}

} // namespace smm::cli
