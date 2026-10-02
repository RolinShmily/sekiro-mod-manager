#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "types.hpp"

namespace smm {

/// Canonical top-level asset directories in a Sekiro ModEngine installation.
inline constexpr std::string_view CANONICAL_DIRS[] = {
    "parts", "chr", "obj", "param", "sfx", "sound", "msg", "menu",
    "mtd", "event", "map", "action", "cutscene", "script",
};

/// UI subdirectories that live under `menu/`.
inline constexpr std::string_view UI_SUB_DIRS[] = {"hi", "font", "low"};

/// Audio subdirectories that are all folded into `sound/`.
inline constexpr std::string_view AUDIO_SUB_DIRS[] = {"sounds", "audio", "voice", "voices", "music"};

/// Loose files that belong in the game root rather than under `mods/`.
inline constexpr std::string_view CANONICAL_ROOT_FILES[] = {"dinput8.dll", "modengine.ini"};

/// A file-extension signature and the canonical directory it implies.
struct FileSignature {
    std::string_view suffix;
    std::string_view canonical_dir;
    AssetCategory category;
};

/// Ordered signature table; first match wins.
inline constexpr FileSignature FILE_SIGNATURES[] = {
    {".partsbnd.dcx", "parts", AssetCategory::Parts},
    {".chrbnd.dcx", "chr", AssetCategory::Chr},
    {".anibnd.dcx", "chr", AssetCategory::Chr},
    {".texbnd.dcx", "chr", AssetCategory::Chr},
    {".objbnd.dcx", "obj", AssetCategory::Obj},
    {".objbnd", "obj", AssetCategory::Obj},
    {".ffxbnd.dcx", "sfx", AssetCategory::Sfx},
    {".parambnd.dcx", "param/gameparam", AssetCategory::Param},
    {".fsb", "sound", AssetCategory::Sound},
    {".bank", "sound", AssetCategory::Sound},
    {".bnk", "sound", AssetCategory::Sound},
    {".fev", "sound", AssetCategory::Sound},
    {".mch", "sound", AssetCategory::Sound},
    {".mix", "sound", AssetCategory::Sound},
    {".rpc", "sound", AssetCategory::Sound},
    {".itl", "sound", AssetCategory::Sound},
    {".wem", "sound", AssetCategory::Sound},
    {".emevd.dcx", "event", AssetCategory::Event},
    {".tpf.dcx", "menu/hi", AssetCategory::Menu},
    {".menubnd.dcx", "menu", AssetCategory::Menu},
    {".gfx", "menu/font", AssetCategory::Font},
    {".swf", "menu/font", AssetCategory::Font},
    {".mtd", "mtd", AssetCategory::Mtd},
    {".mtdbnd.dcx", "mtd", AssetCategory::Mtd},
    {".msb.dcx", "map", AssetCategory::Map},
    {".msgbnd.dcx", "msg", AssetCategory::Msg},
    {".lua", "script", AssetCategory::Script},
    {".fxr", "sfx", AssetCategory::Sfx},
    {".hkx", "action", AssetCategory::Other},
    {".bk2", "cutscene", AssetCategory::Cutscene},
};

/// Outcome of inspecting a mod directory tree.
struct NormalizationResult {
    fs::path canonical_root;
    std::vector<AssetEntry> assets;      ///< Sorted by relative_path
    std::vector<fs::path> ignored_files; ///< Docs, screenshots, archives, metadata
};

/// Heuristic normaliser that turns arbitrarily nested mod packages into canonical paths.
class Normalizer {
public:
    /// Locates the canonical asset root inside `base_path` and collects every deployable asset.
    /// Throws SmmError(NormalizationError) when no Sekiro payload can be recognised,
    /// or SmmError(NoAssetsFound) when the payload contains no usable assets.
    static NormalizationResult normalize_directory(const fs::path& base_path);

    /// Finds the directory that actually holds the mod payload
    /// (e.g. `NestedMod/Sekiro/mods/` for a badly packaged archive).
    static fs::path find_canonical_root(const fs::path& base_path);

    /// Classifies a normalised relative path into an asset subsystem.
    static AssetCategory classify_asset(std::string_view rel_path);

    /// Applies the loose-file heuristics to a single normalised relative path.
    /// Exposed for testing and for the importer's dry runs.
    static std::string normalize_relative_path(std::string_view rel_path);

    /// True when a file is auxiliary (readme, preview, nested archive, Yabber workspace).
    static bool is_ignored_file(const fs::path& path, const fs::path& canonical_root);
};

} // namespace smm
