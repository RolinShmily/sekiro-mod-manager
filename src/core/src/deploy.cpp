#include "smm/deploy.hpp"

#include <map>

#include "smm/conflict.hpp"
#include "smm/loader.hpp"

namespace smm {

DeployPlan DeploymentPlanner::build_plan(std::string profile_name, const std::vector<StagedMod>& mods) {
    DeployPlan plan;
    plan.active_profile = std::move(profile_name);
    plan.timestamp = now_epoch_seconds();
    plan.conflict_report = ConflictEngine::scan_conflicts(mods);

    std::vector<const StagedMod*> enabled;
    enabled.reserve(mods.size());
    for (const auto& mod : mods) {
        if (mod.info.enabled) {
            enabled.push_back(&mod);
        }
    }
    std::sort(enabled.begin(), enabled.end(), [](const StagedMod* a, const StagedMod* b) {
        if (a->info.priority != b->info.priority) {
            return a->info.priority < b->info.priority;
        }
        return a->info.id < b->info.id;
    });

    // First mod to claim a path wins, because the list is already in precedence order.
    // std::map keeps the mapping list ordered by target path.
    struct Claim {
        const AssetEntry* asset;
        const std::string* owner_id;
        uint32_t owner_priority;
        std::vector<std::string> shadowed;
    };
    std::map<std::string, Claim> claims;

    for (const StagedMod* mod : enabled) {
        for (const auto& asset : mod->assets) {
            if (!asset.enabled) {
                continue; // Disabled asset is not deployed to the game
            }
            const auto it = claims.find(asset.relative_path);
            if (it == claims.end()) {
                claims.emplace(asset.relative_path,
                               Claim{&asset, &mod->info.id, mod->info.priority, {}});
            } else {
                it->second.shadowed.push_back(mod->info.id);
            }
        }
    }

    plan.mappings.reserve(claims.size());
    for (const auto& [path, claim] : claims) {
        DeployMapping mapping;
        mapping.target_relative_path = path;
        mapping.source_path = claim.asset->source_path;
        mapping.owner_mod_id = *claim.owner_id;
        mapping.priority = claim.owner_priority;
        mapping.shadowed_mods = claim.shadowed;
        plan.mappings.push_back(std::move(mapping));
    }

    return plan;
}

DeployPlan DeploymentPlanner::build_plan_from_staging(std::string profile_name, const fs::path& staging_dir) {
    const ScanOutcome outcome = ModLoader::scan_mods_directory(staging_dir);
    return build_plan(std::move(profile_name), outcome.mods);
}

} // namespace smm
