#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace smm {

namespace fs = std::filesystem;

/// Stable, machine-readable error taxonomy.
///
/// The string form of every code is part of the CLI's JSON contract consumed by the GUI
/// (`{"error": {"code": "mod_not_found"}}`), so codes must never be renamed silently.
enum class ErrorCode {
    Io,                     ///< Filesystem / OS level failure
    Json,                   ///< JSON parse or serialisation failure
    MetadataNotFound,       ///< No .smm_mod.json / mod.json inside the mod directory
    InvalidMetadata,        ///< Metadata present but malformed
    NormalizationError,     ///< Directory is not a recognisable Sekiro mod payload
    NoAssetsFound,          ///< Normalised, but zero deployable game assets
    DeployError,            ///< Plan generation / target validation failure
    ModNotFound,            ///< No mod with the requested id in staging
    ModAlreadyExists,       ///< Import target id already taken
    Archive,                ///< Archive container level failure (zip/7z/rar)
    ExtractionError,        ///< Extraction failed mid-way
    UnsupportedArchive,     ///< Container format not recognised
    EnvironmentError,       ///< Game / ModEngine environment problem
    ExportError,            ///< Modpack export failure
    ModEngineSourceMissing, ///< No ModEngine payload available to install
    InvalidArgument,        ///< Bad CLI argument combination
    Cancelled               ///< Operation aborted by the user
};

std::string_view to_string(ErrorCode code);
ErrorCode error_code_from_string(std::string_view text);

/// The single exception type thrown by every `smm_core` entry point.
class SmmError : public std::runtime_error {
public:
    SmmError(ErrorCode code, std::string message, fs::path path = {});

    ErrorCode code() const noexcept { return code_; }

    /// Optional offending path, for diagnostics and for the GUI's error banner.
    const fs::path& path() const noexcept { return path_; }

    /// JSON name of error code(), e.g. "mod_not_found".
    std::string code_name() const;

private:
    /// Composes what(): the message, plus the offending path when there is one.
    static std::string build_what(ErrorCode code, const std::string& message, const fs::path& path);

    ErrorCode code_;
    fs::path path_;
};

/// Throws an SmmError. Provided so call sites read as `fail(ErrorCode::X, "...")`.
[[noreturn]] void fail(ErrorCode code, std::string message, fs::path path = {});

} // namespace smm
