#pragma once

#include <string>
#include <string_view>

namespace smm::cli::terminal {

/// Enables UTF-8 output and ANSI escape processing on the Windows console.
/// Called once at startup; a no-op elsewhere.
void initialize();

/// True when stdout is attached to a terminal rather than a pipe or a file.
/// Colour and progress rendering are suppressed otherwise, so redirected output stays clean.
bool stdout_is_terminal();
bool stderr_is_terminal();

/// Wraps \`text\` in an ANSI colour when colour is enabled, otherwise returns it unchanged.
std::string paint(std::string_view text, std::string_view ansi_code, bool enabled);

std::string bold(std::string_view text, bool enabled);
std::string dim(std::string_view text, bool enabled);
std::string green(std::string_view text, bool enabled);
std::string yellow(std::string_view text, bool enabled);
std::string red(std::string_view text, bool enabled);
std::string cyan(std::string_view text, bool enabled);

/// Visible width of a UTF-8 string, counting CJK characters as two columns.
/// Table alignment depends on this: a Chinese mod name is twice as wide as its byte count
/// suggests, and std::string::size() would misalign every column after it.
std::size_t display_width(std::string_view text);

/// Pads \`text\` with spaces to \`width\` display columns (never truncates).
std::string pad_right(std::string_view text, std::size_t width);

std::string pad_left(std::string_view text, std::size_t width);

} // namespace smm::cli::terminal
