#include "smm/doctor.hpp"
#include "smm/runtime_doctor.hpp"
#include "smm/manager.hpp"
#include "smm/executor.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace smm;
void check(bool value, const char* name) {
    if (!value) throw std::runtime_error(name);
    std::cout << "PASS " << name << '\n';
}
void write(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary); file << value;
    if (!file) throw std::runtime_error("Fixture write failed");
}
int main() {
    try {
        RuntimeSnapshot snapshot;
        snapshot.supported = true;
        snapshot.processes.push_back({1, "launcher", {}, false});
        check(diagnose_launch_runtime(snapshot).is_healthy(), "clean launcher passes");
        snapshot.processes.push_back({2, "steam", {}, true});
        check(diagnose_launch_runtime(snapshot).error_count == 1, "Steam PreferSystem32 blocks");
        snapshot.processes.back().prefer_system32 = false;
        snapshot.processes.front().prefer_system32 = true;
        check(!diagnose_launch_runtime(snapshot).is_healthy(), "SMM PreferSystem32 blocks");
        snapshot.processes.front().prefer_system32 = false;
        snapshot.processes.back().prefer_system32.reset();
        check(!diagnose_launch_runtime(snapshot).is_healthy(), "unreadable policy fails closed");
        snapshot.processes.back().prefer_system32 = false;
        snapshot.processes.push_back({3,"game",{},false,true,false});
        check(!diagnose_launch_runtime(snapshot).is_healthy(), "system-only hook blocks");
        snapshot.processes.back().local_hook_loaded = true;
        check(diagnose_launch_runtime(snapshot).is_healthy(), "local plus system DLL passes");
        snapshot.processes.back().modules_readable = false;
        check(!diagnose_launch_runtime(snapshot).is_healthy(), "unreadable game modules block");
        snapshot.enumeration_failure = "Access denied";
        check(diagnose_launch_runtime(snapshot).error_count == 2, "enumeration failure blocks");
        check(!diagnose_launch_runtime({}).is_healthy(), "unsupported runtime blocks");
        RuntimeSnapshot empty; empty.supported = true;
        check(!diagnose_launch_runtime(empty).is_healthy(), "missing launcher observation blocks");

        const auto ini = parse_modengine_ini_text("[files]\nuseModOverrideDirectory = 0\nloadLooseParams = 1\nmodOverrideDirectory = \"\\mods\"\n[mods]\nuseModOverrideDirectory=1\n");
        check(ini.use_mod_override == false, "INI reads override switch from files section");
        check(ini.load_loose_params == true && ini.mod_override_directory == "\\mods", "INI trims key whitespace");
        const auto patched = patch_modengine_ini_text(ini.raw_content,"mods");
        check(parse_modengine_ini_text(patched).use_mod_override == true, "repair enables actual override key");
        check(patched == patch_modengine_ini_text(patched,"mods"), "INI repair is idempotent");
        const auto canonical = parse_modengine_ini_text(render_default_modengine_ini("mods"));
        check(canonical.use_mod_override == true && canonical.cache_paths == true, "default INI uses engine keys");

        const auto root = fs::temp_directory_path() / ("smm-doctor-test-" + std::to_string(now_epoch_seconds()));
        const auto game = root / "game"; const auto staging = root / "staging";
        fs::create_directories(game / "mods"); fs::create_directories(staging / "sample");
        write(game / "sekiro.exe", "fixture, never executed");
        write(game / "dinput8.dll", std::string(20000, 'x'));
        write(game / "modengine.ini", render_default_modengine_ini("mods"));
        ModInfo info; info.id="sample"; info.name="Sample"; info.enabled=true;
        ModManager::save_mod_info(staging / "sample", info);
        write(staging / "sample/parts/wp_a_0300.partsbnd.dcx", "abcdefgh");
        auto report = diagnose_environment(game,staging,true);
        check(report.error_count == 1, "missing deployed resource blocks");
        write(game / "mods/parts/wp_a_0300.partsbnd.dcx", "abcdefgh");
        check(diagnose_environment(game,staging,true).is_healthy(), "matching copied resource passes");
        write(game / "mods/parts/wp_a_0300.partsbnd.dcx", "abcdEfgh");
        check(!diagnose_environment(game,staging,true).is_healthy(), "same-size changed resource blocks");
        fs::remove(game / "mods/parts/wp_a_0300.partsbnd.dcx");
        std::error_code linkError;
        fs::create_hard_link(staging / "sample/parts/wp_a_0300.partsbnd.dcx",
                             game / "mods/parts/wp_a_0300.partsbnd.dcx", linkError);
        if (!linkError) check(diagnose_environment(game,staging,true).is_healthy(), "matching hardlink passes");
        else std::cout << "SKIP hardlink-specific case: " << linkError.message() << '\n';
        write(game / "mods/.smm_manifest.json", "{\"files\":[\"parts/old.partsbnd.dcx\"]}");
        check(!diagnose_environment(game,staging,true).is_healthy(), "stale deployment blocks");
        write(game / "mods/.smm_manifest.json", "invalid json");
        check(!diagnose_environment(game,staging,true).is_healthy(), "invalid manifest blocks");
        fs::remove(game / "mods/.smm_manifest.json");
        write(game / "modengine.ini", "[files]\nuseModOverrideDirectory = 0\nloadLooseParams=1\nmodOverrideDirectory=\\mods\n");
        check(!diagnose_environment(game,staging,true).is_healthy(), "disabled override blocks");
        fs::create_directories(game / "other");
        write(game / "modengine.ini", "[files]\nuseModOverrideDirectory=1\nloadLooseParams=1\nmodOverrideDirectory=\\other\n");
        check(!diagnose_environment(game,staging,true).is_healthy(), "wrong engine target blocks even if folder exists");
        write(game / "modengine.ini", render_default_modengine_ini("mods"));
        write(staging / "broken/.smm_mod.json", "invalid");
        check(!diagnose_environment(game,staging,true).is_healthy(), "unreadable mod remains actionable");
        fs::remove_all(root);
        std::cout << "RESULT all doctor regression checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
