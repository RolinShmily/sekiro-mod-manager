#include "smm/error.hpp"

#include <array>
#include <utility>

namespace smm {
namespace {

/// Maps a code to its stable JSON name. This table is the CLI/GUI wire contract:
/// the GUI switches on these strings, so an entry may be added but never renamed.
constexpr std::array<std::pair<ErrorCode, std::string_view>, 17> kNames{{
    {ErrorCode::Io, "io"},
    {ErrorCode::Json, "json"},
    {ErrorCode::MetadataNotFound, "metadata_not_found"},
    {ErrorCode::InvalidMetadata, "invalid_metadata"},
    {ErrorCode::NormalizationError, "normalization_error"},
    {ErrorCode::NoAssetsFound, "no_assets_found"},
    {ErrorCode::DeployError, "deploy_error"},
    {ErrorCode::ModNotFound, "mod_not_found"},
    {ErrorCode::ModAlreadyExists, "mod_already_exists"},
    {ErrorCode::Archive, "archive"},
    {ErrorCode::ExtractionError, "extraction_error"},
    {ErrorCode::UnsupportedArchive, "unsupported_archive"},
    {ErrorCode::EnvironmentError, "environment_error"},
    {ErrorCode::ExportError, "export_error"},
    {ErrorCode::ModEngineSourceMissing, "mod_engine_source_missing"},
    {ErrorCode::InvalidArgument, "invalid_argument"},
    {ErrorCode::Cancelled, "cancelled"},
}};

} // namespace

std::string_view to_string(ErrorCode code) {
    for (const auto& [candidate, name] : kNames) {
        if (candidate == code) {
            return name;
        }
    }
    return "io";
}

ErrorCode error_code_from_string(std::string_view text) {
    for (const auto& [candidate, name] : kNames) {
        if (name == text) {
            return candidate;
        }
    }
    return ErrorCode::Io;
}

SmmError::SmmError(ErrorCode code, std::string message, fs::path path)
    : std::runtime_error(build_what(code, message, path)),
      code_(code),
      path_(std::move(path)) {}

std::string SmmError::build_what(ErrorCode code, const std::string& message, const fs::path& path) {
    std::string what;
    what.reserve(message.size() + 64);
    what.append(message);
    if (!path.empty()) {
        // The path is what the user has to act on, so it belongs in the message itself and
        // not only in the structured field - the CLI prints what() verbatim.
        what.append(" [path: ").append(path.string()).append("]");
    }
    if (what.empty()) {
        what.assign(to_string(code));
    }
    return what;
}

std::string SmmError::code_name() const {
    return std::string(to_string(code_));
}

void fail(ErrorCode code, std::string message, fs::path path) {
    throw SmmError(code, std::move(message), std::move(path));
}

} // namespace smm
