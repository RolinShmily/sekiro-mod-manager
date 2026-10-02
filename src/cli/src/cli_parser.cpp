#include "cli_parser.hpp"

#include <algorithm>
#include <sstream>

#include "smm/error.hpp"

namespace smm::cli {
namespace {

/// Every subcommand, declared once. Parsing, --help and misuse errors all read this table.
const std::vector<CommandSpec> kCommands = {
    {"list", "List every mod in the staging area with status, priority and size.", {}, {}, {}},
    {"info", "Show full metadata plus the normalised asset tree of one mod.", {"<mod-id>"}, {}, {}},
    {"enable", "Enable a mod so it participates in the next deployment.", {"<mod-id>"}, {}, {}},
    {"disable", "Disable a mod without deleting it.", {"<mod-id>"}, {}, {}},
    {"enable-asset", "Enable a specific file asset within a mod.", {"<mod-id>", "<relative-path>"}, {}, {}},
    {"disable-asset", "Disable a specific file asset within a mod so it is not deployed.", {"<mod-id>", "<relative-path>"}, {}, {}},
    {"set-preview", "Set or update the preview background image of a mod.", {"<mod-id>", "<image-path>"}, {}, {}},
    {"priority", "Set a mod's priority. Lower numbers win conflicts.",
     {"<mod-id>", "<priority>"}, {}, {}},
    {"remove", "Permanently delete a mod from the staging area.", {"<mod-id>"}, {}, {"--yes"}},
    {"import", "Import .zip/.7z/.rar packages or unpacked folders into staging.",
     {"<path>"},
     {"--id", "--name", "--priority", "--source-url"},
     {"--overwrite", "--dry-run"}},
    {"plan", "Build and print the deployment plan and its conflicts, without deploying.",
     {},
     {"--profile"},
     {}},
    {"deploy", "Deploy enabled mods into the Sekiro mods directory using hard links.",
     {},
     {"--profile"},
     {}},
    {"restore", "Remove everything SMM deployed and leave the game directory clean.", {}, {}, {}},
    {"conflicts", "Report asset collisions between the enabled mods.", {}, {}, {}},
    {"doctor", "Diagnose the game install and the ModEngine environment.", {}, {}, {}},
    {"setup-engine", "Install the ModEngine hook and write modengine.ini.", {}, {}, {}},
    {"presets", "Manage activation presets: list, save, apply, delete.",
     {"<action>", "..."},
     {"--description"},
     {}},
    {"export", "Export mods as a .zip (single) or .smmpack (collection).",
     {"<mod-id>", "..."},
     {"--output", "--name", "--description"},
     {"--no-source"}},
    {"import-pack", "Import a .smmpack modpack into staging.", {"<pack-file>"}, {}, {"--overwrite"}},
    {"env", "Print the resolved configuration and the detected environment.", {}, {}, {}},
    {"config", "Persist the staging and game directories for future runs.", {}, {"--game-dir"}, {}},
    {"self-update", "Apply a downloaded update package to replace binaries and restart the application.",
     {},
     {"--zip", "--target-dir", "--wait-pid"},
     {"--restart"}},
};

/// Options that consume the following argument, across every command. Parsing needs this before
/// the command is known, because options may legally appear before it.
std::set<std::string> value_taking_options() {
    std::set<std::string> result;
    for (const auto& spec : kCommands) {
        result.insert(spec.options.begin(), spec.options.end());
    }
    for (const auto& option : global_options()) {
        result.insert(option);
    }
    return result;
}

std::string join(const std::vector<std::string>& parts) {
    std::string result;
    for (const auto& part : parts) {
        if (!result.empty()) {
            result += " ";
        }
        result += part;
    }
    return result;
}

} // namespace

const std::vector<CommandSpec>& command_specs() { return kCommands; }

const CommandSpec* find_spec(std::string_view name) {
    const auto it = std::find_if(kCommands.begin(), kCommands.end(),
                                 [name](const CommandSpec& spec) { return spec.name == name; });
    return it == kCommands.end() ? nullptr : &*it;
}

const std::vector<std::string>& global_options() {
    static const std::vector<std::string> options = {"--staging", "--game-dir"};
    return options;
}

const std::vector<std::string>& global_flags() {
    static const std::vector<std::string> flags = {"--json", "--quiet", "--no-color", "--yes",
                                                   "--help",  "-h",      "--version", "-V"};
    return flags;
}

const std::string& CliContext::require_positional(std::size_t index, std::string_view what) const {
    if (index >= positionals.size() || positionals[index].empty()) {
        fail(ErrorCode::InvalidArgument,
             "The '" + command + "' command requires " + std::string(what) + ".");
    }
    return positionals[index];
}

std::uint32_t CliContext::parse_u32(std::string_view name, std::uint32_t fallback) const {
    if (!has_option(name)) {
        return fallback;
    }
    const std::string value = option(name);
    try {
        const unsigned long parsed = std::stoul(value);
        if (parsed > 0xFFFFFFFFUL) {
            fail(ErrorCode::InvalidArgument,
                 "The value for " + std::string(name) + " is out of range: " + value);
        }
        return static_cast<std::uint32_t>(parsed);
    } catch (const std::exception&) {
        fail(ErrorCode::InvalidArgument,
             "The value for " + std::string(name) + " must be a non-negative integer, got: " +
                 value);
    }
}

CliContext parse_arguments(int argc, char** argv) {
    CliContext ctx;
    const std::set<std::string> value_options = value_taking_options();

    for (int index = 1; index < argc; ++index) {
        std::string token = argv[index];

        if (token.size() > 2 && token.rfind("--", 0) == 0) {
            std::string name = token;
            std::string inline_value;
            if (const std::size_t equals = token.find('='); equals != std::string::npos) {
                name = token.substr(0, equals);
                inline_value = token.substr(equals + 1);
            }

            if (value_options.count(name) != 0) {
                if (!inline_value.empty()) {
                    ctx.options[name] = inline_value;
                } else if (index + 1 < argc) {
                    ctx.options[name] = argv[++index];
                } else {
                    fail(ErrorCode::InvalidArgument, "The option " + name + " needs a value.");
                }
                continue;
            }

            ctx.flags.insert(name);
            continue;
        }

        if (token.size() > 1 && token.front() == '-') {
            ctx.flags.insert(token);
            continue;
        }

        if (ctx.command.empty()) {
            ctx.command = token;
        } else {
            ctx.positionals.push_back(token);
        }
    }

    ctx.json_mode = ctx.has("--json");
    ctx.quiet = ctx.has("--quiet");
    ctx.assume_yes = ctx.has("--yes");
    // Colour is decided by the destination, not by preference: piping into a file must not
    // produce escape sequences, and --no-color forces plain output even on a terminal.
    ctx.use_color = !ctx.has("--no-color");

    if (ctx.command.empty()) {
        return ctx;
    }

    const CommandSpec* spec = find_spec(ctx.command);
    if (spec == nullptr) {
        fail(ErrorCode::InvalidArgument,
             "Unknown command '" + ctx.command + "'. Run 'smm --help' to see the available "
             "commands.");
    }

    for (const auto& [name, value] : ctx.options) {
        const bool global = std::find(global_options().begin(), global_options().end(), name) !=
                            global_options().end();
        const bool local = std::find(spec->options.begin(), spec->options.end(), name) !=
                           spec->options.end();
        if (!global && !local) {
            fail(ErrorCode::InvalidArgument,
                 "The '" + ctx.command + "' command does not accept " + name + ".");
        }
        (void)value;
    }

    for (const auto& flag : ctx.flags) {
        const bool global = std::find(global_flags().begin(), global_flags().end(), flag) !=
                            global_flags().end();
        const bool local = std::find(spec->flags.begin(), spec->flags.end(), flag) !=
                           spec->flags.end();
        if (!global && !local) {
            fail(ErrorCode::InvalidArgument,
                 "The '" + ctx.command + "' command does not accept " + flag + ".");
        }
    }

    // "..." means variadic; anything else fixes the exact count.
    const bool variadic = !spec->positionals.empty() && spec->positionals.back() == "...";
    const std::size_t minimum = variadic ? spec->positionals.size() - 1 : spec->positionals.size();
    if (ctx.positionals.size() < minimum) {
        fail(ErrorCode::InvalidArgument,
             "The '" + ctx.command + "' command expects " +
                 std::to_string(minimum) + " argument(s) " + join(spec->positionals) +
                 ", but got " + std::to_string(ctx.positionals.size()) + ". Usage: " +
                 command_usage(*spec));
    }
    if (!variadic && ctx.positionals.size() > spec->positionals.size()) {
        fail(ErrorCode::InvalidArgument,
             "The '" + ctx.command + "' command takes " + std::to_string(spec->positionals.size()) +
                 " argument(s) " + join(spec->positionals) + ", but got " +
                 std::to_string(ctx.positionals.size()) + ". Usage: " + command_usage(*spec));
    }

    return ctx;
}

std::string command_usage(const CommandSpec& spec) {
    std::string usage = "smm " + spec.name;
    if (!spec.positionals.empty()) {
        usage += " " + join(spec.positionals);
    }
    for (const auto& option : spec.options) {
        usage += " [" + option + " <value>]";
    }
    for (const auto& flag : spec.flags) {
        usage += " [" + flag + "]";
    }
    return usage;
}

std::string full_help() {
    std::ostringstream out;
    out << "Sekiro Mod Manager (SMM) - mod staging, conflict analysis and hard-link deployment.\n"
        << "\n"
        << "Usage: smm [--json] [--staging <path>] [--game-dir <path>] <command> [args]\n"
        << "\n"
        << "Commands:\n";
    for (const auto& spec : kCommands) {
        out << "  " << spec.name;
        // Pad by name length so the summaries line up in any viewer.
        out << std::string(spec.name.size() < 12 ? 12 - spec.name.size() : 1, ' ');
        out << spec.summary << "\n";
    }
    out << "\n"
        << "Global options:\n"
        << "  --json                 Emit one JSON document per line for programmatic callers.\n"
        << "  --staging <path>       Override the staging directory.\n"
        << "  --game-dir <path>      Override the Sekiro installation directory.\n"
        << "  --quiet                Suppress progress output.\n"
        << "  --no-color             Disable ANSI colour.\n"
        << "  --yes                  Skip interactive confirmation prompts.\n"
        << "\n"
        << "Every command also accepts --help for its own usage text.\n";
    return out.str();
}

} // namespace smm::cli
