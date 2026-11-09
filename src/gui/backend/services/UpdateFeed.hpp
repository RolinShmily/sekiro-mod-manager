#pragma once

#include <optional>
#include <string>

/// Pure update-feed logic: parsing the release manifest, comparing versions and building the
/// download links.
///
/// Kept free of Qt on purpose. It is the part of the update flow that is business logic rather
/// than networking or UI state, so it can be unit tested without a QNetworkAccessManager, and a
/// future CLI `smm update --check` can reuse it unchanged.
namespace smm::gui {

struct UpdateManifest {
    std::string version;
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
    /// Parses the `latest.json` body published by the release workflow.
    /// Returns std::nullopt and fills `error` when the body is not a usable manifest.
    static std::optional<UpdateManifest> parse(const std::string& body, std::string* error = nullptr);

    /// True when `remote` is a strictly higher dotted version than `current`.
    /// Non-numeric segments count as zero, and a leading 'v' is ignored on both sides.
    static bool isNewer(const std::string& remote, const std::string& current);

    /// Release page and GUI installer URL for a version (with or without the leading 'v').
    static UpdateLinks linksFor(const std::string& version);

    /// The note body matching a locale tag, falling back to the locale-neutral field.
    static std::string notesFor(const UpdateManifest& manifest, const std::string& localeTag);
};

} // namespace smm::gui
