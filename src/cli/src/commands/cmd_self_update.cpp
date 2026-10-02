#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "commands.hpp"
#include "smm/error.hpp"
#include "smm/extractor.hpp"

namespace smm::cli {

namespace fs = std::filesystem;

namespace {

#ifdef _WIN32
void wait_for_process_exit(DWORD pid, DWORD timeout_ms) {
    if (pid == 0) return;
    HANDLE hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess != NULL) {
        WaitForSingleObject(hProcess, timeout_ms);
        CloseHandle(hProcess);
    }
    // Small extra pause for OS to release file locks
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
}

void launch_process_detached(const fs::path& exe_path, const fs::path& work_dir) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::wstring cmd = L"\"" + exe_path.wstring() + L"\"";
    std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back(L'\0');

    std::wstring dir = work_dir.wstring();

    BOOL ok = CreateProcessW(
        exe_path.wstring().c_str(),
        cmd_buf.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS,
        NULL,
        dir.empty() ? NULL : dir.c_str(),
        &si,
        &pi
    );

    if (ok) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}
#endif

/// Recursively copies all entries from src_dir to dst_dir, renaming locked files to .old if needed.
void copy_and_replace_tree(const fs::path& src_dir, const fs::path& dst_dir) {
    std::error_code ec;
    fs::create_directories(dst_dir, ec);

    for (const auto& entry : fs::recursive_directory_iterator(src_dir, ec)) {
        if (ec) break;
        const fs::path rel = fs::relative(entry.path(), src_dir, ec);
        if (ec) continue;

        const fs::path target = dst_dir / rel;

        if (entry.is_directory()) {
            fs::create_directories(target, ec);
            continue;
        }

        if (entry.is_regular_file()) {
            fs::create_directories(target.parent_path(), ec);

            // Attempt direct overwrite copy
            fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing, ec);
            if (ec) {
                // If failed (likely file locked or busy), try renaming existing file to .old
                std::error_code rename_ec;
                fs::path old_path = target;
                old_path += ".old";
                fs::remove(old_path, rename_ec);
                fs::rename(target, old_path, rename_ec);

                // Now retry copy
                ec.clear();
                fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing, ec);
            }
        }
    }
}

/// Locates the real content directory inside the unpacked folder.
/// If all extracted items are wrapped in a single top-level directory (e.g. SekiroModManager-v0.3.1/),
/// returns that inner directory; otherwise returns unpack_dir.
fs::path resolve_content_dir(const fs::path& unpack_dir) {
    std::error_code ec;
    std::vector<fs::path> items;
    for (const auto& it : fs::directory_iterator(unpack_dir, ec)) {
        if (!ec) items.push_back(it.path());
    }

    if (items.size() == 1 && fs::is_directory(items.front(), ec)) {
        return items.front();
    }
    return unpack_dir;
}

} // namespace

nlohmann::json run_self_update(CliContext& ctx) {
    const std::string zip_arg = ctx.option("--zip");
    if (zip_arg.empty()) {
        fail(ErrorCode::InvalidArgument, "Missing required option '--zip <path-to-update.zip>'");
    }

    fs::path zip_path = fs::absolute(fs::u8path(zip_arg));
    if (!fs::exists(zip_path)) {
        fail(ErrorCode::Io, "Update package does not exist: " + zip_path.string(), zip_path);
    }

    // Target directory: where the application is installed
    fs::path target_dir;
    if (ctx.has_option("--target-dir")) {
        target_dir = fs::absolute(fs::u8path(ctx.option("--target-dir")));
    } else {
        // Default to the directory of this executable
        #ifdef _WIN32
        wchar_t exe_buf[MAX_PATH];
        GetModuleFileNameW(NULL, exe_buf, MAX_PATH);
        target_dir = fs::path(exe_buf).parent_path();
        #else
        target_dir = fs::current_path();
        #endif
    }

    // Wait for calling GUI process to cleanly terminate if PID was given
    if (ctx.has_option("--wait-pid")) {
        try {
            unsigned long pid = std::stoul(ctx.option("--wait-pid"));
            #ifdef _WIN32
            wait_for_process_exit(static_cast<DWORD>(pid), 30000);
            #endif
        } catch (...) {}
    }

    // Temp unpack directory
    const fs::path temp_unpack = fs::temp_directory_path() / (".smm_update_stage_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
    std::error_code ec;
    fs::create_directories(temp_unpack, ec);

    struct Cleaner {
        fs::path p;
        ~Cleaner() {
            std::error_code cleanup_ec;
            fs::remove_all(p, cleanup_ec);
        }
    } cleaner{temp_unpack};

    // Extract zip
    smm::extract_zip(zip_path, temp_unpack);

    // Resolve wrapper folder if any
    const fs::path content_dir = resolve_content_dir(temp_unpack);

    // Copy new files over target directory
    copy_and_replace_tree(content_dir, target_dir);

    // If restart requested, launch GUI executable
    if (ctx.has("--restart")) {
        #ifdef _WIN32
        fs::path candidate = target_dir / "SekiroModManager.exe";
        if (!fs::exists(candidate)) {
            candidate = target_dir / "smm_gui.exe";
        }
        if (fs::exists(candidate)) {
            launch_process_detached(candidate, target_dir);
        }
        #endif
    }

    return {
        {"status", "ok"},
        {"updated", true},
        {"target_dir", target_dir.string()}
    };
}

} // namespace smm::cli
