// Unit tests for the pure part of the update flow: manifest parsing, version comparison and
// link construction. Deliberately Qt-free, so it links against the core library only.
#include "backend/services/UpdateFeed.hpp"

#include <iostream>
#include <stdexcept>

using namespace smm::gui;

namespace {

void check(bool value, const char* name) {
    if (!value) throw std::runtime_error(name);
    std::cout << "PASS " << name << '\n';
}

} // namespace

int main() {
    try {
        // --- parsing ---------------------------------------------------------
        const auto manifest = UpdateFeed::parse(
            R"({"version":"v0.4.3","notes_zh":"中文说明","notes_en":"English notes","notes":"plain"})");
        check(manifest.has_value() && manifest->version == "v0.4.3", "manifest version is read");
        check(UpdateFeed::notesFor(*manifest, "zh_CN") == "中文说明", "localized zh notes win");
        check(UpdateFeed::notesFor(*manifest, "en_US") == "English notes", "localized en notes win");
        check(UpdateFeed::notesFor(*manifest, "fr_FR") == "English notes",
              "unknown locale falls back to en");

        const auto notesOnly = UpdateFeed::parse(R"({"version":"1.0.0","notes":"plain"})");
        check(notesOnly.has_value() && UpdateFeed::notesFor(*notesOnly, "zh_CN") == "plain",
              "missing localized notes fall back to the neutral field");

        std::string error;
        check(!UpdateFeed::parse("{ not json", &error).has_value() && !error.empty(),
              "malformed body reports an error");
        check(!UpdateFeed::parse(R"({"notes":"no version"})").has_value(),
              "manifest without a version is rejected");
        check(!UpdateFeed::parse(R"(["v1.0.0"])").has_value(), "non-object body is rejected");

        // --- version comparison ----------------------------------------------
        check(UpdateFeed::isNewer("0.4.3", "0.4.2"), "patch bump is newer");
        check(!UpdateFeed::isNewer("0.4.2", "0.4.2"), "same version is not newer");
        check(!UpdateFeed::isNewer("v0.4.1", "0.4.2"), "older version is not newer");
        check(UpdateFeed::isNewer("v1.0", "0.9.9"), "minor bump outranks patch");
        check(UpdateFeed::isNewer("0.5", "0.4.2"), "shorter version compares with implicit zeros");
        check(!UpdateFeed::isNewer("", "0.4.2"), "empty remote version is not newer");

        // --- links ------------------------------------------------------------
        const auto links = UpdateFeed::linksFor("0.4.2");
        check(links.releasePage ==
                  "https://github.com/RolinShmily/sekiro-mod-manager/releases/tag/v0.4.2",
              "release page uses the tagged version");
        check(links.setupDownload ==
                  "https://github.com/RolinShmily/sekiro-mod-manager/releases/download/v0.4.2/"
                  "sekiro-mod-manager-v0.4.2-windows-x64-setup.exe",
              "setup download points at the tagged installer");
        check(UpdateFeed::linksFor("v0.4.2").setupDownload == links.setupDownload,
              "links are stable with or without the leading v");
        check(UpdateFeed::linksFor("").releasePage ==
                  "https://github.com/RolinShmily/sekiro-mod-manager/releases",
              "empty version falls back to the releases index");

        std::cout << "all update feed tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
