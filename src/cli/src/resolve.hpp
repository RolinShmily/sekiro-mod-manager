#pragma once

#include <filesystem>
#include <string>

#include "cli_parser.hpp"

namespace smm::cli {

namespace fs = std::filesystem;

/// Persisted user settings.
///
/// The CLI owns this file, not the GUI: with the layered design the CLI is the single source of
/// truth for configuration, and the GUI simply reads it back through \`smm env\`.
struct CliSettings {
    fs::path staging_dir;
    fs::path game_dir;
};

/// Location of settings.json, honouring SMM_SETTINGS_FILE when set (used by tests and by a
/// portable install that wants everything self-contained).
fs::path settings_path();

CliSettings load_settings();
void save_settings(const CliSettings& settings);

/// Resolves the staging directory: explicit --staging, then saved settings, then auto-detection.
/// Never returns an empty path; the directory itself is not required to exist yet.
fs::path resolve_staging_dir(const CliContext& ctx);

/// Resolves the game directory the same way. May return an empty path when Sekiro cannot be
/// located, which callers report as an actionable environment problem rather than an error.
fs::path resolve_game_dir(const CliContext& ctx);

/// Creates the resolved staging directory and returns it.
fs::path ensure_staging_dir(const CliContext& ctx);

} // namespace smm::cli
