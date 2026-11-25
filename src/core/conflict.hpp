#pragma once

#include <vector>

#include "types.hpp"

namespace smm {

/// Collision analysis across the currently enabled mods.
class ConflictEngine {
public:
    /// Groups every enabled mod's assets by normalised target path.
    ///
    /// The winner of a path is the mod with the lowest priority number, ties broken by
    /// ascending mod id, which makes planning fully deterministic regardless of scan order.
    /// Records come back sorted critical -> warning -> info, then by path.
    static ConflictReport scan_conflicts(const std::vector<StagedMod>& mods);

    /// Severity a collision on `rel_path` would carry, independent of any plan.
    static ConflictSeverity severity_for_path(std::string_view rel_path);
};

} // namespace smm
