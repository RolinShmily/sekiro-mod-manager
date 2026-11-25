#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "types.hpp"

namespace smm {

/// Caller supplied overrides for an import. Everything left empty is inferred.
struct ImportOptions {
    std::optional<std::string> id;       ///< Otherwise slugified from the source name
    std::optional<std::string> name;     ///< Otherwise humanised from the source name
    std::optional<uint32_t> priority;    ///< Otherwise default_priority_for_category()
    std::optional<std::string> source_url; ///< Upstream download location, recorded in metadata
    bool overwrite{false};               ///< Replace an existing mod with the same id
    bool dry_run{false};                 ///< Analyse and report, but write nothing
};

/// What an import actually produced.
struct ImportResult {
    ModInfo info;
    fs::path mod_dir;
    std::size_t asset_count{0};
    std::size_t ignored_count{0};
    uint64_t total_bytes{0};
    bool replaced_existing{false};
    bool dry_run{false};
    /// True when the source was an archive and SMM had to extract it first.
    bool extracted_archive{false};
};

void to_json(json& j, const ImportResult& r);

/// Imports a directory, .zip, .7z or .rar into staging.
///
/// The archive path is fully unpacked into a scratch directory first, the canonical root is
/// located with Normalizer, and only then is the payload moved into `staging_dir/<id>`
/// with metadata written to both `.smm_mod.json` and `mod.json`.
///
/// Throws SmmError(ModAlreadyExists) unless options.overwrite is set.
ImportResult import_mod(const fs::path& source, const fs::path& staging_dir,
                        const ImportOptions& options = {});

/// Bundles several loose files (e.g. a multi-part download) into one staged mod.
ImportResult import_multiple_files_as_mod(const std::vector<fs::path>& files,
                                          const fs::path& staging_dir,
                                          const ImportOptions& options = {});

/// Documentation worth carrying alongside a mod: README / LICENSE / LICENCE / CHANGELOG / *.md,
/// deduplicated case-insensitively so a pack shipping both README.md and readme.md cannot
/// collide on Windows. Shallower files win.
std::vector<std::pair<std::string, fs::path>> collect_documentation_files(const fs::path& root);

/// "Dream of the Damned v1.2 (Nexus)" -> "dream-of-the-damned"
std::string slugify(std::string_view input);

/// "dream_of_the_damned_v1.2" -> "Dream Of The Damned"
std::string humanize_name(std::string_view input);

/// Pulls "1.2.3" / "v2.0" out of a filename, when present.
std::optional<std::string> extract_version(std::string_view input);

/// Guesses a category from the asset mix (a payload of only parts/ files is a weapon skin).
std::string infer_category(const std::vector<AssetEntry>& assets);

/// Default priority for a category; loaders and overhauls must win over cosmetics.
uint32_t default_priority_for_category(std::string_view category);

} // namespace smm
