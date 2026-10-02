#include "smm/conflict.hpp"

#include <algorithm>
#include <map>
#include <string>

namespace smm {

ConflictSeverity ConflictEngine::severity_for_path(std::string_view rel_path) {
    if (is_critical_asset(rel_path)) {
        return ConflictSeverity::Critical;
    }
    if (is_slot_asset(rel_path)) {
        return ConflictSeverity::Warning;
    }
    return ConflictSeverity::Info;
}

ConflictReport ConflictEngine::scan_conflicts(const std::vector<StagedMod>& mods) {
    std::vector<const StagedMod*> enabled;
    enabled.reserve(mods.size());
    for (const auto& mod : mods) {
        if (mod.info.enabled) {
            enabled.push_back(&mod);
        }
    }

    // Priority ascending, ties broken by id: the winner of a path can therefore never depend
    // on directory iteration order, which keeps plans and reports byte-for-byte reproducible.
    std::sort(enabled.begin(), enabled.end(), [](const StagedMod* a, const StagedMod* b) {
        if (a->info.priority != b->info.priority) {
            return a->info.priority < b->info.priority;
        }
        return a->info.id < b->info.id;
    });

    struct Provider {
        const std::string* mod_id;
        const AssetEntry* asset;
    };
    std::map<std::string, std::vector<Provider>> by_path;
    for (const StagedMod* mod : enabled) {
        for (const auto& asset : mod->assets) {
            if (!asset.enabled) {
                continue; // Disabled asset does not participate in collision analysis
            }
            by_path[asset.relative_path].push_back(Provider{&mod->info.id, &asset});
        }
    }

    ConflictReport report;
    for (const auto& [path, providers] : by_path) {
        if (providers.size() <= 1) {
            continue;
        }

        const std::string& winner_id = *providers.front().mod_id;
        const AssetEntry& winner_asset = *providers.front().asset;

        std::vector<std::string> shadowed;
        shadowed.reserve(providers.size() - 1);
        for (std::size_t i = 1; i < providers.size(); ++i) {
            shadowed.push_back(*providers[i].mod_id);
        }

        // The asset's own classification can lag the path (a mod may ship gameparam under an
        // unexpected name), so both are consulted before settling on a severity.
        ConflictSeverity severity = ConflictSeverity::Info;
        if (winner_asset.is_critical || is_critical_asset(path)) {
            severity = ConflictSeverity::Critical;
        } else if (winner_asset.is_exclusive_slot || is_slot_asset(path)) {
            severity = ConflictSeverity::Warning;
        }

        std::string shadowed_list;
        for (std::size_t i = 0; i < shadowed.size(); ++i) {
            if (i) {
                shadowed_list += ", ";
            }
            shadowed_list += shadowed[i];
        }

        ConflictRecord record;
        record.relative_path = path;
        record.severity = severity;
        record.winner_mod_id = winner_id;
        record.shadowed_mod_ids = std::move(shadowed);
        switch (severity) {
            case ConflictSeverity::Critical:
                record.message = "Critical parameter collision on '" + path + "'. '" + winner_id +
                                 "' and [" + shadowed_list +
                                 "] both modify gameparam.parambnd.dcx; the parameters of the "
                                 "shadowed mods will be discarded entirely.";
                break;
            case ConflictSeverity::Warning:
                record.message = "Exclusive model or weapon slot collision on '" + path + "'. '" +
                                 winner_id + "' wins; the appearance from [" + shadowed_list +
                                 "] will be shadowed.";
                break;
            case ConflictSeverity::Info:
                record.message = "Asset override on '" + path + "'. '" + winner_id +
                                 "' supersedes [" + shadowed_list + "].";
                break;
        }
        report.records.push_back(std::move(record));
    }

    std::sort(report.records.begin(), report.records.end(),
              [](const ConflictRecord& a, const ConflictRecord& b) {
                  if (a.severity != b.severity) {
                      return static_cast<int>(a.severity) > static_cast<int>(b.severity);
                  }
                  return a.relative_path < b.relative_path;
              });

    report.recompute_flags();
    return report;
}

} // namespace smm
