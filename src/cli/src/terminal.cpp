#include "terminal.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace smm::cli::terminal {
namespace {

/// Decodes one UTF-8 code point, advancing \`index\`. Returns U+FFFD on malformed input so a
/// broken byte in a mod name can never desynchronise the whole table.
std::uint32_t decode_utf8(std::string_view text, std::size_t& index) {
    const auto byte = static_cast<unsigned char>(text[index]);
    if (byte < 0x80) {
        ++index;
        return byte;
    }

    std::size_t extra = 0;
    std::uint32_t code = 0;
    if ((byte & 0xE0) == 0xC0) {
        extra = 1;
        code = byte & 0x1F;
    } else if ((byte & 0xF0) == 0xE0) {
        extra = 2;
        code = byte & 0x0F;
    } else if ((byte & 0xF8) == 0xF0) {
        extra = 3;
        code = byte & 0x07;
    } else {
        ++index;
        return 0xFFFD;
    }

    if (index + extra >= text.size()) {
        ++index;
        return 0xFFFD;
    }
    for (std::size_t i = 1; i <= extra; ++i) {
        const auto next = static_cast<unsigned char>(text[index + i]);
        if ((next & 0xC0) != 0x80) {
            ++index;
            return 0xFFFD;
        }
        code = (code << 6) | (next & 0x3F);
    }
    index += extra + 1;
    return code;
}

/// Wide code points occupy two terminal columns: CJK ideographs, Hangul, full-width forms.
bool is_wide(std::uint32_t code) {
    return (code >= 0x1100 && code <= 0x115F) ||   // Hangul Jamo
           (code >= 0x2E80 && code <= 0xA4CF) ||   // CJK radicals .. Yi
           (code >= 0xAC00 && code <= 0xD7A3) ||   // Hangul syllables
           (code >= 0xF900 && code <= 0xFAFF) ||   // CJK compatibility ideographs
           (code >= 0xFE30 && code <= 0xFE6F) ||   // CJK compatibility forms
           (code >= 0xFF00 && code <= 0xFF60) ||   // Full-width forms
           (code >= 0xFFE0 && code <= 0xFFE6) ||
           (code >= 0x20000 && code <= 0x3FFFD);   // CJK extension planes
}

} // namespace

void initialize() {
#ifdef _WIN32
    // Without this the console would render UTF-8 asset paths as mojibake.
    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);

    const HANDLE output = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (output != INVALID_HANDLE_VALUE && output != nullptr) {
        DWORD mode = 0;
        if (::GetConsoleMode(output, &mode) != 0) {
            ::SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif
}

bool stdout_is_terminal() {
#ifdef _WIN32
    const HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE || handle == nullptr) {
        return false;
    }
    DWORD mode = 0;
    return ::GetConsoleMode(handle, &mode) != 0;
#else
    return ::isatty(STDOUT_FILENO) == 1;
#endif
}

bool stderr_is_terminal() {
#ifdef _WIN32
    const HANDLE handle = ::GetStdHandle(STD_ERROR_HANDLE);
    if (handle == INVALID_HANDLE_VALUE || handle == nullptr) {
        return false;
    }
    DWORD mode = 0;
    return ::GetConsoleMode(handle, &mode) != 0;
#else
    return ::isatty(STDERR_FILENO) == 1;
#endif
}

std::string paint(std::string_view text, std::string_view ansi_code, bool enabled) {
    if (!enabled || ansi_code.empty()) {
        return std::string(text);
    }
    std::string result;
    result.reserve(text.size() + ansi_code.size() + 5);
    result.append("\x1b[").append(ansi_code).append("m");
    result.append(text);
    result.append("\x1b[0m");
    return result;
}

std::string bold(std::string_view text, bool enabled) { return paint(text, "1", enabled); }
std::string dim(std::string_view text, bool enabled) { return paint(text, "2", enabled); }
std::string green(std::string_view text, bool enabled) { return paint(text, "32", enabled); }
std::string yellow(std::string_view text, bool enabled) { return paint(text, "33", enabled); }
std::string red(std::string_view text, bool enabled) { return paint(text, "31", enabled); }
std::string cyan(std::string_view text, bool enabled) { return paint(text, "36", enabled); }

std::size_t display_width(std::string_view text) {
    std::size_t width = 0;
    std::size_t index = 0;
    while (index < text.size()) {
        const std::uint32_t code = decode_utf8(text, index);
        if (code == 0x1B && index < text.size() && text[index] == '[') {
            // Skip an ANSI escape sequence; it occupies no columns.
            while (index < text.size() && text[index] != 'm') {
                ++index;
            }
            if (index < text.size()) {
                ++index;
            }
            continue;
        }
        width += is_wide(code) ? 2 : 1;
    }
    return width;
}

std::string pad_right(std::string_view text, std::size_t width) {
    std::string result(text);
    const std::size_t current = display_width(text);
    if (current < width) {
        result.append(width - current, ' ');
    }
    return result;
}

std::string pad_left(std::string_view text, std::size_t width) {
    const std::size_t current = display_width(text);
    if (current >= width) {
        return std::string(text);
    }
    return std::string(width - current, ' ') + std::string(text);
}

} // namespace smm::cli::terminal
