#pragma once

#include <string>
#include <vector>

#include "types.hpp"

namespace smm {

/// Turns the enabled mods of a staging directory into an executable deployment plan.
class DeploymentPlanner {
public:
    /// Builds the plan for `profile_name`.
    ///
    /// Disabled mods are ignored. For every target path exactly one mapping survives:
    /// the owner is the enabled mod with the lowest priority number (ties broken by id),
    /// and every other mod contributing that path is listed in DeployMapping::shadowed_mods
    /// in the order it lost. Mappings are ordered by target path.
    static DeployPlan build_plan(std::string profile_name, const std::vector<StagedMod>& mods);

    /// Convenience overload that scans staging first.
    static DeployPlan build_plan_from_staging(std::string profile_name, const fs::path& staging_dir);
};

} // namespace smm
