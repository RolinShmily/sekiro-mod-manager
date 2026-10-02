#include "smm/executor.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <set>
#include <system_error>

#include "smm/error.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace smm {
namespace {

std::string normalize_slashes(std::string_view path) {
    std::string result(path);
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

/// Absolute form of \`path\`, or the input when it cannot be resolved.
fs::path absolute_of(const fs::path& path) {
    std::error_code ec;
    if (path.is_absolute()) {
        return path;
    }
    const fs::path cwd = fs::current_path(ec);
    if (ec) {
        return path;
    }
    return cwd / path;
}

} // namespace

void to_json(json& j, const DeployManifest& m) {
    j = json{
        {"version", m.version},
        {"profile", m.profile},
        {"deployed_at", m.deployed_at},
        {"files", m.files},
        {"total_bytes", m.total_bytes},
    };
}

void from_json(const json& j, DeployManifest& m) {
    m.version = j.value("version", uint32_t{1});
    m.profile = j.value("profile", std::string{"default"});
    m.deployed_at = j.value("deployed_at", uint64_t{0});
    m.files = j.value("files", std::vector<std::string>{});
    m.total_bytes = j.value("total_bytes", uint64_t{0});
}

void to_json(json& j, const DeployResult& r) {
    j = json{
        {"target_dir", r.target_dir.string()},
        {"total_files", r.total_files},
        {"hard_links_created", r.hard_links_created},
        {"copied_files", r.copied_files},
        {"skipped_files", r.skipped_files},
        {"failed_files", r.failed_files},
        {"bytes_saved", r.bytes_saved},
        {"duration_ms", r.duration_ms},
        {"success", r.is_success()},
        {"warnings", r.warnings},
    };
    json errors = json::array();
    for (const auto& [path, message] : r.errors) {
        errors.push_back(json{{"path", path}, {"message", message}});
    }
    j["errors"] = std::move(errors);
}

void to_json(json& j, const RestoreResult& r) {
    j = json{
        {"target_dir", r.target_dir.string()},
        {"removed_files", r.removed_files},
        {"removed_dirs", r.removed_dirs},
        {"duration_ms", r.duration_ms},
        {"success", r.success},
        {"warnings", r.warnings},
    };
}

void validate_target_dir(const fs::path& target_dir) {
    if (target_dir.empty() || target_dir.string().find_first_not_of(" \t") == std::string::npos) {
        fail(ErrorCode::DeployError, "Target directory path cannot be empty", target_dir);
    }

    // Deploying into a filesystem root would scatter mod files across the whole drive and
    // make restore() a data-loss event, so it is refused outright.
    const fs::path absolute = absolute_of(target_dir);
    if (absolute == absolute.root_path()) {
        fail(ErrorCode::DeployError,
             "Refusing to deploy into the filesystem root: " + absolute.string(), target_dir);
    }
    if (absolute.root_name().empty() && absolute.root_directory().empty()) {
        fail(ErrorCode::DeployError,
             "Refusing to operate on a path without a directory component: " + absolute.string(),
             target_dir);
    }
}

#ifdef _WIN32
namespace {

/// Opens a file just far enough to ask the filesystem about its identity.
HANDLE open_for_query(const fs::path& path) {
    return ::CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
}

} // namespace
#endif

bool is_same_volume(const fs::path& source, const fs::path& target) {
#ifdef _WIN32
    const fs::path left = absolute_of(source);
    const fs::path right = absolute_of(target);
    if (!left.root_name().empty() && !right.root_name().empty() &&
        left.root_name() != right.root_name()) {
        return false;
    }

    // A drive letter alone is not enough: a mounted volume or a network path can share one.
    // When both endpoints exist, the volume serial number is authoritative.
    std::error_code ec;
    if (fs::exists(source, ec) && fs::exists(target, ec)) {
        const HANDLE a = open_for_query(left);
        const HANDLE b = open_for_query(right);
        if (a != INVALID_HANDLE_VALUE && b != INVALID_HANDLE_VALUE) {
            BY_HANDLE_FILE_INFORMATION info_a{};
            BY_HANDLE_FILE_INFORMATION info_b{};
            const bool ok_a = ::GetFileInformationByHandle(a, &info_a) != 0;
            const bool ok_b = ::GetFileInformationByHandle(b, &info_b) != 0;
            ::CloseHandle(a);
            ::CloseHandle(b);
            if (ok_a && ok_b) {
                return info_a.dwVolumeSerialNumber == info_b.dwVolumeSerialNumber;
            }
        } else {
            if (a != INVALID_HANDLE_VALUE) ::CloseHandle(a);
            if (b != INVALID_HANDLE_VALUE) ::CloseHandle(b);
        }
    }
    return true;
#else
    std::error_code ec;
    const fs::path left = absolute_of(source);
    fs::path right = absolute_of(target);
    const auto left_dev = fs::status(left, ec).dev();
    if (ec) {
        return true;
    }
    while (!right.empty()) {
        if (fs::exists(right, ec)) {
            const auto right_dev = fs::status(right, ec).dev();
            return ec || left_dev == right_dev;
        }
        right = right.parent_path();
    }
    return true;
#endif
}

uint32_t hard_link_count(const fs::path& path) {
#ifdef _WIN32
    const HANDLE handle = open_for_query(path);
    if (handle == INVALID_HANDLE_VALUE) {
        return 0;
    }
    BY_HANDLE_FILE_INFORMATION info{};
    const bool ok = ::GetFileInformationByHandle(handle, &info) != 0;
    ::CloseHandle(handle);
    return ok ? info.nNumberOfLinks : 0;
#else
    std::error_code ec;
    const auto count = fs::hard_link_count(path, ec);
    return ec ? 0 : static_cast<uint32_t>(count);
#endif
}

void create_hard_link(const fs::path& source, const fs::path& target) {
    std::error_code ec;
    if (const fs::path parent = target.parent_path(); !parent.empty()) {
        fs::create_directories(parent, ec);
        if (ec) {
            fail(ErrorCode::DeployError,
                 "Cannot create the destination directory: " + ec.message(), parent);
        }
    }

    // Replace whatever is already there; a leftover copy would keep the old bytes alive and
    // silently defeat the whole point of linking.
    fs::remove(target, ec);
    ec.clear();

#ifdef _WIN32
    const std::wstring source_wide = source.wstring();
    const std::wstring target_wide = target.wstring();
    if (::CreateHardLinkW(target_wide.c_str(), source_wide.c_str(), nullptr) != 0) {
        return;
    }
    const DWORD error = ::GetLastError();
    std::error_code fallback_ec;
    fs::create_hard_link(source, target, fallback_ec);
    if (fallback_ec) {
        fail(ErrorCode::DeployError,
             "Cannot hard link '" + target.string() + "' to '" + source.string() +
                 "': " + fallback_ec.message() + " (Win32 error " + std::to_string(error) + ")",
             target);
    }
#else
    fs::create_hard_link(source, target, ec);
    if (ec) {
        fail(ErrorCode::DeployError,
             "Cannot hard link '" + target.string() + "' to '" + source.string() +
                 "': " + ec.message(),
             target);
    }
#endif
}

fs::path resolve_destination_path(const fs::path& target_dir, std::string_view rel_path) {
    const std::string name = normalize_slashes(rel_path);
    const bool is_loader = [&name] {
        std::string lower;
        lower.reserve(name.size());
        for (const unsigned char c : name) {
            lower.push_back(static_cast<char>(std::tolower(c)));
        }
        return lower == "dinput8.dll" || lower == "modengine.ini";
    }();

    std::string dir_name;
    {
        const std::string raw = target_dir.filename().string();
        dir_name.reserve(raw.size());
        for (const unsigned char c : raw) {
            dir_name.push_back(static_cast<char>(std::tolower(c)));
        }
    }

    // ModEngine's own payload has to sit beside sekiro.exe to be loaded at process start;
    // every other asset is addressed by the engine relative to the mods directory.
    if (is_loader && dir_name == "mods") {
        if (const fs::path parent = target_dir.parent_path(); !parent.empty()) {
            return parent / fs::path(name);
        }
    }
    return target_dir / fs::path(name);
}

namespace {

/// Removes directories left empty by a cleanup, deepest first. The root itself is kept.
std::size_t remove_empty_subdirs(const fs::path& dir) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        return 0;
    }

    std::vector<fs::path> directories;
    fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    while (!ec && it != end) {
        std::error_code type_ec;
        if (it->is_directory(type_ec) && !type_ec) {
            directories.push_back(it->path());
        }
        it.increment(ec);
    }

    std::sort(directories.begin(), directories.end(), [](const fs::path& a, const fs::path& b) {
        return a.string().size() > b.string().size();
    });

    std::size_t removed = 0;
    for (const fs::path& candidate : directories) {
        std::error_code probe_ec;
        fs::directory_iterator contents(candidate, probe_ec);
        if (probe_ec) {
            continue;
        }
        if (contents == fs::directory_iterator()) {
            std::error_code remove_ec;
            if (fs::remove(candidate, remove_ec) && !remove_ec) {
                ++removed;
            }
        }
    }
    return removed;
}

uint64_t elapsed_ms(const std::chrono::steady_clock::time_point& start) {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start)
            .count());
}

/// Copies \`source\` over \`target\`, creating parents and replacing any existing file.
void copy_over(const fs::path& source, const fs::path& target) {
    std::error_code ec;
    if (const fs::path parent = target.parent_path(); !parent.empty()) {
        fs::create_directories(parent, ec);
    }
    fs::remove(target, ec);
    fs::copy_file(source, target, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        fail(ErrorCode::DeployError, "Cannot copy file into place: " + ec.message(), target);
    }
}

} // namespace

DeployResult execute_deploy(const DeployPlan& plan, const fs::path& target_mods_dir,
                            const DeployProgressCallback& on_progress) {
    validate_target_dir(target_mods_dir);

    const auto start = std::chrono::steady_clock::now();
    std::error_code ec;
    fs::create_directories(target_mods_dir, ec);
    if (ec) {
        fail(ErrorCode::DeployError,
             "Cannot create the target directory: " + ec.message(), target_mods_dir);
    }

    DeployResult result;
    result.target_dir = target_mods_dir;
    result.total_files = plan.mappings.size();

    const fs::path manifest_path = target_mods_dir / ".smm_manifest.json";

    std::set<std::string> planned;
    for (const auto& mapping : plan.mappings) {
        planned.insert(normalize_slashes(mapping.target_relative_path));
    }

    // Remove files this deployment no longer owns before writing the new ones, so switching
    // mod sets never leaves stale assets behind for the engine to load.
    {
        std::ifstream stream(manifest_path, std::ios::binary);
        if (stream) {
            json previous;
            try {
                stream >> previous;
            } catch (const json::exception&) {
                result.warnings.push_back(
                    "The previous deployment manifest is unreadable; stale files may remain.");
            }
            if (previous.is_object()) {
                DeployManifest old_manifest;
                try {
                    old_manifest = previous.get<DeployManifest>();
                } catch (const json::exception&) {
                    result.warnings.push_back(
                        "The previous deployment manifest is malformed; stale files may remain.");
                }
                for (const auto& old_file : old_manifest.files) {
                    if (planned.count(normalize_slashes(old_file)) != 0) {
                        continue;
                    }
                    const fs::path obsolete = resolve_destination_path(target_mods_dir, old_file);
                    std::error_code remove_ec;
                    if (fs::exists(obsolete, remove_ec)) {
                        fs::remove(obsolete, remove_ec);
                    }
                }
            }
        }
    }

    std::vector<std::string> deployed;
    deployed.reserve(plan.mappings.size());

    std::size_t done = 0;
    for (const auto& mapping : plan.mappings) {
        const std::string relative = normalize_slashes(mapping.target_relative_path);
        const fs::path destination = resolve_destination_path(target_mods_dir, relative);

        if (on_progress) {
            on_progress(DeployProgress{done, plan.mappings.size(), relative});
        }
        ++done;

        std::error_code exists_ec;
        if (!fs::exists(mapping.source_path, exists_ec)) {
            const std::string message = "Source file is missing: " + mapping.source_path.string();
            result.warnings.push_back(message);
            result.errors.emplace_back(relative, message);
            ++result.failed_files;
            continue;
        }

        std::error_code size_ec;
        const uint64_t file_size = static_cast<uint64_t>(fs::file_size(mapping.source_path, size_ec));

        if (is_same_volume(mapping.source_path, target_mods_dir)) {
            try {
                // Qualified: comparable overloads exist in std::filesystem, and ADL on
                // std::filesystem::path would otherwise make this call ambiguous.
                smm::create_hard_link(mapping.source_path, destination);
                ++result.hard_links_created;
                result.bytes_saved += size_ec ? 0 : file_size;
                deployed.push_back(relative);
                continue;
            } catch (const SmmError& e) {
                // A link can still fail on a filesystem that reports the same volume but does
                // not support links (FAT32, some network shares); a copy is always correct.
                result.warnings.push_back("Hard link failed for '" + relative +
                                          "'; falling back to a physical copy: " + e.what());
            }
        } else {
            result.warnings.push_back("'" + relative +
                                      "' crosses volumes; falling back to a physical copy.");
        }

        try {
            copy_over(mapping.source_path, destination);
            ++result.copied_files;
            deployed.push_back(relative);
        } catch (const SmmError& e) {
            result.errors.emplace_back(relative, e.what());
            ++result.failed_files;
        }
    }

    remove_empty_subdirs(target_mods_dir);

    DeployManifest manifest;
    manifest.version = 1;
    manifest.profile = plan.active_profile;
    manifest.deployed_at = now_epoch_seconds();
    manifest.files = deployed;
    manifest.total_bytes = result.bytes_saved;

    std::ofstream stream(manifest_path, std::ios::binary | std::ios::trunc);
    if (stream) {
        stream << json(manifest).dump(2);
        stream.flush();
    } else {
        result.warnings.push_back("Could not write the deployment manifest; restore() will fall "
                                  "back to sweeping the target directory.");
    }

    result.duration_ms = elapsed_ms(start);
    return result;
}

RestoreResult restore_deploy(const fs::path& target_mods_dir, const DeployProgressCallback& on_progress) {
    validate_target_dir(target_mods_dir);

    const auto start = std::chrono::steady_clock::now();
    RestoreResult result;
    result.target_dir = target_mods_dir;

    std::error_code ec;
    if (!fs::exists(target_mods_dir, ec)) {
        result.duration_ms = elapsed_ms(start);
        result.warnings.push_back("The target directory does not exist; nothing to restore.");
        return result;
    }

    const fs::path manifest_path = target_mods_dir / ".smm_manifest.json";

    std::ifstream stream(manifest_path, std::ios::binary);
    if (stream) {
        json parsed;
        try {
            stream >> parsed;
        } catch (const json::exception&) {
            result.warnings.push_back("The deployment manifest is unreadable; the whole target "
                                      "directory will be swept instead.");
        }
        stream.close();

        if (parsed.is_object()) {
            DeployManifest manifest;
            try {
                manifest = parsed.get<DeployManifest>();
            } catch (const json::exception&) {
                result.warnings.push_back("The deployment manifest is malformed; the whole target "
                                          "directory will be swept instead.");
            }
            const std::size_t total = manifest.files.size();
            std::size_t index = 0;
            for (const auto& file : manifest.files) {
                if (on_progress) {
                    on_progress(DeployProgress{index, total, file});
                }
                ++index;
                const fs::path deployed = resolve_destination_path(target_mods_dir, file);
                std::error_code remove_ec;
                if (fs::remove(deployed, remove_ec) && !remove_ec) {
                    ++result.removed_files;
                }
            }
        }
        fs::remove(manifest_path, ec);
    }

    // Anything still lying around was not tracked (an interrupted deploy, a hand-copied mod,
    // a lost manifest). Leaving it would mean the "clean" state is not actually clean.
    {
        std::error_code walk_ec;
        fs::recursive_directory_iterator it(
            target_mods_dir, fs::directory_options::skip_permission_denied, walk_ec);
        const fs::recursive_directory_iterator end;
        while (!walk_ec && it != end) {
            std::error_code type_ec;
            const fs::path path = it->path();
            const bool is_file = it->is_regular_file(type_ec) || it->is_symlink(type_ec);
            if (is_file && path.filename() != ".smm_manifest.json") {
                std::error_code remove_ec;
                if (fs::remove(path, remove_ec) && !remove_ec) {
                    ++result.removed_files;
                }
            }
            it.increment(walk_ec);
        }
    }

    result.removed_dirs = remove_empty_subdirs(target_mods_dir);
    result.duration_ms = elapsed_ms(start);
    return result;
}

} // namespace smm
