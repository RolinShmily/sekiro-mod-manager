#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "types.hpp"

namespace smm {

/// A mod directory that could not be scanned, kept so the UI can surface it instead of hiding it.
struct ScanFailure {
    fs::path path;
    std::string message;
};

/// Result of scanning a staging directory. Partial failures never abort the scan.
struct ScanOutcome {
    std::vector<StagedMod> mods; ///< Sorted by priority ascending, then by id
    std::vector<ScanFailure> failures;

    const StagedMod* find(const std::string& mod_id) const;
};

/// Loading and scanning of mod directories.
class ModLoader {
public:
    /// Reads `.smm_mod.json`, falling back to `mod.json`.
    /// Throws SmmError(MetadataNotFound) or SmmError(InvalidMetadata).
    static ModInfo load_mod_info(const fs::path& mod_dir);

    /// Loads metadata and normalises the whole asset tree of a single mod.
    static StagedMod scan_mod(const fs::path& mod_dir);

    /// Scans every mod directory in staging. Unreadable mods are reported in ScanOutcome::failures.
    static ScanOutcome scan_mods_directory(const fs::path& staging_dir);

    /// True when the directory looks like a mod (carries either metadata file).
    static bool is_mod_directory(const fs::path& dir);
};

} // namespace smm
