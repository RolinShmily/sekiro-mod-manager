#pragma once

#include <nlohmann/json.hpp>

#include "cli_parser.hpp"
#include "smm/loader.hpp"
#include "smm/types.hpp"

namespace smm::cli {

/// Machine-readable projection of a mod. Every command that returns mods uses this exact shape,
/// so the GUI has one decoder for all of them.
nlohmann::json mod_json(const StagedMod& mod);

/// Machine-readable projection of one asset.
nlohmann::json asset_json(const AssetEntry& asset);

/// Machine-readable projection of a conflict record.
nlohmann::json conflict_json(const ConflictRecord& record);

/// Human-readable table shared by \`list\` and any other command that enumerates mods.
void print_mod_table(const CliContext& ctx, const std::vector<StagedMod>& mods);

/// Prints the failures a staging scan produced, which would otherwise be invisible.
void report_scan_failures(const CliContext& ctx, const ScanOutcome& outcome);

nlohmann::json run_list(CliContext& ctx);
nlohmann::json run_info(CliContext& ctx);
nlohmann::json run_enable(CliContext& ctx, bool enabled);
nlohmann::json run_toggle_asset(CliContext& ctx, bool enabled);
nlohmann::json run_set_preview(CliContext& ctx);
nlohmann::json run_priority(CliContext& ctx);
nlohmann::json run_remove(CliContext& ctx);
nlohmann::json run_conflicts(CliContext& ctx);
nlohmann::json run_env(CliContext& ctx);
nlohmann::json run_config(CliContext& ctx);

nlohmann::json run_plan(CliContext& ctx, bool execute);
nlohmann::json run_restore(CliContext& ctx);

nlohmann::json run_import(CliContext& ctx);
nlohmann::json run_import_pack(CliContext& ctx);

nlohmann::json run_export(CliContext& ctx);

nlohmann::json run_doctor(CliContext& ctx);
nlohmann::json run_setup_engine(CliContext& ctx);

nlohmann::json run_presets(CliContext& ctx);

} // namespace smm::cli
