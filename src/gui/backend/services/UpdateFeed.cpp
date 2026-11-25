#include "backend/services/UpdateFeed.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

#include "core/types.hpp"

namespace smm::gui {

namespace {

constexpr const char* kReleasesBase = "https://github.com/RolinShmily/sekiro-mod-manager/releases";
constexpr const char* kMirrorDownloadBase = "https://sekiro.srprolin.com/releases/download";

std::string strippedTag(const std::string& version) {
    std::string tag = version;
    tag.erase(tag.begin(), std::find_if(tag.begin(), tag.end(), [](unsigned char c) {
                  return std::isspace(c) == 0;
              }));
    tag.erase(std::find_if(tag.rbegin(), tag.rend(), [](unsigned char c) {
                  return std::isspace(c) == 0;
              }).base(), tag.end());
    if (!tag.empty() && (tag.front() == 'v' || tag.front() == 'V')) {
        tag.erase(tag.begin());
    }
    return tag;
}

std::vector<int> versionParts(const std::string& version) {
    std::vector<int> parts;
    int value = 0;
    bool inNumber = false;
    for (const char c : strippedTag(version)) {
        if (c >= '0' && c <= '9') {
            value = value * 10 + (c - '0');
            inNumber = true;
            continue;
        }
        if (inNumber) {
            parts.push_back(value);
            value = 0;
            inNumber = false;
        }
    }
    if (inNumber) {
        parts.push_back(value);
    }
    return parts;
}

std::optional<std::string> stringField(const smm::json& object, const char* key) {
    const auto it = object.find(key);
    if (it == object.end() || !it->is_string()) {
        return std::nullopt;
    }
    return it->get<std::string>();
}

bool isTrustedDownloadUrl(const std::string& url, const std::string& tag, const std::string& filename) {
    if (tag.empty() || url.find('?') != std::string::npos || url.find('#') != std::string::npos) {
        return false;
    }
    const std::string expectedGithub =
        std::string(kReleasesBase) + "/download/" + tag + "/" + filename;
    const std::string expectedMirror =
        std::string(kMirrorDownloadBase) + "/" + tag + "/" + filename;
    return url == expectedGithub || url == expectedMirror;
}

} // namespace

std::optional<UpdateManifest> UpdateFeed::parse(const std::string& body, std::string* error) {
    const auto fail_with = [error](std::string message) -> std::optional<UpdateManifest> {
        if (error != nullptr) {
            *error = std::move(message);
        }
        return std::nullopt;
    };

    std::string cleaned = body;
    if (cleaned.size() >= 3 && static_cast<unsigned char>(cleaned[0]) == 0xEF &&
        static_cast<unsigned char>(cleaned[1]) == 0xBB &&
        static_cast<unsigned char>(cleaned[2]) == 0xBF) {
        cleaned.erase(0, 3);
    }

    smm::json document;
    try {
        document = smm::json::parse(cleaned);
    } catch (const smm::json::exception& exception) {
        return fail_with(std::string("Malformed update manifest: ") + exception.what());
    }
    if (!document.is_object()) {
        return fail_with("The update manifest is not a JSON object");
    }

    UpdateManifest manifest;
    const auto version = stringField(document, "version");
    const std::string bare = version.has_value() ? strippedTag(*version) : std::string{};
    if (bare.empty()) {
        return fail_with("The update manifest carries no usable version");
    }
    manifest.version = *version;

    const std::string tag = "v" + bare;
    const std::string setupFile = "sekiro-mod-manager-" + tag + "-windows-x64-setup.exe";
    const std::string guiFile = "sekiro-mod-manager-" + tag + "-windows-x64-gui.zip";

    if (const auto setupUrl = stringField(document, "setup_exe_url");
        setupUrl.has_value() && isTrustedDownloadUrl(*setupUrl, tag, setupFile)) {
        manifest.setupExeUrl = *setupUrl;
    }
    if (const auto guiUrl = stringField(document, "gui_zip_url");
        guiUrl.has_value() && isTrustedDownloadUrl(*guiUrl, tag, guiFile)) {
        manifest.guiZipUrl = *guiUrl;
    }

    manifest.notesZh = stringField(document, "notes_zh");
    manifest.notesEn = stringField(document, "notes_en");
    manifest.notes = stringField(document, "notes");
    return manifest;
}

bool UpdateFeed::isNewer(const std::string& remote, const std::string& current) {
    const auto remoteParts = versionParts(remote);
    const auto currentParts = versionParts(current);
    const std::size_t count = std::max(remoteParts.size(), currentParts.size());
    for (std::size_t i = 0; i < count; ++i) {
        const int remoteValue = i < remoteParts.size() ? remoteParts[i] : 0;
        const int currentValue = i < currentParts.size() ? currentParts[i] : 0;
        if (remoteValue != currentValue) {
            return remoteValue > currentValue;
        }
    }
    return false;
}

UpdateLinks UpdateFeed::linksFor(const std::string& version, bool preferMirror) {
    const std::string bare = strippedTag(version);
    const std::string tag = bare.empty() ? std::string{} : "v" + bare;
    UpdateLinks links;
    if (tag.empty()) {
        links.releasePage = kReleasesBase;
        return links;
    }
    const std::string filename = "sekiro-mod-manager-" + tag + "-windows-x64-setup.exe";
    links.releasePage = std::string(kReleasesBase) + "/tag/" + tag;
    links.setupDownload = preferMirror
                              ? std::string(kMirrorDownloadBase) + "/" + tag + "/" + filename
                              : std::string(kReleasesBase) + "/download/" + tag + "/" + filename;
    return links;
}

UpdateLinks UpdateFeed::linksFor(const UpdateManifest& manifest, bool preferMirror) {
    auto links = linksFor(manifest.version, preferMirror);
    if (manifest.setupExeUrl.has_value() && !manifest.setupExeUrl->empty()) {
        links.setupDownload = *manifest.setupExeUrl;
    }
    return links;
}

std::string UpdateFeed::notesFor(const UpdateManifest& manifest, const std::string& localeTag) {
    std::string locale = localeTag;
    std::transform(locale.begin(), locale.end(), locale.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (locale.rfind("zh", 0) == 0 && manifest.notesZh.has_value()) {
        return *manifest.notesZh;
    }
    if (manifest.notesEn.has_value()) {
        return *manifest.notesEn;
    }
    return manifest.notes.value_or(std::string{});
}

} // namespace smm::gui
