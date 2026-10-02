#include "commands.hpp"

#include <algorithm>
#include <iostream>

#include "json_output.hpp"
#include "resolve.hpp"
#include "table.hpp"
#include "terminal.hpp"

#include "smm/conflict.hpp"
#include "smm/doctor.hpp"
#include "smm/error.hpp"
#include "smm/loader.hpp"
#include "smm/manager.hpp"
#include "smm/normalizer.hpp"

namespace smm::cli {
namespace {

std::string status_label(const ModInfo& info, bool use_color) {
    return info.enabled ? terminal::green("enabled", use_color)
                        : terminal::dim("disabled", use_color);
}

std::string severity_label(ConflictSeverity severity, bool use_color) {
    switch (severity) {
        case ConflictSeverity::Critical: return terminal::red("critical", use_color);
        case ConflictSeverity::Warning: return terminal::yellow("warning", use_color);
        case ConflictSeverity::Info: return terminal::dim("info", use_color);
    }
    return {};
}

/// Asks before an irreversible action. Never prompts when stdout is not a terminal, because a
/// GUI or a script would otherwise block forever waiting for input nobody can type.
bool confirm(const CliContext& ctx, const std::string& question) {
    if (ctx.assume_yes) {
        return true;
    }
    if (ctx.json_mode || !terminal::stdout_is_terminal()) {
        fail(ErrorCode::InvalidArgument,
             "Refusing to continue without confirmation. Re-run with --yes to proceed.");
    }

    std::cout << question << " [y/N] " << std::flush;
    std::string answer;
    std::getline(std::cin, answer);
    return !answer.empty() && (answer.front() == 'y' || answer.front() == 'Y');
}

} // namespace

nlohmann::json mod_json(const StagedMod& mod) {
    return nlohmann::json{
        {"id", mod.info.id},
        {"name", mod.info.name},
        {"version", mod.info.version},
        {"author", mod.info.author},
        {"category", mod.info.category},
        {"description", mod.info.description ? nlohmann::json(*mod.info.description)
                                             : nlohmann::json(nullptr)},
        {"homepage", mod.info.homepage ? nlohmann::json(*mod.info.homepage)
                                       : nlohmann::json(nullptr)},
        {"source_url", mod.info.source_url ? nlohmann::json(*mod.info.source_url)
                                           : nlohmann::json(nullptr)},
        {"license", mod.info.license ? nlohmann::json(*mod.info.license)
                                     : nlohmann::json(nullptr)},
        {"enabled", mod.info.enabled},
        {"priority", mod.info.priority},
        {"tags", mod.info.tags},
        {"disabled_assets", mod.info.disabled_assets},
        {"preview_image", mod.info.preview_image ? nlohmann::json(*mod.info.preview_image)
                                                : nlohmann::json(nullptr)},
        {"asset_count", mod.assets.size()},
        {"total_bytes", mod.total_bytes()},
        {"root_path", mod.info.root_path ? mod.info.root_path->string() : std::string{}},
    };
}

nlohmann::json asset_json(const AssetEntry& asset) {
    return nlohmann::json{
        {"relative_path", asset.relative_path},
        {"source_path", asset.source_path.string()},
        {"file_size", asset.file_size},
        {"category", to_string(asset.category)},
        {"is_critical", asset.is_critical},
        {"is_exclusive_slot", asset.is_exclusive_slot},
        {"enabled", asset.enabled},
    };
}

nlohmann::json conflict_json(const ConflictRecord& record) {
    return nlohmann::json{
        {"relative_path", record.relative_path},
        {"severity", to_string(record.severity)},
        {"winner_mod_id", record.winner_mod_id},
        {"shadowed_mod_ids", record.shadowed_mod_ids},
        {"message", record.message},
    };
}

void print_mod_table(const CliContext& ctx, const std::vector<StagedMod>& mods) {
    std::vector<std::vector<std::string>> rows;
    rows.reserve(mods.size());
    for (const auto& mod : mods) {
        rows.push_back({
            mod.info.id,
            mod.info.name.empty() ? terminal::dim("(unnamed)", ctx.use_color) : mod.info.name,
            mod.info.version,
            mod.info.category,
            std::to_string(mod.info.priority),
            status_label(mod.info, ctx.use_color),
            std::to_string(mod.assets.size()),
            format_bytes(mod.total_bytes()),
        });
    }

    std::cout << table::render({{"ID"}, {"Name"}, {"Version"}, {"Category"}, {"Pri", table::Align::Right},
                                {"Status"}, {"Assets", table::Align::Right}, {"Size", table::Align::Right}},
                               rows, ctx.use_color);
}

void report_scan_failures(const CliContext& ctx, const ScanOutcome& outcome) {
    for (const auto& failure : outcome.failures) {
        if (ctx.json_mode) {
            continue;
        }
        std::cout << terminal::yellow("warning", ctx.use_color) << " "
                  << failure.path.filename().string() << ": " << failure.message << "\n";
    }
}

nlohmann::json run_list(CliContext& ctx) {
    const fs::path staging = ensure_staging_dir(ctx);
    const ScanOutcome outcome = ModLoader::scan_mods_directory(staging);

    if (!ctx.json_mode) {
        std::cout << terminal::bold("Staging: " + staging.string(), ctx.use_color) << "\n\n";
        if (outcome.mods.empty()) {
            std::cout << terminal::dim("No mods staged yet. Import one with 'smm import <path>'.",
                                       ctx.use_color)
                      << "\n";
        } else {
            print_mod_table(ctx, outcome.mods);
            std::cout << "\n" << outcome.mods.size() << " mod(s), "
                      << std::count_if(outcome.mods.begin(), outcome.mods.end(),
                                       [](const StagedMod& m) { return m.info.enabled; })
                      << " enabled.\n";
        }
        report_scan_failures(ctx, outcome);
    }

    nlohmann::json mods = nlohmann::json::array();
    for (const auto& mod : outcome.mods) {
        mods.push_back(mod_json(mod));
    }
    nlohmann::json failures = nlohmann::json::array();
    for (const auto& failure : outcome.failures) {
        failures.push_back({{"path", failure.path.string()}, {"message", failure.message}});
    }

    return nlohmann::json{
        {"staging_dir", staging.string()},
        {"count", outcome.mods.size()},
        {"enabled_count", std::count_if(outcome.mods.begin(), outcome.mods.end(),
                                        [](const StagedMod& m) { return m.info.enabled; })},
        {"mods", mods},
        {"failures", failures},
    };
}

nlohmann::json run_info(CliContext& ctx) {
    const fs::path staging = ensure_staging_dir(ctx);
    const std::string mod_id = ctx.require_positional(0, "a mod id");
    const StagedMod mod = ModManager::get_mod_details(staging, mod_id);

    if (!ctx.json_mode) {
        const std::vector<std::pair<std::string, std::string>> fields = {
            {"ID", mod.info.id},
            {"Name", mod.info.name},
            {"Version", mod.info.version},
            {"Author", mod.info.author},
            {"Category", mod.info.category},
            {"Enabled", mod.info.enabled ? "yes" : "no"},
            {"Priority", std::to_string(mod.info.priority)},
            {"Assets", std::to_string(mod.assets.size())},
            {"Total size", format_bytes(mod.total_bytes())},
            {"Description", mod.info.description.value_or("")},
            {"Homepage", mod.info.homepage.value_or("")},
            {"Source", mod.info.source_url.value_or("")},
            {"License", mod.info.license.value_or("")},
            {"Path", mod.info.root_path ? mod.info.root_path->string() : std::string{}},
        };
        std::cout << table::render_key_values(fields, ctx.use_color) << "\n";

        std::vector<std::vector<std::string>> rows;
        rows.reserve(mod.assets.size());
        for (const auto& asset : mod.assets) {
            std::string flags;
            if (asset.is_critical) {
                flags += terminal::red("critical", ctx.use_color);
            }
            if (asset.is_exclusive_slot) {
                if (!flags.empty()) {
                    flags += " ";
                }
                flags += terminal::yellow("slot", ctx.use_color);
            }
            rows.push_back({asset.relative_path, to_string(asset.category),
                            format_bytes(asset.file_size), flags});
        }
        std::cout << table::render(
            {{"Asset"}, {"Category"}, {"Size", table::Align::Right}, {"Flags"}}, rows, ctx.use_color);
    }

    nlohmann::json assets = nlohmann::json::array();
    for (const auto& asset : mod.assets) {
        assets.push_back(asset_json(asset));
    }

    nlohmann::json result = mod_json(mod);
    result["assets"] = assets;
    return result;
}

nlohmann::json run_enable(CliContext& ctx, bool enabled) {
    const fs::path staging = ensure_staging_dir(ctx);
    const std::string mod_id = ctx.require_positional(0, "a mod id");
    const ModInfo info = ModManager::set_mod_enabled(staging, mod_id, enabled);
    const StagedMod fresh = ModManager::get_mod_details(staging, mod_id);

    if (!ctx.json_mode) {
        std::cout << (enabled ? terminal::green("Enabled", ctx.use_color)
                              : terminal::dim("Disabled", ctx.use_color))
                  << " " << info.name << " (" << info.id << ")\n";
    }
    return mod_json(fresh);
}

nlohmann::json run_toggle_asset(CliContext& ctx, bool enabled) {
    const fs::path staging = ensure_staging_dir(ctx);
    const std::string mod_id = ctx.require_positional(0, "a mod id");
    const std::string rel_path = ctx.require_positional(1, "an asset relative path");
    const ModInfo info = ModManager::set_asset_enabled(staging, mod_id, rel_path, enabled);
    const StagedMod fresh = ModManager::get_mod_details(staging, mod_id);

    if (!ctx.json_mode) {
        std::cout << (enabled ? terminal::green("Enabled asset", ctx.use_color)
                              : terminal::dim("Disabled asset", ctx.use_color))
                  << " '" << rel_path << "' in mod " << info.name << " (" << info.id << ")\n";
    }
    return mod_json(fresh);
}

nlohmann::json run_set_preview(CliContext& ctx) {
    const fs::path staging = ensure_staging_dir(ctx);
    const std::string mod_id = ctx.require_positional(0, "a mod id");
    const std::string img_path_str = ctx.require_positional(1, "an image file path");
    const fs::path img_path(img_path_str);
    const ModInfo info = ModManager::set_mod_preview(staging, mod_id, img_path);
    const StagedMod fresh = ModManager::get_mod_details(staging, mod_id);

    if (!ctx.json_mode) {
        std::cout << terminal::green("Updated preview image", ctx.use_color)
                  << " for " << info.name << " -> " << (info.preview_image ? *info.preview_image : "") << "\n";
    }
    return mod_json(fresh);
}

nlohmann::json run_priority(CliContext& ctx) {
    const fs::path staging = ensure_staging_dir(ctx);
    const std::string mod_id = ctx.require_positional(0, "a mod id");
    const std::string raw = ctx.require_positional(1, "a priority number");

    std::uint32_t priority = 0;
    try {
        const unsigned long parsed = std::stoul(raw);
        if (parsed > 0xFFFFFFFFUL) {
            fail(ErrorCode::InvalidArgument, "Priority is out of range: " + raw);
        }
        priority = static_cast<std::uint32_t>(parsed);
    } catch (const std::exception&) {
        fail(ErrorCode::InvalidArgument, "Priority must be a non-negative integer, got: " + raw);
    }

    const ModInfo info = ModManager::set_mod_priority(staging, mod_id, priority);
    const StagedMod fresh = ModManager::get_mod_details(staging, mod_id);

    if (!ctx.json_mode) {
        std::cout << "Priority of " << info.name << " is now " << info.priority
                  << " (lower wins).\n";
    }
    return mod_json(fresh);
}

nlohmann::json run_remove(CliContext& ctx) {
    const fs::path staging = ensure_staging_dir(ctx);
    const std::string mod_id = ctx.require_positional(0, "a mod id");
    const StagedMod mod = ModManager::get_mod_details(staging, mod_id);
    const nlohmann::json removed = mod_json(mod);

    if (!confirm(ctx, "Delete '" + mod.info.name + "' (" + mod.info.id +
                          ") and all of its files?")) {
        if (!ctx.json_mode) {
            std::cout << "Cancelled.\n";
        }
        return nlohmann::json{{"removed", false}, {"mod", removed}};
    }

    ModManager::delete_mod(staging, mod_id);

    if (!ctx.json_mode) {
        std::cout << terminal::green("Removed", ctx.use_color) << " " << mod.info.name << "\n";
    }
    return nlohmann::json{{"removed", true}, {"mod", removed}};
}

nlohmann::json run_conflicts(CliContext& ctx) {
    const fs::path staging = ensure_staging_dir(ctx);
    const ScanOutcome outcome = ModLoader::scan_mods_directory(staging);
    const ConflictReport report = ConflictEngine::scan_conflicts(outcome.mods);

    if (!ctx.json_mode) {
        if (report.records.empty()) {
            std::cout << terminal::green("No conflicts", ctx.use_color)
                      << " among the enabled mods.\n";
        } else {
            std::vector<std::vector<std::string>> rows;
            rows.reserve(report.records.size());
            for (const auto& record : report.records) {
                std::string shadowed;
                for (const auto& id : record.shadowed_mod_ids) {
                    if (!shadowed.empty()) {
                        shadowed += ", ";
                    }
                    shadowed += id;
                }
                rows.push_back({severity_label(record.severity, ctx.use_color),
                                record.relative_path, record.winner_mod_id, shadowed});
            }
            std::cout << table::render({{"Severity"}, {"Path"}, {"Winner"}, {"Shadowed"}}, rows, ctx.use_color);
            std::cout << "\n" << report.total_conflicts << " conflict(s)";
            if (report.has_critical_conflict) {
                std::cout << ", " << terminal::red("including critical", ctx.use_color);
            }
            std::cout << ".\n";
        }
        report_scan_failures(ctx, outcome);
    }

    nlohmann::json records = nlohmann::json::array();
    for (const auto& record : report.records) {
        records.push_back(conflict_json(record));
    }
    return nlohmann::json{
        {"has_critical_conflict", report.has_critical_conflict},
        {"has_warning_conflict", report.has_warning_conflict},
        {"total_conflicts", report.total_conflicts},
        {"records", records},
    };
}

nlohmann::json run_env(CliContext& ctx) {
    const fs::path settings_file = settings_path();
    const CliSettings settings = load_settings();
    const fs::path staging = resolve_staging_dir(ctx);
    const fs::path game = resolve_game_dir(ctx);

    std::error_code ec;
    const bool staging_exists = fs::is_directory(staging, ec);
    const bool game_exists = !game.empty() && fs::is_directory(game, ec);

    if (!ctx.json_mode) {
        std::cout << table::render_key_values({
                         {"Settings file", settings_file.string()},
                         {"Staging", staging.string() +
                                         (staging_exists ? "" : "  (missing)")},
                         {"Game directory", game.empty() ? "(not found)"
                                                         : game.string() +
                                                               (game_exists ? ""
                                                                            : "  (missing)")},
                         {"Saved staging", settings.staging_dir.string()},
                         {"Saved game dir", settings.game_dir.string()},
                     }, ctx.use_color);
    }

    return nlohmann::json{
        {"settings_file", settings_file.string()},
        {"staging_dir", staging.string()},
        {"staging_exists", staging_exists},
        {"game_dir", game.string()},
        {"game_dir_found", !game.empty()},
        {"game_dir_exists", game_exists},
        {"saved_staging_dir", settings.staging_dir.string()},
        {"saved_game_dir", settings.game_dir.string()},
    };
}

nlohmann::json run_config(CliContext& ctx) {
    CliSettings settings = load_settings();

    if (ctx.has_option("--staging")) {
        settings.staging_dir = fs::path(ctx.option("--staging"));
    }
    if (ctx.has_option("--game-dir")) {
        settings.game_dir = fs::path(ctx.option("--game-dir"));
    }
    if (!ctx.has_option("--staging") && !ctx.has_option("--game-dir")) {
        fail(ErrorCode::InvalidArgument,
             "Nothing to change. Pass --staging <path> and/or --game-dir <path>.");
    }

    save_settings(settings);

    if (!ctx.json_mode) {
        std::cout << table::render_key_values({
            {"Settings file", settings_path().string()},
            {"Staging", settings.staging_dir.string()},
            {"Game directory", settings.game_dir.string()},
        }, ctx.use_color);
    }

    return nlohmann::json{
        {"settings_file", settings_path().string()},
        {"staging_dir", settings.staging_dir.string()},
        {"game_dir", settings.game_dir.string()},
    };
}

} // namespace smm::cli
