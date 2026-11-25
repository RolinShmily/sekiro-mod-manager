#pragma once

#include <optional>
#include <string>

/// Pure update-feed logic: parsing the release manifest, comparing versions and building the
/// download links. Kept free of Qt so it can be unit tested without QNetworkAccessManager.
namespace smm::gui {

struct UpdateManifest {
    std::string version;
    std::optional<std::string> setupExeUrl;
    std::optional<std::string> guiZipUrl;
    std::optional<std::string> notesZh;
    std::optional<std::string> notesEn;
    std::optional<std::string> notes;
};

struct UpdateLinks {
    std::string releasePage;
    std::string setupDownload;
};

class UpdateFeed {
public:
    static constexpr const char* kMirrorOrigin = "https://sekiro.srprolin.com";
    static constexpr const char* kMirrorManifestUrl = "https://sekiro.srprolin.com/releases/latest.json";
    static constexpr const char* kGithubManifestUrl =
        "https://github.com/RolinShmily/sekiro-mod-manager/releases/latest/download/latest.json";

    /// Parses the `latest.json` body published by the release workflow / Worker proxy.
    /// Returns std::nullopt and fills `error` when the body is not a usable manifest.
    static std::optional<UpdateManifest> parse(const std::string& body, std::string* error = nullptr);

    /// True when `remote` is a strictly higher dotted version than `current`.
    /// Non-numeric segments count as zero, and a leading 'v' is ignored on both sides.
    static bool isNewer(const std::string& remote, const std::string& current);

    /// Release page and installer URL for a version (defaults to mirror proxy when preferMirror is true).
    static UpdateLinks linksFor(const std::string& version, bool preferMirror = false);

    /// Resolves links from a parsed manifest, honouring a validated `setup_exe_url` when present.
    static UpdateLinks linksFor(const UpdateManifest& manifest, bool preferMirror = true);

    /// The note body matching a locale tag, falling back to the locale-neutral field.
    static std::string notesFor(const UpdateManifest& manifest, const std::string& localeTag);
};

} // namespace smm::gui
