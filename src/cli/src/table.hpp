#pragma once

#include <string>
#include <vector>

namespace smm::cli::table {

/// How a column's cells are aligned.
enum class Align { Left, Right };

struct Column {
    std::string header;
    Align align{Align::Left};
};

/// Renders a Unicode box-drawing table.
///
/// Column widths are computed from display width (CJK counts as two columns), not byte length,
/// so a Chinese mod name cannot push every following column out of alignment.
/// \`use_color\` gates the header emphasis; the caller decides based on where stdout points.
std::string render(const std::vector<Column>& columns,
                   const std::vector<std::vector<std::string>>& rows, bool use_color);

/// Two-column "Field | Value" table used by \`info\` and \`doctor\`.
std::string render_key_values(const std::vector<std::pair<std::string, std::string>>& entries,
                              bool use_color);

} // namespace smm::cli::table
