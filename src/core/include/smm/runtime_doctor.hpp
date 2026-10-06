#pragma once

#include "doctor.hpp"

namespace smm {

/// Read-only observations. Kept separate from evaluation for deterministic tests.
struct RuntimeProcess {
    uint32_t pid{0};
    std::string role; // "launcher", "steam", "game"
    fs::path executable;
    std::optional<bool> prefer_system32;
    bool modules_readable{false};
    bool local_hook_loaded{false};
    std::string failure;
};

struct RuntimeSnapshot {
    bool supported{false};
    std::string enumeration_failure;
    std::vector<RuntimeProcess> processes;
};

RuntimeSnapshot inspect_launch_runtime(const fs::path& game_dir);
/// Returns errors for dangerous or unverifiable launch paths. Does not change policy.
HealthReport diagnose_launch_runtime(const RuntimeSnapshot& snapshot);

} // namespace smm
