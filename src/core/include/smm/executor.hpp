#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "types.hpp"

namespace smm {

/// Manifest written into the game's mods directory so a later deploy/restore can
/// remove exactly what SMM put there and nothing else.
struct DeployManifest {
    uint32_t version{1};
    std::string profile{"default"};
    uint64_t deployed_at{0};
    std::vector<std::string> files; ///< Relative paths as deployed
    uint64_t total_bytes{0};
};

void to_json(json& j, const DeployManifest& m);
void from_json(const json& j, DeployManifest& m);

/// Progress tick emitted while files are being linked/copied.
struct DeployProgress {
    std::size_t done{0};
    std::size_t total{0};
    std::string current_path;
};

using DeployProgressCallback = std::function<void(const DeployProgress&)>;

/// Structured outcome of execute_deploy().
struct DeployResult {
    fs::path target_dir;
    std::size_t total_files{0};
    std::size_t hard_links_created{0};
    std::size_t copied_files{0};
    std::size_t skipped_files{0};
    std::size_t failed_files{0};
    uint64_t bytes_saved{0}; ///< Bytes not physically duplicated thanks to hard links
    uint64_t duration_ms{0};
    std::vector<std::string> warnings;
    std::vector<std::pair<std::string, std::string>> errors; ///< (target path, message)

    bool is_success() const { return failed_files == 0; }
};

void to_json(json& j, const DeployResult& r);

/// Structured outcome of restore_deploy().
struct RestoreResult {
    fs::path target_dir;
    std::size_t removed_files{0};
    std::size_t removed_dirs{0};
    uint64_t duration_ms{0};
    bool success{true};
    std::vector<std::string> warnings;
};

void to_json(json& j, const RestoreResult& r);

/// Rejects empty paths and filesystem roots. Throws SmmError(DeployError).
void validate_target_dir(const fs::path& target_dir);

/// Core loader files live beside sekiro.exe, everything else under mods/.
/// When `target_dir` is not named "mods", every asset is placed inside it.
fs::path resolve_destination_path(const fs::path& target_dir, std::string_view rel_path);

/// True when both paths sit on the same volume, so NTFS hard links are legal.
bool is_same_volume(const fs::path& source, const fs::path& target);

/// Creates the parent directories and hard-links `target` to `source`,
/// replacing any pre-existing file. Windows uses CreateHardLinkW.
void create_hard_link(const fs::path& source, const fs::path& target);

/// Number of hard links pointing at `path`, or 0 when unavailable.
uint32_t hard_link_count(const fs::path& path);

/// Deploys `plan` into `target_mods_dir`.
///
/// 1. Validates the target.
/// 2. Reads any previous manifest and deletes files that are no longer part of the plan.
/// 3. Hard-links same-volume assets, falls back to a copy across volumes or on link failure.
/// 4. Prunes emptied directories and rewrites the manifest.
///
/// `on_progress` is invoked once per mapping; the CLI streams it as JSON lines so the GUI
/// can show a real progress bar instead of an indeterminate spinner.
DeployResult execute_deploy(const DeployPlan& plan,
                            const fs::path& target_mods_dir,
                            const DeployProgressCallback& on_progress = {});

/// Removes everything SMM deployed. Prefers the manifest; also sweeps leftovers.
RestoreResult restore_deploy(const fs::path& target_mods_dir,
                             const DeployProgressCallback& on_progress = {});

} // namespace smm
