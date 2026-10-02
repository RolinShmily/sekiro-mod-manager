#include <exception>
#include <iostream>
#include <string>

#include "commands/commands.hpp"
#include "json_output.hpp"
#include "terminal.hpp"

#include "smm/error.hpp"

namespace {

constexpr int kExitSuccess = 0;
constexpr int kExitFailure = 1;
constexpr int kExitUsage = 2;

/// Prints \`text\` to stderr, wrapped in red when a terminal is attached.
void print_error(const std::string& text) {
    std::cerr << smm::cli::terminal::red("error", smm::cli::terminal::stderr_is_terminal()) << ": "
              << text << "\n";
}

} // namespace

int main(int argc, char** argv) {
    smm::cli::terminal::initialize();

    smm::cli::CliContext ctx;
    try {
        ctx = smm::cli::parse_arguments(argc, argv);
        if (!ctx.has("--no-color") && !smm::cli::terminal::stdout_is_terminal()) {
            ctx.use_color = false;
        }
    } catch (const smm::SmmError& error) {
        print_error(error.what());
        return kExitUsage;
    }

    if (ctx.has("--version") || ctx.has("-V")) {
        std::cout << "smm (Sekiro Mod Manager) 0.3.1\n";
        return kExitSuccess;
    }

    if (ctx.command.empty()) {
        std::cout << smm::cli::full_help();
        // No command is a request for help only when it was asked for; a bare invocation is a
        // usage mistake and says so through the exit code.
        return (ctx.has("--help") || ctx.has("-h")) ? kExitSuccess : kExitUsage;
    }

    if (ctx.has("--help") || ctx.has("-h")) {
        const smm::cli::CommandSpec* spec = smm::cli::find_spec(ctx.command);
        if (spec != nullptr) {
            std::cout << spec->summary << "\n\nUsage: " << smm::cli::command_usage(*spec) << "\n";
        } else {
            std::cout << smm::cli::full_help();
        }
        return kExitSuccess;
    }

    try {
        using namespace smm::cli;
        nlohmann::json data;

        if (ctx.command == "list") {
            data = run_list(ctx);
        } else if (ctx.command == "info") {
            data = run_info(ctx);
        } else if (ctx.command == "enable") {
            data = run_enable(ctx, true);
        } else if (ctx.command == "disable") {
            data = run_enable(ctx, false);
        } else if (ctx.command == "enable-asset") {
            data = run_toggle_asset(ctx, true);
        } else if (ctx.command == "disable-asset") {
            data = run_toggle_asset(ctx, false);
        } else if (ctx.command == "set-preview") {
            data = run_set_preview(ctx);
        } else if (ctx.command == "priority") {
            data = run_priority(ctx);
        } else if (ctx.command == "remove") {
            data = run_remove(ctx);
        } else if (ctx.command == "import") {
            data = run_import(ctx);
        } else if (ctx.command == "import-pack") {
            data = run_import_pack(ctx);
        } else if (ctx.command == "export") {
            data = run_export(ctx);
        } else if (ctx.command == "plan") {
            data = run_plan(ctx, false);
        } else if (ctx.command == "deploy") {
            data = run_plan(ctx, true);
        } else if (ctx.command == "restore") {
            data = run_restore(ctx);
        } else if (ctx.command == "conflicts") {
            data = run_conflicts(ctx);
        } else if (ctx.command == "doctor") {
            data = run_doctor(ctx);
        } else if (ctx.command == "setup-engine") {
            data = run_setup_engine(ctx);
        } else if (ctx.command == "presets") {
            data = run_presets(ctx);
        } else if (ctx.command == "env") {
            data = run_env(ctx);
        } else if (ctx.command == "config") {
            data = run_config(ctx);
        } else if (ctx.command == "self-update") {
            data = run_self_update(ctx);
        } else {
            fail(smm::ErrorCode::InvalidArgument,
                 "Unknown command '" + ctx.command + "'. Run 'smm --help'.");
        }

        emit_success(ctx, data);
        return kExitSuccess;
    } catch (const smm::SmmError& error) {
        // A command may have already streamed progress events; the error document is still the
        // last line, which is exactly where the GUI looks for it.
        emit_error(ctx, error.code(), error.what());
        if (!ctx.json_mode) {
            print_error(error.what());
        }
        return kExitFailure;
    } catch (const std::exception& error) {
        emit_error(ctx, smm::ErrorCode::Io, error.what());
        if (!ctx.json_mode) {
            print_error(std::string("unexpected failure: ") + error.what());
        }
        return kExitFailure;
    }
}
