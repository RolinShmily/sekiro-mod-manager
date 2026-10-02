#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace smm::cli {

namespace fs = std::filesystem;

/// Everything a command needs to know about how it was invoked.
///
/// The GUI drives this binary exclusively through \`--json\`, so every command must work in both
/// modes: human output for a terminal, one JSON document for a program.
struct CliContext {
    std::string command;
    std::vector<std::string> positionals;
    std::map<std::string, std::string> options;
    std::set<std::string> flags;

    bool json_mode{false};
    bool quiet{false};
    bool use_color{true};
    bool assume_yes{false};

    /// Resolved after global option parsing; commands never re-derive these.
    fs::path staging_dir;
    fs::path game_dir;

    bool has(std::string_view flag) const { return flags.count(std::string(flag)) != 0; }

    std::string option(std::string_view name, std::string_view fallback = {}) const {
        const auto it = options.find(std::string(name));
        return it == options.end() ? std::string(fallback) : it->second;
    }

    bool has_option(std::string_view name) const {
        return options.find(std::string(name)) != options.end();
    }

    /// Positional at \`index\`, or an InvalidArgument error naming the command.
    const std::string& require_positional(std::size_t index, std::string_view what) const;

    std::uint32_t parse_u32(std::string_view name, std::uint32_t fallback) const;
};

/// Declarative description of one subcommand. Drives parsing, --help and error messages from a
/// single source, so the three can never drift apart.
struct CommandSpec {
    std::string name;
    std::string summary;
    /// Positional placeholders in order, e.g. {"<mod-id>"}. Trailing "..." means variadic.
    std::vector<std::string> positionals;
    /// Options that consume a following value, e.g. {"--priority"}.
    std::vector<std::string> options;
    /// Boolean switches, e.g. {"--overwrite"}.
    std::vector<std::string> flags;
};

const std::vector<CommandSpec>& command_specs();
const CommandSpec* find_spec(std::string_view name);

/// Options every command accepts.
const std::vector<std::string>& global_options();
const std::vector<std::string>& global_flags();

/// Parses argv. Throws SmmError(InvalidArgument) carrying an actionable message on misuse.
CliContext parse_arguments(int argc, char** argv);

std::string global_usage();
std::string command_usage(const CommandSpec& spec);
std::string full_help();

} // namespace smm::cli
