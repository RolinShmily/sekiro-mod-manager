#include "commands.hpp"

#include <iostream>

#include "json_output.hpp"
#include "resolve.hpp"
#include "terminal.hpp"

#include "smm/doctor.hpp"
#include "smm/error.hpp"
#include "smm/exporter.hpp"
#include "smm/importer.hpp"

namespace smm::cli {
namespace {

/// Turns the shared ImportOptions switches into what the core expects.
ImportOptions import_options_from(const CliContext& ctx) {
    ImportOptions options;
    if (ctx.has_option("--id")) {
        options.id = ctx.option("--id");
    }
    if (ctx.has_option("--name")) {
        options.name = ctx.option("--name");
    }
    if (ctx.has_option("--priority")) {
        options.priority = ctx.parse_u32("--priority", 100);
    }
    if (ctx.has_option("--source-url")) {
        options.source_url = ctx.option("--source-url");
    }
    options.overwrite = ctx.has("--overwrite");
    options.dry_run = ctx.has("--dry-run");
    return options;
}

/// A dry run must never look like a real import, so the summary wording changes with it.
void print_import(const CliContext& ctx, const ImportResult& result) {
    if (ctx.json_mode) {
        return;
    }

    const std::string verb = result.dry_run ? "Would import" : "Imported";
    std::cout << terminal::green(verb, ctx.use_color) << " " << result.info.name << " ("
              << result.info.id << ")\n";
    std::cout << "  version:  " << result.info.version << "\n"
              << "  category: " << result.info.category << "\n"
              << "  priority: " << result.info.priority << "\n"
              << "  assets:   " << result.asset_count << "\n"
              << "  size:     " << format_bytes(result.total_bytes) << "\n";
    if (result.ignored_count != 0) {
        std::cout << "  skipped:  " << result.ignored_count << " non-asset file(s)\n";
    }
    if (!result.dry_run) {
        std::cout << "  location: " << result.mod_dir.string() << "\n";
        if (result.replaced_existing) {
            std::cout << "  " << terminal::yellow("replaced an existing mod", ctx.use_color)
                      << "\n";
        }
    }
}

} // namespace

nlohmann::json run_import(CliContext& ctx) {
    const fs::path staging = ensure_staging_dir(ctx);
    const ImportOptions options = import_options_from(ctx);

    // One path with --id/--name applies those overrides; several paths are merged into a single
    // mod, which is what a multi-part download needs.
    ImportResult result;
    if (ctx.positionals.size() == 1) {
        result = import_mod(fs::path(ctx.positionals[0]), staging, options);
    } else {
        std::vector<fs::path> sources;
        sources.reserve(ctx.positionals.size());
        for (const auto& positional : ctx.positionals) {
            sources.emplace_back(positional);
        }
        result = import_multiple_files_as_mod(sources, staging, options);
    }

    print_import(ctx, result);
    return nlohmann::json{{"import", result}, {"staging_dir", staging.string()}};
}

nlohmann::json run_import_pack(CliContext& ctx) {
    const fs::path staging = ensure_staging_dir(ctx);
    const fs::path pack = fs::path(ctx.require_positional(0, "a modpack file"));
    const bool overwrite = ctx.has("--overwrite");

    ModPackImportResult result = import_modpack(pack, staging, overwrite);

    if (!ctx.json_mode) {
        std::cout << terminal::green("Imported modpack", ctx.use_color) << " "
                  << (result.manifest.name.empty() ? pack.filename().string()
                                                   : result.manifest.name)
                  << "\n";
        if (result.manifest.description && !result.manifest.description->empty()) {
            std::cout << "  " << *result.manifest.description << "\n";
        }
        std::cout << "  mods: " << result.mods.size() << "\n";
        for (const auto& mod : result.mods) {
            std::cout << "    - " << mod.info.name << " (" << mod.info.id
                      << ", priority " << mod.info.priority
                      << (mod.info.enabled ? ", enabled" : ", disabled") << ")\n";
        }
    }

    nlohmann::json mods = nlohmann::json::array();
    for (const auto& mod : result.mods) {
        mods.push_back(nlohmann::json{{"import", mod}});
    }

    return nlohmann::json{
        {"manifest", result.manifest},
        {"count", result.mods.size()},
        {"mods", mods},
        {"staging_dir", staging.string()},
    };
}

} // namespace smm::cli
