#include "commands.hpp"

#include <iostream>

#include "json_output.hpp"
#include "resolve.hpp"
#include "table.hpp"
#include "terminal.hpp"

#include "smm/doctor.hpp"
#include "smm/error.hpp"

namespace smm::cli {
namespace {

std::string status_style(DiagnosticStatus status, const CliContext& ctx) {
    switch (status) {
        case DiagnosticStatus::Ok: return terminal::green("ok", ctx.use_color);
        case DiagnosticStatus::Warning: return terminal::yellow("warning", ctx.use_color);
        case DiagnosticStatus::Error: return terminal::red("error", ctx.use_color);
        case DiagnosticStatus::Info: return terminal::dim("info", ctx.use_color);
    }
    return {};
}

void print_report(const CliContext& ctx, const HealthReport& report) {
    std::cout << terminal::bold("Environment report", ctx.use_color) << "\n"
              << "  game:    " << (report.game_dir.empty() ? "(not found)"
                                                           : report.game_dir.string())
              << "\n"
              << "  staging: " << report.staging_dir.string() << "\n\n";

    std::vector<std::vector<std::string>> rows;
    rows.reserve(report.items.size());
    for (const auto& item : report.items) {
        rows.push_back({status_style(item.status, ctx), item.category, item.title, item.detail});
    }
    std::cout << table::render({{"Status"}, {"Area"}, {"Check"}, {"Detail"}}, rows, ctx.use_color);

    // Remediation is the actionable half of the report, so it is listed rather than folded into
    // a table cell where it would be unreadable.
    bool printed_header = false;
    for (const auto& item : report.items) {
        if (!item.remediation) {
            continue;
        }
        if (!printed_header) {
            std::cout << "\n" << terminal::bold("What to do", ctx.use_color) << "\n";
            printed_header = true;
        }
        std::cout << "  " << item.title << ":\n      " << *item.remediation << "\n";
    }

    std::cout << "\n" << to_string(report.overall) << ": " << report.ok_count << " ok, "
              << report.warning_count << " warning(s), " << report.error_count << " error(s).\n";
}

} // namespace

nlohmann::json run_doctor(CliContext& ctx) {
    const fs::path game = resolve_game_dir(ctx);
    const fs::path staging = resolve_staging_dir(ctx);

    const HealthReport report = diagnose_environment(game, staging);

    if (!ctx.json_mode) {
        print_report(ctx, report);
    }

    return nlohmann::json{
        {"health", report},
        {"overall", to_string(report.overall)},
        {"healthy", report.is_healthy()},
    };
}

nlohmann::json run_setup_engine(CliContext& ctx) {
    const fs::path game = resolve_game_dir(ctx);
    if (game.empty()) {
        fail(ErrorCode::EnvironmentError,
             "The Sekiro installation could not be located. Pass --game-dir <path> or save it "
             "with 'smm config --game-dir <path>'.");
    }

    // The staging directory doubles as the place a user is expected to drop their own ModEngine
    // payload, so it is searched for one.
    const fs::path staging = resolve_staging_dir(ctx);
    const EngineProvisionResult result = provision_mod_engine(game, staging);

    if (!ctx.json_mode) {
        std::cout << terminal::green("ModEngine configured", ctx.use_color) << "\n"
                  << "  hook: " << result.dinput8_path.string()
                  << (result.installed_dinput8 ? "  (installed)" : "  (already present)") << "\n"
                  << "  config: " << result.ini_path.string() << "\n";
        if (!result.source_used.empty() && result.installed_dinput8) {
            std::cout << "  source: " << result.source_used.string() << "\n";
        }
        for (const auto& warning : result.warnings) {
            std::cout << "  " << terminal::yellow("warning", ctx.use_color) << " " << warning
                      << "\n";
        }
    }

    return nlohmann::json{
        {"engine", result},
        {"game_dir", game.string()},
    };
}

} // namespace smm::cli
