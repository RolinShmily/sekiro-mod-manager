#include "commands.hpp"

#include <iostream>

#include "json_output.hpp"
#include "resolve.hpp"
#include "terminal.hpp"

#include "smm/error.hpp"
#include "smm/exporter.hpp"

namespace smm::cli {

nlohmann::json run_export(CliContext& ctx) {
    const fs::path staging = ensure_staging_dir(ctx);

    std::vector<std::string> mod_ids;
    mod_ids.reserve(ctx.positionals.size());
    for (const auto& positional : ctx.positionals) {
        mod_ids.push_back(positional);
    }

    ExportOptions options;
    options.include_source = !ctx.has("--no-source");

    const fs::path output = ctx.has_option("--output") ? fs::path(ctx.option("--output"))
                                                       : fs::path("exports");
    const bool explicit_pack = ctx.has_option("--name");

    const ExportProgressCallback on_progress =
        [&ctx](std::size_t done, std::size_t total, const std::string& current) {
            emit_progress(ctx, "export", done, total, current);
        };

    // A single mod with no pack name is the common "give me this mod as a file" case; anything
    // else is a curated collection and gets a manifest.
    fs::path produced;
    const bool as_pack = explicit_pack || mod_ids.size() > 1;

    if (!as_pack) {
        fs::path destination = output;
        if (const fs::path name = output.filename(); name.empty() || output.extension() != ".zip") {
            destination = output / (mod_ids.front() + ".zip");
        }
        produced = export_single_mod(staging, mod_ids.front(), destination, options, on_progress);
    } else {
        const std::string pack_name = ctx.has_option("--name") ? ctx.option("--name")
                                                               : std::string("modpack");
        produced = export_modpack(staging, mod_ids, output, pack_name,
                                  ctx.option("--description"), options, on_progress);
    }

    if (!ctx.json_mode) {
        std::cout << terminal::green("Exported", ctx.use_color) << " " << produced.string()
                  << "\n";
        if (!options.include_source) {
            std::cout << "  " << terminal::dim(
                                     "original downloads were not included (--no-source)",
                                     ctx.use_color)
                      << "\n";
        }
    }

    return nlohmann::json{
        {"output", produced.string()},
        {"as_pack", as_pack},
        {"mod_count", mod_ids.size()},
        {"include_source", options.include_source},
    };
}

} // namespace smm::cli
