#include "smm/exporter.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <random>
#include <system_error>

#include <miniz.h>

#include "smm/error.hpp"
#include "smm/extractor.hpp"
#include "smm/loader.hpp"
#include "smm/manager.hpp"
#include "smm/normalizer.hpp"

namespace smm {
namespace {

bool ends_with_ignore_case(std::string_view text, std::string_view suffix) {
    if (text.size() < suffix.size()) {
        return false;
    }
    const std::size_t offset = text.size() - suffix.size();
    for (std::size_t i = 0; i < suffix.size(); ++i) {
        const unsigned char a = static_cast<unsigned char>(text[offset + i]);
        const unsigned char b = static_cast<unsigned char>(suffix[i]);
        if (std::tolower(a) != std::tolower(b)) {
            return false;
        }
    }
    return true;
}

/// Replaces '\\' with '/', because zip entries are slash-separated regardless of platform.
std::string to_archive_path(std::string_view path) {
    std::string result(path);
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

std::FILE* open_read(const fs::path& path) {
#ifdef _WIN32
    std::FILE* file = nullptr;
    if (::_wfopen_s(&file, path.wstring().c_str(), L"rb") != 0) {
        return nullptr;
    }
    return file;
#else
    return std::fopen(path.string().c_str(), "rb");
#endif
}

std::FILE* open_write(const fs::path& path) {
#ifdef _WIN32
    std::FILE* file = nullptr;
    if (::_wfopen_s(&file, path.wstring().c_str(), L"wb") != 0) {
        return nullptr;
    }
    return file;
#else
    return std::fopen(path.string().c_str(), "wb");
#endif
}

/// Zip compression level for exports. Assets (dcx/tpf/bnd) are already compressed, so pushing
/// the level higher buys almost nothing while costing a lot of time on multi-GB packs.
constexpr mz_uint kZipCompressionLevel = static_cast<mz_uint>(MZ_DEFAULT_LEVEL);

/// RAII zip writer.
///
/// Finalising and closing on every path matters: a partially written archive that is left
/// behind would look like a successful export to the caller.
class ZipWriter {
public:
    explicit ZipWriter(const fs::path& output) : file_(open_write(output)), path_(output) {
        if (file_ == nullptr) {
            fail(ErrorCode::ExportError, "Cannot create the output archive", output);
        }
        if (mz_zip_writer_init_cfile(&zip_, file_, 0) == MZ_FALSE) {
            std::fclose(file_);
            file_ = nullptr;
            fail(ErrorCode::ExportError, "Cannot initialise the ZIP writer", output);
        }
        opened_ = true;
    }

    ~ZipWriter() {
        if (opened_) {
            mz_zip_writer_finalize_archive(&zip_);
            mz_zip_writer_end(&zip_);
        }
        if (file_ != nullptr) {
            std::fclose(file_);
        }
    }

    ZipWriter(const ZipWriter&) = delete;
    ZipWriter& operator=(const ZipWriter&) = delete;

    void add_text(const std::string& archive_name, const std::string& content) {
        if (mz_zip_writer_add_mem(&zip_, archive_name.c_str(), content.data(), content.size(),
                                  kZipCompressionLevel) == MZ_FALSE) {
            fail(ErrorCode::ExportError, "Failed to add '" + archive_name + "' to the archive",
                 path_);
        }
    }

    /// Streams a file straight from disk, so exporting a multi-GB mod never loads it into RAM.
    void add_file(const std::string& archive_name, const fs::path& source) {
        std::error_code ec;
        const uint64_t size = static_cast<uint64_t>(fs::file_size(source, ec));
        if (ec) {
            fail(ErrorCode::Io, "Cannot read the file being exported: " + ec.message(), source);
        }

        std::FILE* input = open_read(source);
        if (input == nullptr) {
            fail(ErrorCode::Io, "Cannot open the file being exported", source);
        }

        const mz_bool ok = mz_zip_writer_add_cfile(
            &zip_, archive_name.c_str(), input, size, nullptr, nullptr, 0, kZipCompressionLevel,
            nullptr, 0, nullptr, 0);
        std::fclose(input);

        if (ok == MZ_FALSE) {
            fail(ErrorCode::ExportError, "Failed to add '" + archive_name + "' to the archive",
                 source);
        }
    }

private:
    mz_zip_archive zip_{};
    std::FILE* file_{nullptr};
    fs::path path_;
    bool opened_{false};
};

/// A unique scratch name in \`directory\`, so a failed export never overwrites the target.
fs::path make_temp_path(const fs::path& directory, std::string_view prefix) {
    std::random_device device;
    std::mt19937_64 generator(device() ^ static_cast<uint64_t>(
                                              std::chrono::steady_clock::now()
                                                  .time_since_epoch()
                                                  .count()));
    std::error_code ec;
    for (int attempt = 0; attempt < 64; ++attempt) {
        char suffix[32] = {};
        std::snprintf(suffix, sizeof(suffix), "%08llx",
                      static_cast<unsigned long long>(generator() & 0xFFFFFFFFULL));
        const fs::path candidate = directory / (std::string(prefix) + suffix + ".tmp");
        if (!fs::exists(candidate, ec)) {
            return candidate;
        }
    }
    return directory / (std::string(prefix) + "export.tmp");
}

/// Finalises a temp file into its destination, replacing whatever was there.
void commit_file(const fs::path& temp_path, const fs::path& final_path) {
    std::error_code ec;
    fs::remove(final_path, ec);
    ec.clear();
    fs::rename(temp_path, final_path, ec);
    if (ec) {
        fs::copy_file(temp_path, final_path, fs::copy_options::overwrite_existing, ec);
        std::error_code cleanup_ec;
        fs::remove(temp_path, cleanup_ec);
        if (ec) {
            fail(ErrorCode::ExportError, "Failed to finalise the export: " + ec.message(),
                 final_path);
        }
    }
}

/// Resolves the requested output path, appending \`extension\` when the caller gave none.
fs::path resolve_output_path(const fs::path& output, std::string_view extension) {
    std::error_code ec;
    if (fs::is_directory(output, ec) && !ec) {
        return output / ("export" + std::string(extension));
    }
    if (output.has_extension()) {
        return output;
    }
    fs::path result = output;
    result += extension;
    return result;
}

ModPackItem pack_item_from(const ModInfo& info, const StagedMod& staged) {
    ModPackItem item;
    item.id = info.id;
    item.name = info.name;
    item.version = info.version;
    item.author = info.author;
    item.category = info.category;
    item.priority = info.priority;
    item.enabled = info.enabled;
    item.source_url = info.source_url;
    item.homepage = info.homepage;
    item.description = info.description;
    item.file_count = staged.assets.size();
    item.total_bytes = staged.total_bytes();
    return item;
}

/// Adds the metadata container (.smm_mod.json) and bundles the preview image if present.
void add_metadata(ZipWriter& writer, const std::string& prefix, const ModInfo& info, const fs::path& mod_dir = {}) {
    ModInfo portable = info;
    portable.root_path.reset();
    const std::string payload = json(portable).dump(2);
    writer.add_text(prefix + ".smm_mod.json", payload);

    if (!mod_dir.empty() && info.preview_image.has_value() && !info.preview_image->empty()) {
        std::error_code ec;
        const fs::path img_path = mod_dir / *info.preview_image;
        if (fs::exists(img_path, ec)) {
            writer.add_file(prefix + *info.preview_image, img_path);
        }
    }
}

/// Adds the normalised assets of \`staged\` under \`prefix\`.
void add_assets(ZipWriter& writer, const std::string& prefix, const StagedMod& staged,
                const ExportProgressCallback& on_progress, std::size_t& done, std::size_t total) {
    for (const auto& asset : staged.assets) {
        writer.add_file(prefix + to_archive_path(asset.relative_path), asset.source_path);
        ++done;
        if (on_progress) {
            on_progress(done, total, asset.relative_path);
        }
    }
}

/// Adds the preserved documentation and, when requested, the untouched original download.
void add_docs_and_source(ZipWriter& writer, const std::string& prefix, const fs::path& mod_dir,
                         const ExportOptions& options) {
    for (const auto& [name, path] : collect_documentation_files(mod_dir)) {
        if (name == "mod.json" || name == ".smm_mod.json") {
            continue;
        }
        writer.add_file(prefix + name, path);
    }

    if (!options.include_source) {
        return;
    }

    const fs::path source_dir = mod_dir / ".smm_source";
    std::error_code ec;
    if (!fs::is_directory(source_dir, ec)) {
        return;
    }

    fs::recursive_directory_iterator it(source_dir, fs::directory_options::skip_permission_denied,
                                        ec);
    const fs::recursive_directory_iterator end;
    while (!ec && it != end) {
        std::error_code type_ec;
        if (it->is_regular_file(type_ec) && !type_ec) {
            std::error_code rel_ec;
            const fs::path relative = fs::relative(it->path(), source_dir, rel_ec);
            if (!rel_ec) {
                writer.add_file(prefix + ".smm_source/" + to_archive_path(path_to_utf8(relative)),
                                it->path());
            }
        }
        it.increment(ec);
    }
}

/// Locates \`smm_pack.json\`, tolerating a pack that nests it one level down.
fs::path find_manifest_file(const fs::path& root) {
    std::error_code ec;
    if (fs::is_regular_file(root / "smm_pack.json", ec)) {
        return root / "smm_pack.json";
    }

    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    while (!ec && it != end) {
        std::error_code type_ec;
        if (it->is_regular_file(type_ec) && !type_ec) {
            std::string name = path_to_utf8(it->path().filename());
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (name == "smm_pack.json") {
                return it->path();
            }
        }
        it.increment(ec);
    }
    return {};
}

/// Finds the folder holding one mod of a pack: by id, under mods/, then by its own metadata.
fs::path locate_mod_folder(const fs::path& base, const std::string& mod_id) {
    std::error_code ec;
    const fs::path direct = base / mod_id;
    if (fs::is_directory(direct, ec)) {
        return direct;
    }
    const fs::path under_mods = base / "mods" / mod_id;
    if (fs::is_directory(under_mods, ec)) {
        return under_mods;
    }

    fs::recursive_directory_iterator it(base, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    while (!ec && it != end) {
        std::error_code type_ec;
        if (it->is_directory(type_ec) && !type_ec && it.depth() <= 4) {
            const fs::path candidate = it->path();
            if (ModLoader::is_mod_directory(candidate)) {
                try {
                    const ModInfo info = ModLoader::load_mod_info(candidate);
                    if (info.id == mod_id) {
                        return candidate;
                    }
                } catch (const SmmError&) {
                }
            }
        }
        it.increment(ec);
    }
    return {};
}

} // namespace

void to_json(json& j, const ModPackItem& i) {
    j = json{
        {"id", i.id},
        {"name", i.name},
        {"version", i.version},
        {"author", i.author},
        {"category", i.category},
        {"priority", i.priority},
        {"enabled", i.enabled},
        {"file_count", i.file_count},
        {"total_bytes", i.total_bytes},
    };
    if (i.source_url) j["source_url"] = *i.source_url;
    if (i.homepage) j["homepage"] = *i.homepage;
    if (i.description) j["description"] = *i.description;
}

void from_json(const json& j, ModPackItem& i) {
    i.id = j.value("id", std::string{});
    i.name = j.value("name", std::string{});
    i.version = j.value("version", std::string{});
    i.author = j.value("author", std::string{});
    i.category = j.value("category", std::string{});
    i.priority = j.value("priority", uint32_t{100});
    i.enabled = j.value("enabled", true);
    i.file_count = j.value("file_count", std::size_t{0});
    i.total_bytes = j.value("total_bytes", uint64_t{0});
    if (const auto it = j.find("source_url"); it != j.end() && it->is_string()) {
        i.source_url = it->get<std::string>();
    }
    if (const auto it = j.find("homepage"); it != j.end() && it->is_string()) {
        i.homepage = it->get<std::string>();
    }
    if (const auto it = j.find("description"); it != j.end() && it->is_string()) {
        i.description = it->get<std::string>();
    }
}

void to_json(json& j, const ModPackManifest& m) {
    j = json{
        {"format_version", m.format_version},
        {"name", m.name},
        {"version", m.version},
        {"created_at", m.created_at},
        {"mods", m.mods},
    };
    if (m.author) j["author"] = *m.author;
    if (m.description) j["description"] = *m.description;
}

void from_json(const json& j, ModPackManifest& m) {
    m.format_version = j.value("format_version", uint32_t{1});
    m.name = j.value("name", std::string{});
    m.version = j.value("version", std::string{"1.0.0"});
    m.created_at = j.value("created_at", uint64_t{0});
    m.mods = j.value("mods", std::vector<ModPackItem>{});
    if (const auto it = j.find("author"); it != j.end() && it->is_string()) {
        m.author = it->get<std::string>();
    }
    if (const auto it = j.find("description"); it != j.end() && it->is_string()) {
        m.description = it->get<std::string>();
    }
}

fs::path export_single_mod(const fs::path& staging_dir, const std::string& mod_id,
                           const fs::path& output_file, const ExportOptions& options,
                           const ExportProgressCallback& on_progress) {
    const fs::path mod_dir = ModManager::find_mod_dir(staging_dir, mod_id);
    const ModInfo info = ModLoader::load_mod_info(mod_dir);
    const StagedMod staged = ModLoader::scan_mod(mod_dir);

    fs::path final_path = resolve_output_path(output_file, ".zip");
    if (final_path.has_extension() &&
        !ends_with_ignore_case(path_to_utf8(final_path.extension()), ".zip")) {
        final_path.replace_extension(".zip");
    }

    std::error_code ec;
    const fs::path parent = final_path.parent_path().empty() ? fs::path(".") : final_path.parent_path();
    fs::create_directories(parent, ec);
    const fs::path temp_path = make_temp_path(parent, ".smm_export_");

    std::size_t done = 0;
    const std::size_t total = staged.assets.size();
    try {
        ZipWriter writer(temp_path);
        add_metadata(writer, "", info, mod_dir);
        add_assets(writer, "", staged, on_progress, done, total);
        add_docs_and_source(writer, "", mod_dir, options);
    } catch (...) {
        std::error_code cleanup_ec;
        fs::remove(temp_path, cleanup_ec);
        throw;
    }

    commit_file(temp_path, final_path);
    return final_path;
}

fs::path export_modpack(const fs::path& staging_dir, const std::vector<std::string>& mod_ids,
                        const fs::path& output_file, const std::string& pack_name,
                        const std::string& description, const ExportOptions& options,
                        const ExportProgressCallback& on_progress) {
    if (mod_ids.empty()) {
        fail(ErrorCode::ExportError, "Cannot export a modpack with no mods selected");
    }

    struct LoadedMod {
        fs::path dir;
        ModInfo info;
        StagedMod staged;
    };
    std::vector<LoadedMod> loaded;
    loaded.reserve(mod_ids.size());
    std::size_t total_assets = 0;
    for (const auto& id : mod_ids) {
        LoadedMod entry;
        entry.dir = ModManager::find_mod_dir(staging_dir, id);
        entry.info = ModLoader::load_mod_info(entry.dir);
        entry.staged = ModLoader::scan_mod(entry.dir);
        total_assets += entry.staged.assets.size();
        loaded.push_back(std::move(entry));
    }

    ModPackManifest manifest;
    manifest.format_version = 1;
    manifest.name = pack_name;
    manifest.description = description;
    manifest.created_at = now_epoch_seconds();
    for (const auto& entry : loaded) {
        manifest.mods.push_back(pack_item_from(entry.info, entry.staged));
    }

    fs::path final_path = resolve_output_path(output_file, ".smmpack");
    if (final_path.has_extension() &&
        !ends_with_ignore_case(path_to_utf8(final_path.extension()), ".smmpack")) {
        final_path.replace_extension(".smmpack");
    }

    std::error_code ec;
    const fs::path parent = final_path.parent_path().empty() ? fs::path(".") : final_path.parent_path();
    fs::create_directories(parent, ec);
    const fs::path temp_path = make_temp_path(parent, ".smm_pack_");

    std::size_t done = 0;
    try {
        ZipWriter writer(temp_path);
        writer.add_text("smm_pack.json", json(manifest).dump(2));

        for (const auto& entry : loaded) {
            const std::string prefix = entry.info.id + "/";
            add_metadata(writer, prefix, entry.info, entry.dir);
            add_assets(writer, prefix, entry.staged, on_progress, done, total_assets);
            add_docs_and_source(writer, prefix, entry.dir, options);
        }
    } catch (...) {
        std::error_code cleanup_ec;
        fs::remove(temp_path, cleanup_ec);
        throw;
    }

    commit_file(temp_path, final_path);
    return final_path;
}

ModPackImportResult import_modpack(const fs::path& pack_file, const fs::path& staging_dir,
                                   bool overwrite) {
    std::error_code ec;
    if (!fs::exists(pack_file, ec)) {
        fail(ErrorCode::Io, "Modpack file does not exist: " + path_to_utf8(pack_file), pack_file);
    }

    fs::create_directories(staging_dir, ec);

    const fs::path workspace = staging_dir / (".smm_modpack_" + std::to_string(now_epoch_seconds()));
    const fs::path unpack_dir = workspace / "unpacked";
    fs::create_directories(unpack_dir, ec);

    struct WorkspaceGuard {
        fs::path path;
        ~WorkspaceGuard() {
            std::error_code cleanup_ec;
            fs::remove_all(path, cleanup_ec);
        }
    } guard{workspace};

    extract_zip(pack_file, unpack_dir);

    const fs::path manifest_path = find_manifest_file(unpack_dir);
    if (manifest_path.empty()) {
        fail(ErrorCode::InvalidMetadata, "The modpack does not contain smm_pack.json", pack_file);
    }

    ModPackManifest manifest;
    {
        std::ifstream stream(manifest_path, std::ios::binary);
        if (!stream) {
            fail(ErrorCode::Io, "Cannot open the modpack manifest", manifest_path);
        }
        json parsed;
        try {
            stream >> parsed;
        } catch (const json::exception& e) {
            fail(ErrorCode::InvalidMetadata,
                 std::string("Invalid smm_pack.json: ") + e.what(), manifest_path);
        }
        try {
            manifest = parsed.get<ModPackManifest>();
        } catch (const json::exception& e) {
            fail(ErrorCode::InvalidMetadata,
                 std::string("Unexpected modpack manifest shape: ") + e.what(), manifest_path);
        }
    }

    if (manifest.format_version > 1) {
        fail(ErrorCode::InvalidMetadata,
             "This modpack was written by a newer SMM (format version " +
                 std::to_string(manifest.format_version) + ")",
             manifest_path);
    }
    if (manifest.mods.empty()) {
        fail(ErrorCode::InvalidMetadata, "The modpack manifest lists no mods", manifest_path);
    }

    // Fail before touching staging when any mod would collide, so a refused pack cannot leave
    // the library half-imported.
    if (!overwrite) {
        for (const auto& item : manifest.mods) {
            if (fs::exists(staging_dir / item.id, ec)) {
                fail(ErrorCode::ModAlreadyExists,
                     "A mod with this id is already staged: " + item.id, staging_dir / item.id);
            }
        }
    }

    ModPackImportResult result;
    result.manifest = manifest;

    const fs::path pack_root = manifest_path.parent_path();
    for (const auto& item : manifest.mods) {
        const fs::path mod_folder = locate_mod_folder(pack_root, item.id);
        if (mod_folder.empty()) {
            fail(ErrorCode::ModNotFound,
                 "Mod '" + item.id + "' is listed in the manifest but missing from the pack",
                 pack_file);
        }

        ImportOptions options;
        options.id = item.id;
        options.name = item.name;
        options.priority = item.priority;
        options.source_url = item.source_url;
        options.overwrite = true;

        ImportResult imported = import_mod(mod_folder, staging_dir, options);

        // The manifest records the user's curation, not just what the files say, so its
        // enabled flag and priority win over anything inferred during the import.
        const fs::path final_dir = staging_dir / item.id;
        imported.info.enabled = item.enabled;
        imported.info.priority = item.priority;
        if (item.source_url) {
            imported.info.source_url = item.source_url;
        }
        imported.info.root_path = final_dir;
        ModManager::save_mod_info(final_dir, imported.info);
        imported.mod_dir = final_dir;

        result.mods.push_back(std::move(imported));
    }

    return result;
}

} // namespace smm
