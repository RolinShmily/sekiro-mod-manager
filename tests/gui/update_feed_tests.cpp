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
            R"({"version":"0.4.8","tag":"v0.4.8","setup_exe_url":"https://sekiro.srprolin.com/releases/download/v0.4.8/sekiro-mod-manager-v0.4.8-windows-x64-setup.exe","notes_zh":"中文说明","notes_en":"English notes","notes":"plain"})");
        check(manifest.has_value() && manifest->version == "0.4.8", "manifest version is read");
        check(manifest->setupExeUrl.has_value() &&
                  *manifest->setupExeUrl ==
                      "https://sekiro.srprolin.com/releases/download/v0.4.8/"
                      "sekiro-mod-manager-v0.4.8-windows-x64-setup.exe",
              "trusted mirror setup_exe_url is accepted");
        check(UpdateFeed::notesFor(*manifest, "zh_CN") == "中文说明", "localized zh notes win");
        check(UpdateFeed::notesFor(*manifest, "en_US") == "English notes", "localized en notes win");
        check(UpdateFeed::notesFor(*manifest, "fr_FR") == "English notes",
              "unknown locale falls back to en");

        // Untrusted URLs in setup_exe_url must be ignored cleanly.
        const auto untrusted = UpdateFeed::parse(
            R"({"version":"0.4.8","setup_exe_url":"https://evil.example/payload.exe"})");
        check(untrusted.has_value() && !untrusted->setupExeUrl.has_value(),
              "untrusted setup_exe_url is dropped");

        // UTF-8 BOM tolerance
        const auto withBom = UpdateFeed::parse("\xEF\xBB\xBF{\"version\":\"1.0.0\",\"notes\":\"plain\"}");
        check(withBom.has_value() && UpdateFeed::notesFor(*withBom, "zh_CN") == "plain",
              "UTF-8 BOM and neutral notes fallback work");

        std::string error;
        check(!UpdateFeed::parse("{ not json", &error).has_value() && !error.empty(),
              "malformed body reports an error");
        check(!UpdateFeed::parse(R"({"notes":"no version"})").has_value(),
              "manifest without a version is rejected");
        check(!UpdateFeed::parse(R"(["v1.0.0"])").has_value(), "non-object body is rejected");

        // --- version comparison ----------------------------------------------
        check(UpdateFeed::isNewer("0.4.9", "0.4.8"), "patch bump is newer");
        check(!UpdateFeed::isNewer("0.4.8", "0.4.8"), "same version is not newer");
        check(!UpdateFeed::isNewer("v0.4.7", "0.4.8"), "older version is not newer");
        check(UpdateFeed::isNewer("v1.0", "0.9.9"), "minor bump outranks patch");
        check(UpdateFeed::isNewer("0.5", "0.4.8"), "shorter version compares with implicit zeros");
        check(!UpdateFeed::isNewer("", "0.4.8"), "empty remote version is not newer");

        // --- links ------------------------------------------------------------
        const auto ghLinks = UpdateFeed::linksFor("0.4.8", false);
        check(ghLinks.releasePage ==
                  "https://github.com/RolinShmily/sekiro-mod-manager/releases/tag/v0.4.8",
              "release page uses the tagged version");
        check(ghLinks.setupDownload ==
                  "https://github.com/RolinShmily/sekiro-mod-manager/releases/download/v0.4.8/"
                  "sekiro-mod-manager-v0.4.8-windows-x64-setup.exe",
              "github setup download points at the tagged installer");

        const auto mirrorLinks = UpdateFeed::linksFor("v0.4.8", true);
        check(mirrorLinks.setupDownload ==
                  "https://sekiro.srprolin.com/releases/download/v0.4.8/"
                  "sekiro-mod-manager-v0.4.8-windows-x64-setup.exe",
              "mirror setup download points at site proxy");

        const auto fromManifest = UpdateFeed::linksFor(*manifest, false);
        check(fromManifest.setupDownload == *manifest->setupExeUrl,
              "manifest linksFor preserves validated setup_exe_url");

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
