// Focused regression tests for the business rules the CLI and the GUI both depend on:
// mods-directory resolution, id sanitation / rename, priority reindexing, deployment
// manifest reading, and doctor re-use of a pre-scanned staging directory.
#include "smm/deploy.hpp"
#include "smm/doctor.hpp"
#include "smm/error.hpp"
#include "smm/executor.hpp"
#include "smm/loader.hpp"
#include "smm/manager.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace smm;

namespace {

void check(bool value, const char* name) {
    if (!value) throw std::runtime_error(name);
    std::cout << "PASS " << name << '\n';
}

void write_file(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << value;
    if (!file) throw std::runtime_error("fixture write failed");
}

/// A staged mod that passes metadata loading and normalisation without touching the game.
fs::path make_mod(const fs::path& staging, const std::string& id, uint32_t priority) {
    const fs::path dir = staging / id;
    write_file(dir / ".smm_mod.json",
               "{\"id\":\"" + id + "\",\"name\":\"" + id + "\",\"version\":\"1.0\","
               "\"author\":\"tests\",\"category\":\"weapon_skin\",\"enabled\":true,"
               "\"priority\":" + std::to_string(priority) + "}");
    write_file(dir / "parts" / "wp_a_0000.partsbnd.dcx", "fixture payload");
    return dir;
}

template <typename Fn>
bool throws_code(ErrorCode expected, Fn&& body) {
    try {
        body();
    } catch (const SmmError& error) {
        return error.code() == expected;
    } catch (...) {
        return false;
    }
    return false;
}

} // namespace

int main() {
    try {
        // --- resolve_mods_dir -----------------------------------------------------------
        const fs::path game = fs::path("C:/games/Sekiro");
        check(resolve_mods_dir(game) == game / "mods", "game root maps to its mods directory");
        check(resolve_mods_dir(game / "mods") == game / "mods", "mods directory stays unchanged");
        check(resolve_mods_dir(game / "MODS") == game / "MODS", "mods detection ignores case");
        check(resolve_mods_dir({}).empty(), "empty game path stays empty");

        // --- sanitize_mod_id ------------------------------------------------------------
        check(ModManager::sanitize_mod_id("  dream-of-the-damned ") == "dream-of-the-damned",
              "id whitespace is trimmed");
        check(ModManager::sanitize_mod_id("bad/na\\me:*?\"<>|") == "bad-na-me-------",
              "path separators and illegal characters become dashes");
        check(ModManager::sanitize_mod_id("   ").empty(), "blank id sanitises to empty");

        // --- fixture staging ------------------------------------------------------------
        const auto root = fs::temp_directory_path() /
                          ("smm-core-business-" + std::to_string(now_epoch_seconds()));
        const fs::path staging = root / "staging";
        make_mod(staging, "alpha", 100);
        make_mod(staging, "beta", 200);
        make_mod(staging, "gamma", 300);

        // --- rename_mod ----------------------------------------------------------------
        const ModInfo renamed = ModManager::rename_mod(staging, "alpha", "alpha/renamed");
        check(renamed.id == "alpha-renamed", "rename sanitises the requested id");
        check(fs::exists(staging / "alpha-renamed" / ".smm_mod.json") && !fs::exists(staging / "alpha"),
              "rename moves the directory");
        check(ModManager::load_mod_info(staging / "alpha-renamed").id == "alpha-renamed",
              "rename rewrites the metadata id");
        check(throws_code(ErrorCode::ModAlreadyExists,
                          [&] { ModManager::rename_mod(staging, "alpha-renamed", "beta"); }),
              "rename refuses an existing id");
        check(throws_code(ErrorCode::InvalidArgument,
                          [&] { ModManager::rename_mod(staging, "beta", "   "); }),
              "rename refuses an empty id");
        check(throws_code(ErrorCode::ModNotFound,
                          [&] { ModManager::rename_mod(staging, "missing", "whatever"); }),
              "rename reports a missing mod");
        check(ModManager::rename_mod(staging, "beta", "beta").id == "beta",
              "renaming to the same id is a no-op");

        // --- reindex_priorities ---------------------------------------------------------
        const auto updated = ModManager::reindex_priorities(
            staging, {"gamma", "alpha-renamed", "beta", "vanished"});
        check(updated.size() == 3, "reindex writes every existing mod");
        check(ModManager::load_mod_info(staging / "gamma").priority == 10, "first row wins");
        check(ModManager::load_mod_info(staging / "alpha-renamed").priority == 20,
              "second row follows");
        check(ModManager::load_mod_info(staging / "beta").priority == 30, "third row follows");
        check(throws_code(ErrorCode::InvalidArgument,
                          [&] { ModManager::reindex_priorities(staging, {"gamma"}, 0); }),
              "reindex rejects a zero step");

        // --- load_deploy_manifest -------------------------------------------------------
        const fs::path mods_dir = root / "game" / "mods";
        fs::create_directories(mods_dir);
        check(!load_deploy_manifest(mods_dir).has_value(), "missing manifest yields nullopt");
        write_file(mods_dir / std::string(DEPLOY_MANIFEST_NAME),
                   "{\"version\":1,\"profile\":\"default\",\"deployed_at\":1,"
                   "\"files\":[\"parts/a.partsbnd.dcx\"],\"total_bytes\":1234}");
        const auto manifest = load_deploy_manifest(mods_dir);
        check(manifest.has_value() && manifest->files.size() == 1 && manifest->total_bytes == 1234,
              "manifest round-trips through the core loader");
        write_file(mods_dir / std::string(DEPLOY_MANIFEST_NAME), "{ not json");
        check(throws_code(ErrorCode::Json, [&] { load_deploy_manifest(mods_dir); }),
              "malformed manifest raises a Json error");

        // --- doctor with a pre-scanned staging directory --------------------------------
        const ScanOutcome scan = ModLoader::scan_mods_directory(staging);
        check(scan.mods.size() == 3, "fixture scans three mods");
        const HealthReport direct = diagnose_environment(root / "game", staging, true);
        const HealthReport reused = diagnose_environment(root / "game", staging, true, &scan);
        check(direct.items.size() == reused.items.size(), "pre-scanned doctor keeps the item count");
        bool same = true;
        for (std::size_t i = 0; i < direct.items.size() && i < reused.items.size(); ++i) {
            same = same && direct.items[i].id == reused.items[i].id &&
                   direct.items[i].status == reused.items[i].status;
        }
        check(same, "pre-scanned doctor reports identical items");

        std::error_code ec;
        fs::remove_all(root, ec);
        std::cout << "all core business tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
