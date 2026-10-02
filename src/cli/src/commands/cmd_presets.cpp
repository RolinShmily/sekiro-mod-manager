#include "commands.hpp"

#include <iostream>

#include "json_output.hpp"
#include "resolve.hpp"
#include "table.hpp"
#include "terminal.hpp"

#include "smm/error.hpp"
#include "smm/loader.hpp"
#include "smm/preset.hpp"

namespace smm::cli {
namespace {

nlohmann::json preset_json(const ModPreset& preset) {
    nlohmann::json mods = nlohmann::json::array();
    for (const auto& entry : preset.mods) {
        mods.push_back({{"mod_id", entry.mod_id}, {"priority", entry.priority}});
    }
    return nlohmann::json{
        {"id", preset.id},
        {"name", preset.name},
        {"description", preset.description ? nlohmann::json(*preset.description)
                                           : nlohmann::json(nullptr)},
        {"created_at", preset.created_at},
        {"updated_at", preset.updated_at},
        {"mod_count", preset.mods.size()},
        {"mods", mods},
    };
}

void print_presets(const CliContext& ctx, const std::vector<ModPreset>& presets) {
    if (presets.empty()) {
        std::cout << terminal::dim(
                         "No presets yet. Save the current setup with 'smm presets save <name>'.",
                         ctx.use_color)
                  << "\n";
        return;
    }

    std::vector<std::vector<std::string>> rows;
    rows.reserve(presets.size());
    for (const auto& preset : presets) {
        rows.push_back({preset.id, preset.name, std::to_string(preset.mods.size()),
                        preset.description.value_or("")});
    }
    std::cout << table::render(
        {{"ID"}, {"Name"}, {"Mods", table::Align::Right}, {"Description"}}, rows, ctx.use_color);
}

} // namespace

nlohmann::json run_presets(CliContext& ctx) {
    const fs::path staging = ensure_staging_dir(ctx);
    const std::string action = ctx.require_positional(0, "an action (list, save, apply, delete)");

    if (action == "list") {
        const std::vector<ModPreset> presets = PresetManager::list_presets(staging);
        if (!ctx.json_mode) {
            print_presets(ctx, presets);
        }
        nlohmann::json items = nlohmann::json::array();
        for (const auto& preset : presets) {
            items.push_back(preset_json(preset));
        }
        return nlohmann::json{{"action", "list"}, {"count", presets.size()}, {"presets", items}};
    }

    if (action == "save") {
        const std::string name = ctx.require_positional(1, "a preset name");
        std::optional<std::string> description;
        if (ctx.has_option("--description")) {
            description = ctx.option("--description");
        }

        const ModPreset preset = PresetManager::create_preset_from_current(staging, name, description);
        if (!ctx.json_mode) {
            std::cout << terminal::green("Saved preset", ctx.use_color) << " " << preset.name
                      << " (" << preset.id << ") with " << preset.mods.size() << " mod(s).\n";
        }
        return nlohmann::json{{"action", "save"}, {"preset", preset_json(preset)}};
    }

    if (action == "apply") {
        const std::string preset_id = ctx.require_positional(1, "a preset id");
        const ModPreset preset = PresetManager::apply_preset(staging, preset_id);
        if (!ctx.json_mode) {
            std::cout << terminal::green("Applied preset", ctx.use_color) << " " << preset.name
                      << ": " << preset.mods.size() << " mod(s) enabled, everything else "
                      << "disabled.\n";
        }
        return nlohmann::json{{"action", "apply"}, {"preset", preset_json(preset)}};
    }

    if (action == "delete") {
        const std::string preset_id = ctx.require_positional(1, "a preset id");
        PresetManager::delete_preset(staging, preset_id);
        if (!ctx.json_mode) {
            std::cout << terminal::green("Deleted preset", ctx.use_color) << " " << preset_id
                      << "\n";
        }
        return nlohmann::json{{"action", "delete"}, {"preset_id", preset_id}};
    }

    fail(ErrorCode::InvalidArgument,
         "Unknown preset action '" + action + "'. Use one of: list, save, apply, delete.");
}

} // namespace smm::cli
