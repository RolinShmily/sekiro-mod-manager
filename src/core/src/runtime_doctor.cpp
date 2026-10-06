#include "smm/runtime_doctor.hpp"

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace smm {

#ifdef _WIN32
namespace {
class Handle {
public:
    explicit Handle(HANDLE handle) : handle_(handle) {}
    ~Handle() { if (handle_ && handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    HANDLE get() const { return handle_; }
    bool valid() const { return handle_ && handle_ != INVALID_HANDLE_VALUE; }
private:
    HANDLE handle_;
};

std::string win_error(DWORD code) { return "Windows error " + std::to_string(code); }

fs::path process_path(HANDLE process) {
    wchar_t path[32768]{};
    DWORD size = static_cast<DWORD>(std::size(path));
    if (!QueryFullProcessImageNameW(process, 0, path, &size)) return {};
    return fs::path(std::wstring(path, size));
}

RuntimeProcess observe(HANDLE process, DWORD pid, std::string role, const fs::path& game_dir) {
    RuntimeProcess result;
    result.pid = pid;
    result.role = std::move(role);
    result.executable = process_path(process);
    PROCESS_MITIGATION_IMAGE_LOAD_POLICY policy{};
    if (GetProcessMitigationPolicy(process, ProcessImageLoadPolicy, &policy, sizeof(policy))) {
        result.prefer_system32 = policy.PreferSystem32Images != 0;
    } else {
        result.failure = win_error(GetLastError());
    }
    if (result.role == "game") {
        const Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
        if (!snapshot.valid()) {
            result.failure = win_error(GetLastError());
            return result;
        }
        MODULEENTRY32W module{};
        module.dwSize = sizeof(module);
        if (!Module32FirstW(snapshot.get(), &module)) {
            result.failure = win_error(GetLastError());
            return result;
        }
        do {
            if (_wcsicmp(module.szModule, L"dinput8.dll") == 0) {
                std::error_code ec;
                if (fs::equivalent(fs::path(module.szExePath), game_dir / "dinput8.dll", ec) && !ec)
                    result.local_hook_loaded = true;
            }
        } while (Module32NextW(snapshot.get(), &module));
        result.modules_readable = GetLastError() == ERROR_NO_MORE_FILES;
        if (!result.modules_readable) result.failure = win_error(GetLastError());
    }
    return result;
}
} // namespace
#endif

RuntimeSnapshot inspect_launch_runtime(const fs::path& game_dir) {
    RuntimeSnapshot result;
#ifdef _WIN32
    result.supported = true;
    result.processes.push_back(observe(GetCurrentProcess(), GetCurrentProcessId(), "launcher", game_dir));
    const Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snapshot.valid()) {
        result.enumeration_failure = win_error(GetLastError());
        return result;
    }
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (!Process32FirstW(snapshot.get(), &entry)) {
        result.enumeration_failure = win_error(GetLastError());
        return result;
    }
    do {
        const bool steam = _wcsicmp(entry.szExeFile, L"steam.exe") == 0;
        const bool game = _wcsicmp(entry.szExeFile, L"sekiro.exe") == 0;
        if (!steam && !game) continue;
        const Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID));
        if (!process.valid()) {
            const DWORD error = GetLastError();
            // A process that exited between enumeration and OpenProcess is no longer a launch path.
            if (error == ERROR_INVALID_PARAMETER) continue;
            RuntimeProcess unknown;
            unknown.pid = entry.th32ProcessID;
            unknown.role = steam ? "steam" : "game";
            unknown.failure = win_error(error);
            result.processes.push_back(std::move(unknown));
            continue;
        }
        DWORD exitCode = 0;
        if (GetExitCodeProcess(process.get(), &exitCode) && exitCode != STILL_ACTIVE) continue;
        auto observed = observe(process.get(), entry.th32ProcessID, steam ? "steam" : "game", game_dir);
        if (game && !observed.executable.empty()) {
            std::error_code ec;
            if (!fs::equivalent(observed.executable, game_dir / "sekiro.exe", ec) || ec) {
                // Another installation does not describe this game's launch chain.
                continue;
            }
        }
        result.processes.push_back(std::move(observed));
    } while (Process32NextW(snapshot.get(), &entry));
    if (GetLastError() != ERROR_NO_MORE_FILES) result.enumeration_failure = win_error(GetLastError());
#else
    (void)game_dir;
#endif
    return result;
}

HealthReport diagnose_launch_runtime(const RuntimeSnapshot& snapshot) {
    HealthReport report;
    if (!snapshot.supported) {
        report.items.push_back({"runtime-platform", "runtime", "Runtime verification",
            "ModEngine runtime verification requires Windows.", DiagnosticStatus::Error,
            "Run SMM on Windows to verify the launch chain."});
    }
    if (!snapshot.enumeration_failure.empty()) {
        report.items.push_back({"runtime-enumeration", "runtime", "Launch processes",
            "Cannot inspect launch processes: " + snapshot.enumeration_failure, DiagnosticStatus::Error,
            "Retry the check after launchers finish starting. Verify access to the game and Steam processes."});
    }
    bool launcherFound = false;
    for (const auto& process : snapshot.processes) {
        launcherFound |= process.role == "launcher";
        const std::string label = process.role == "launcher" ? "SMM" : process.role == "steam" ? "Steam" : "Sekiro";
        const std::string prefix = label + " (PID " + std::to_string(process.pid) + "): ";
        const std::string id = "runtime-" + process.role + "-" + std::to_string(process.pid);
        if (!process.prefer_system32.has_value()) {
            report.items.push_back({id + "-policy", "runtime", label + " DLL search policy",
                prefix + "The DLL search policy could not be verified. " + process.failure,
                DiagnosticStatus::Error, "Retry with access to this process; an unknown policy is not a passed check."});
        } else if (*process.prefer_system32) {
            report.items.push_back({id + "-policy", "runtime", label + " DLL search policy",
                prefix + "PreferSystem32 is enabled. The system DINPUT8.dll can be loaded before the game's ModEngine hook.",
                DiagnosticStatus::Error,
                "Fully exit the game and Steam, then restart Steam and SMM from a launcher without this policy. Recheck before launching; do not disable global Windows protection."});
        } else {
            report.items.push_back({id + "-policy", "runtime", label + " DLL search policy",
                prefix + "PreferSystem32 is disabled.", DiagnosticStatus::Ok, std::nullopt});
        }
        if (process.role == "game") {
            if (!process.modules_readable) {
                report.items.push_back({id + "-hook", "runtime", "Loaded ModEngine hook",
                    prefix + "Loaded modules could not be verified. " + process.failure, DiagnosticStatus::Error,
                    "Close the game, check that the selected installation is correct, then retry."});
            } else if (!process.local_hook_loaded) {
                report.items.push_back({id + "-hook", "runtime", "Loaded ModEngine hook",
                    prefix + "The game's local dinput8.dll is not loaded. Deployed files alone do not activate mods.",
                    DiagnosticStatus::Error, "Fully exit the game and Steam; verify the launch policy before restarting."});
            } else {
                report.items.push_back({id + "-hook", "runtime", "Loaded ModEngine hook",
                    prefix + "The game's local dinput8.dll is loaded. This does not verify every individual mod.",
                    DiagnosticStatus::Ok, std::nullopt});
            }
        }
    }
    if (snapshot.supported && !launcherFound) {
        report.items.push_back({"runtime-launcher-missing", "runtime", "SMM DLL search policy",
            "The current launcher's policy was not verified.", DiagnosticStatus::Error,
            "Run the diagnostic again before launching."});
    }
    report.recompute();
    return report;
}
} // namespace smm
