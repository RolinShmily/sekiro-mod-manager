#include "json_output.hpp"

#include <cstdio>
#include <iostream>

#include "terminal.hpp"

namespace smm::cli {

void emit_json_line(const nlohmann::json& value) {
    // Streamed rather than buffered so a GUI reading the pipe sees progress immediately and not
    // only once the process exits.
    std::cout << value.dump() << '\n';
    std::cout.flush();
}

void emit_progress(const CliContext& ctx, std::string_view phase, std::size_t done,
                   std::size_t total, std::string_view current) {
    if (total == 0) {
        return;
    }

    if (ctx.json_mode) {
        emit_json_line(nlohmann::json{
            {"event", "progress"},
            {"phase", phase},
            {"done", done},
            {"total", total},
            {"percent", static_cast<int>((done * 100) / total)},
            {"current", current},
        });
        return;
    }

    if (ctx.quiet || !terminal::stdout_is_terminal()) {
        // A progress bar that rewrites one line is meaningless in a log file or a pipe, and the
        // carriage returns would corrupt the output.
        return;
    }

    const int percent = static_cast<int>((done * 100) / total);
    std::string line = "\r" + terminal::cyan(phase, ctx.use_color) + " " +
                       std::to_string(done) + "/" + std::to_string(total) + " (" +
                       std::to_string(percent) + "%)";
    if (!current.empty()) {
        line += "  " + terminal::dim(current, ctx.use_color);
    }
    // Erase whatever the previous, possibly longer, status line left behind.
    std::cout << line << "\x1b[K";
    std::cout.flush();
    if (done >= total) {
        std::cout << '\n';
    }
}

void emit_success(const CliContext& ctx, const nlohmann::json& data) {
    if (!ctx.json_mode) {
        return;
    }
    emit_json_line(nlohmann::json{
        {"ok", true},
        {"command", ctx.command},
        {"data", data},
    });
}

void emit_error(const CliContext& ctx, smm::ErrorCode code, const std::string& message) {
    if (!ctx.json_mode) {
        return;
    }
    emit_json_line(error_document(ctx, code, message));
}

nlohmann::json error_document(const CliContext& ctx, smm::ErrorCode code,
                              const std::string& message) {
    return nlohmann::json{
        {"ok", false},
        {"command", ctx.command},
        {"error", nlohmann::json{{"code", std::string(smm::to_string(code))},
                                 {"message", message}}},
    };
}

} // namespace smm::cli
