#include "commands.hpp"

#include <algorithm>
#include <iostream>

#include "json_output.hpp"
#include "resolve.hpp"
#include "table.hpp"
#include "terminal.hpp"

#include "smm/conflict.hpp"
#include "smm/deploy.hpp"
#include "smm/error.hpp"
#include "smm/executor.hpp"
#include "smm/loader.hpp"

namespace smm::cli {
namespace {

/// The directory ModEngine actually reads. Keeping this in one place means deploy and restore
/// can never disagree about where assets live.
fs::path target_mods_dir(const fs::path& game_dir) { return game_dir / "mods"; }

fs::path require_game_dir(const CliContext& ctx) {
    const fs::path game = resolve_game_dir(ctx);
    if (game.empty()) {
        fail(ErrorCode::EnvironmentError,
             "The Sekiro installation could not be located. Pass --game-dir <path> or save it "
             "with 'smm config --game-dir <path>'.");
    }
    std::error_code ec;
    if (!fs::is_directory(game, ec)) {
        fail(ErrorCode::EnvironmentError,
             "The configured game directory does not exist: " + game.string(), game);
    }
    return game;
}

std::string profile_of(const CliContext& ctx) {
    const std::string profile = ctx.option("--profile", "default");
    return profile.empty() ? "default" : profile;
}

void print_plan(const CliContext& ctx, const DeployPlan& plan, const fs::path& target) {
    std::cout << terminal::bold("Deployment plan", ctx.use_color) << "  profile="
              << plan.active_profile << "  target=" << target.string() << "\n\n";

    if (plan.mappings.empty()) {
        std::cout << terminal::dim("Nothing to deploy: no enabled mods provide any assets.",
                                   ctx.use_color)
                  << "\n";
        return;
    }

    std::vector<std::vector<std::string>> rows;
    rows.reserve(plan.mappings.size());
    for (const auto& mapping : plan.mappings) {
        std::string shadowed;
        for (const auto& id : mapping.shadowed_mods) {
            if (!shadowed.empty()) {
                shadowed += ", ";
            }
            shadowed += id;
        }
        rows.push_back({mapping.target_relative_path, mapping.owner_mod_id,
                        std::to_string(mapping.priority), shadowed});
    }
    std::cout << table::render({{"Target"}, {"Owner"}, {"Pri", table::Align::Right}, {"Shadowed"}},
                               rows, ctx.use_color);
    std::cout << "\n" << plan.mappings.size() << " file(s) would be linked.\n";
}

void print_conflicts(const CliContext& ctx, const ConflictReport& report) {
    if (report.records.empty()) {
        return;
    }

    std::cout << "\n" << terminal::bold("Conflicts", ctx.use_color) << "\n";
    for (const auto& record : report.records) {
        const char* ansi = record.severity == ConflictSeverity::Critical
                               ? "31"
                               : (record.severity == ConflictSeverity::Warning ? "33" : "2");
        std::cout << "  " << terminal::paint(to_string(record.severity), ansi, ctx.use_color) << " "
                  << record.relative_path << "\n";
        std::cout << "      " << record.message << "\n";
    }
}

nlohmann::json plan_json(const DeployPlan& plan, const fs::path& target) {
    nlohmann::json mappings = nlohmann::json::array();
    for (const auto& mapping : plan.mappings) {
        mappings.push_back({
            {"target_relative_path", mapping.target_relative_path},
            {"owner_mod_id", mapping.owner_mod_id},
            {"priority", mapping.priority},
            {"shadowed_mods", mapping.shadowed_mods},
        });
    }

    nlohmann::json records = nlohmann::json::array();
    for (const auto& record : plan.conflict_report.records) {
        records.push_back(conflict_json(record));
    }

    return nlohmann::json{
        {"profile", plan.active_profile},
        {"target_dir", target.string()},
        {"file_count", plan.mappings.size()},
        {"mappings", mappings},
        {"conflicts", records},
        {"has_critical_conflict", plan.conflict_report.has_critical_conflict},
        {"has_warning_conflict", plan.conflict_report.has_warning_conflict},
        {"total_conflicts", plan.conflict_report.total_conflicts},
    };
}

} // namespace

nlohmann::json run_plan(CliContext& ctx, bool execute) {
    const fs::path staging = ensure_staging_dir(ctx);
    const fs::path game = require_game_dir(ctx);
    const fs::path target = target_mods_dir(game);

    // Refuse before doing anything at all when the plan contains a critical collision: the user
    // should see the warning with a chance to reorder priorities, not afterwards.
    const ScanOutcome outcome = ModLoader::scan_mods_directory(staging);
    const DeployPlan plan = DeploymentPlanner::build_plan(profile_of(ctx), outcome.mods);

    report_scan_failures(ctx, outcome);

    if (!execute) {
        if (!ctx.json_mode) {
            print_plan(ctx, plan, target);
            print_conflicts(ctx, plan.conflict_report);
        }
        return plan_json(plan, target);
    }

    if (!ctx.json_mode) {
        print_plan(ctx, plan, target);
        print_conflicts(ctx, plan.conflict_report);
        std::cout << "\n";
    }

    const DeployProgressCallback on_progress = [&ctx](const DeployProgress& progress) {
        emit_progress(ctx, "deploy", progress.done, progress.total, progress.current_path);
    };

    const DeployResult result = execute_deploy(plan, target, on_progress);

    if (!ctx.json_mode) {
        std::cout << "\n" << terminal::bold("Deployed", ctx.use_color) << " " << result.total_files
                  << " file(s) in " << result.duration_ms << " ms\n"
                  << "  hard links: " << result.hard_links_created << ", copies: "
                  << result.copied_files << ", failed: " << result.failed_files << "\n"
                  << "  disk space saved: " << format_bytes(result.bytes_saved) << "\n";
        for (const auto& warning : result.warnings) {
            std::cout << "  " << terminal::yellow("warning", ctx.use_color) << " " << warning
                      << "\n";
        }
        for (const auto& [path, message] : result.errors) {
            std::cout << "  " << terminal::red("error", ctx.use_color) << " " << path << ": "
                      << message << "\n";
        }
    }

    nlohmann::json payload = plan_json(plan, target);
    payload["result"] = result;
    payload["success"] = result.is_success();
    return payload;
}

nlohmann::json run_restore(CliContext& ctx) {
    const fs::path game = require_game_dir(ctx);
    const fs::path target = target_mods_dir(game);

    const DeployProgressCallback on_progress = [&ctx](const DeployProgress& progress) {
        emit_progress(ctx, "restore", progress.done, progress.total, progress.current_path);
    };

    const RestoreResult result = restore_deploy(target, on_progress);

    if (!ctx.json_mode) {
        std::cout << terminal::bold("Restored", ctx.use_color) << " " << target.string() << "\n"
                  << "  files removed: " << result.removed_files << "\n"
                  << "  empty directories removed: " << result.removed_dirs << "\n";
        for (const auto& warning : result.warnings) {
            std::cout << "  " << terminal::yellow("warning", ctx.use_color) << " " << warning
                      << "\n";
        }
    }

    return nlohmann::json{
        {"target_dir", target.string()},
        {"result", result},
    };
}

} // namespace smm::cli
