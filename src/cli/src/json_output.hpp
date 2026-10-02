#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "cli_parser.hpp"
#include "smm/error.hpp"

namespace smm::cli {

/// Writes one JSON object as a single line.
///
/// One object per line is the whole protocol: the GUI reads stdout line by line, so progress
/// events can be rendered while a long deploy is still running instead of only at the end.
void emit_json_line(const nlohmann::json& value);

/// Reports progress. In human mode it rewrites a single status line; in JSON mode it emits an
/// event object.
void emit_progress(const CliContext& ctx, std::string_view phase, std::size_t done,
                   std::size_t total, std::string_view current);

/// Closes a run with the success envelope, in JSON mode only.
void emit_success(const CliContext& ctx, const nlohmann::json& data);

/// Closes a run with the error envelope, in JSON mode only.
void emit_error(const CliContext& ctx, smm::ErrorCode code, const std::string& message);

/// Convenience wrapper used by main so every command gets an identical outer shape:
///   {"ok": true,  "command": "list", "data": {...}}
///   {"ok": false, "command": "list", "error": {"code": "...", "message": "..."}}
nlohmann::json error_document(const CliContext& ctx, smm::ErrorCode code,
                              const std::string& message);

} // namespace smm::cli
