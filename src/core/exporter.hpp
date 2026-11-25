#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "importer.hpp"
#include "types.hpp"

namespace smm {

/// One mod's descriptor inside a modpack manifest.
struct ModPackItem {
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::string category;
    uint32_t priority{100};
    bool enabled{true};
    std::optional<std::string> source_url;
    std::optional<std::string> homepage;
    std::optional<std::string> description;
    /// Informational only: filled in by the exporter so a pack can be inspected without opening it.
    std::size_t file_count{0};
    uint64_t total_bytes{0};
};

void to_json(json& j, const ModPackItem& i);
void from_json(const json& j, ModPackItem& i);

/// Manifest stored as \`smm_pack.json\` at the root of a .smmpack container (a plain ZIP).
struct ModPackManifest {
    uint32_t format_version{1};
    std::string name;
    std::string version{"1.0.0"};
    std::optional<std::string> author;
    std::optional<std::string> description;
    uint64_t created_at{0};
    std::vector<ModPackItem> mods;
};

void to_json(json& j, const ModPackManifest& m);
void from_json(const json& j, ModPackManifest& m);

/// What an export should carry.
struct ExportOptions {
    /// Include the .smm_source payload (the untouched original download) when present.
    /// By default false: normal mod export and modpack export do not include .smm_source.
    bool include_source{false};
};

using ExportProgressCallback =
    std::function<void(std::size_t done, std::size_t total, const std::string& current)>;

/// Writes one staged mod (metadata + normalised assets + docs) into a standalone .zip.
/// A \`.zip\` or \`.smmpack\` extension is appended when \`output_file\` has neither.
fs::path export_single_mod(const fs::path& staging_dir, const std::string& mod_id,
                           const fs::path& output_file,
                           const ExportOptions& options = {},
                           const ExportProgressCallback& on_progress = {});

/// Writes several staged mods plus a manifest into one .smmpack container.
fs::path export_modpack(const fs::path& staging_dir, const std::vector<std::string>& mod_ids,
                        const fs::path& output_file, const std::string& pack_name,
                        const std::string& description = {},
                        const ExportOptions& options = {},
                        const ExportProgressCallback& on_progress = {});

/// Outcome of importing a pack: the manifest that was read and what each mod became.
struct ModPackImportResult {
    ModPackManifest manifest;
    std::vector<ImportResult> mods;
};

/// Reads a pack produced by export_modpack() and stages every mod inside it.
///
/// The manifest is authoritative for priority, enabled state and source_url, because those are
/// the user's curation decisions rather than properties of the files on disk.
ModPackImportResult import_modpack(const fs::path& pack_file, const fs::path& staging_dir,
                                   bool overwrite = false);

} // namespace smm
