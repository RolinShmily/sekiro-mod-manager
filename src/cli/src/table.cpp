#include "table.hpp"

#include <algorithm>

#include "terminal.hpp"

namespace smm::cli::table {
namespace {

constexpr std::string_view kTopLeft = "\u250C";
constexpr std::string_view kTopRight = "\u2510";
constexpr std::string_view kBottomLeft = "\u2514";
constexpr std::string_view kBottomRight = "\u2518";
constexpr std::string_view kHorizontal = "\u2500";
constexpr std::string_view kVertical = "\u2502";
constexpr std::string_view kCross = "\u253C";
constexpr std::string_view kTeeDown = "\u252C";
constexpr std::string_view kTeeUp = "\u2534";
constexpr std::string_view kTeeRight = "\u251C";
constexpr std::string_view kTeeLeft = "\u2524";

std::string repeat(std::string_view unit, std::size_t count) {
    std::string result;
    result.reserve(unit.size() * count);
    for (std::size_t i = 0; i < count; ++i) {
        result.append(unit);
    }
    return result;
}

std::string horizontal_rule(const std::vector<std::size_t>& widths, std::string_view left,
                            std::string_view middle, std::string_view right) {
    std::string rule(left);
    for (std::size_t i = 0; i < widths.size(); ++i) {
        if (i != 0) {
            rule.append(middle);
        }
        // One space of padding on each side of the content.
        rule.append(repeat(kHorizontal, widths[i] + 2));
    }
    rule.append(right);
    return rule;
}

} // namespace

std::string render(const std::vector<Column>& columns,
                   const std::vector<std::vector<std::string>>& rows, bool use_color) {
    if (columns.empty()) {
        return {};
    }

    std::vector<std::size_t> widths(columns.size(), 0);
    for (std::size_t i = 0; i < columns.size(); ++i) {
        widths[i] = terminal::display_width(columns[i].header);
    }
    for (const auto& row : rows) {
        for (std::size_t i = 0; i < columns.size() && i < row.size(); ++i) {
            widths[i] = std::max(widths[i], terminal::display_width(row[i]));
        }
    }

    std::string output;
    output.append(horizontal_rule(widths, kTopLeft, kTeeDown, kTopRight)).append("\n");

    const auto emit_row = [&](const std::vector<std::string>& cells, bool header) {
        output.append(kVertical);
        for (std::size_t i = 0; i < columns.size(); ++i) {
            const std::string cell = i < cells.size() ? cells[i] : std::string{};
            output.push_back(' ');
            output.append(header ? terminal::pad_right(cell, widths[i])
                                 : (columns[i].align == Align::Right
                                        ? terminal::pad_left(cell, widths[i])
                                        : terminal::pad_right(cell, widths[i])));
            output.push_back(' ');
            output.append(kVertical);
        }
        output.push_back('\n');
    };

    std::vector<std::string> header_cells;
    header_cells.reserve(columns.size());
    for (const auto& column : columns) {
        header_cells.push_back(terminal::bold(column.header, use_color));
    }
    emit_row(header_cells, false);
    output.append(horizontal_rule(widths, kTeeRight, kCross, kTeeLeft)).append("\n");

    for (const auto& row : rows) {
        emit_row(row, false);
    }
    output.append(horizontal_rule(widths, kBottomLeft, kTeeUp, kBottomRight)).append("\n");
    return output;
}

std::string render_key_values(const std::vector<std::pair<std::string, std::string>>& entries,
                              bool use_color) {
    std::size_t width = 0;
    for (const auto& [key, value] : entries) {
        width = std::max(width, terminal::display_width(key));
    }

    std::string output;
    for (const auto& [key, value] : entries) {
        output.append(terminal::pad_right(key, width));
        output.append("  ");
        if (value.empty()) {
            output.append(terminal::dim("(none)", use_color));
        } else {
            output.append(value);
        }
        output.push_back('\n');
    }
    return output;
}

} // namespace smm::cli::table
