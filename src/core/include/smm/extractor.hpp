#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "types.hpp"

namespace smm {

/// Container formats the importer can open.
enum class ArchiveFormat { Zip, SevenZip, Rar, Unknown };

std::string to_string(ArchiveFormat format);
bool is_supported_archive(const fs::path& path);

/// Detects by extension first, then by magic bytes. Unknown for directories.
ArchiveFormat detect_archive_format(const fs::path& path);

using ExtractProgressCallback = std::function<void(std::size_t entries_done, std::size_t entries_total)>;

/// Extracts a .zip using the vendored miniz reader. No external tool required.
void extract_zip(const fs::path& archive, const fs::path& dest_dir,
                 const ExtractProgressCallback& on_progress = {});

/// Extracts a .7z through an external 7-Zip / 7za / 7zr binary.
/// Throws SmmError(ExtractionError) when no 7-Zip is installed.
void extract_7z(const fs::path& archive, const fs::path& dest_dir);

/// Extracts a .rar through an external UnRAR / WinRAR binary.
void extract_rar(const fs::path& archive, const fs::path& dest_dir);

/// Dispatches on detect_archive_format().
void extract_archive(const fs::path& archive, const fs::path& dest_dir,
                     const ExtractProgressCallback& on_progress = {});

/// Absolute path of a usable 7-Zip binary, or empty when none was found.
/// Searched: PATH, Program Files, Program Files (x86), and the SMM tools directory.
fs::path find_external_7z();

/// Absolute path of a usable UnRAR/WinRAR binary, or empty.
fs::path find_external_unrar();

} // namespace smm
