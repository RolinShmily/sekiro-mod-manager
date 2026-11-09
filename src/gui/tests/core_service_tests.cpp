// Headless tests for the GUI's core adapter (`CoreService`).
//
// These run without a window, without HuskarUI and without a display: the service only needs
// Qt Core plus smm_core. They pin the behaviour the QML layer depends on, which used to live
// in SmmClient and could not be tested at all.
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <iostream>
#include <stdexcept>

#include "backend/adapters/CoreMapping.hpp"
#include "backend/services/CoreService.hpp"

#include "smm/error.hpp"

using namespace smm::gui;

namespace {

void check(bool value, const char* name) {
    if (!value) throw std::runtime_error(name);
    std::cout << "PASS " << name << '\n';
}

void writeFile(const QString& path, const QByteArray& body) {
    QFileInfo info(path);
    QDir().mkpath(info.absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        throw std::runtime_error("fixture write failed");
    }
    file.write(body);
}

void makeMod(const QString& staging, const QString& id, int priority) {
    writeFile(staging + "/" + id + "/.smm_mod.json",
              QByteArray("{\"id\":\"") + id.toUtf8() + "\",\"name\":\"" + id.toUtf8() +
                  "\",\"version\":\"1.0\",\"author\":\"tests\",\"category\":\"weapon_skin\","
                  "\"enabled\":true,\"priority\":" + QByteArray::number(priority) + "}");
    // A unique payload path per mod, so the plan has one mapping each instead of a conflict.
    writeFile(staging + "/" + id + "/parts/" + id + ".partsbnd.dcx", "fixture payload");
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        const QString root = QDir::tempPath() + "/smm-core-service-" +
                             QString::number(QDateTime::currentSecsSinceEpoch());
        const QString staging = root + "/staging";
        const QString game = root + "/game";
        makeMod(staging, "alpha", 100);
        makeMod(staging, "beta", 200);

        // --- scan ------------------------------------------------------------
        const auto snapshot = CoreService::scan(staging);
        check(snapshot.view.scanned, "scan reports that staging was read");
        check(snapshot.view.mods.size() == 2, "scan projects every mod");
        check(snapshot.view.failures.isEmpty(), "clean staging has no failures");
        check(snapshot.view.plan.fileCount == 2, "plan comes from the same scan");
        check(snapshot.view.mods.first().id == "alpha", "mods are ordered by priority");

        // --- missing staging is not an error ---------------------------------
        const auto empty = CoreService::scan(root + "/does-not-exist");
        check(!empty.view.scanned && empty.view.mods.isEmpty(), "missing staging scans empty");

        // --- environment -----------------------------------------------------
        const auto environment = CoreService::environment(staging, game);
        check(environment.stagingExists, "environment reports the staging directory");
        check(!environment.gameDirExists, "environment reports a missing game directory");

        // --- detail ----------------------------------------------------------
        QVector<AssetEntry> assets;
        const auto detail = CoreService::modDetail(staging, "alpha", &assets);
        check(detail.id == "alpha" && assets.size() == 1, "mod detail carries metadata and assets");
        check(detail.previewImage.isEmpty(), "a mod without a preview has no preview path");

        // --- metadata edit + rename -----------------------------------------
        const QString renamed = CoreService::editModMetadata(staging, "alpha", "alpha/renamed",
                                                             "Alpha Prime", "tests", "1.1",
                                                             "weapon_skin", "desc", "https://x");
        check(renamed == "alpha-renamed", "metadata edit sanitises and applies the new id");
        const auto afterRename = CoreService::scan(staging);
        check(afterRename.view.mods.size() == 2, "rename keeps the mod count");
        bool found = false;
        for (const auto& mod : afterRename.view.mods) {
            if (mod.id == "alpha-renamed") {
                found = mod.name == "Alpha Prime" && mod.sourceUrl == "https://x" &&
                        mod.version == "1.1";
            }
        }
        check(found, "rename persists the edited fields");

        // --- priorities ------------------------------------------------------
        CoreService::reorderMods(staging, {"beta", "alpha-renamed"});
        const auto reordered = CoreService::scan(staging);
        check(reordered.view.mods.first().id == "beta", "reorder rewrites priorities in order");
        check(reordered.view.mods.first().priority == 10, "reorder starts at the first step");

        // --- presets ---------------------------------------------------------
        CoreService::savePreset(staging, "Loadout", "desc", "Loadout EN", "desc EN");
        const auto presets = CoreService::presets(staging);
        check(presets.size() == 1, "preset is saved and listed");
        check(presets.first().name == "Loadout" && presets.first().nameEn == "Loadout EN",
              "preset keeps both languages");
        const QStringList presetIds = CoreService::presetModIds(staging, presets.first().id);
        check(presetIds.size() == 2, "preset lists the enabled mods");
        check(CoreService::presetModIds(staging, "missing").isEmpty(),
              "unknown preset yields no ids");

        // --- deployment + telemetry -----------------------------------------
        QDir().mkpath(game);  // a deploy needs an existing game directory
        const auto deployed = CoreService::deploy(staging, game, {}, {});
        check(deployed.success && deployed.totalFiles == 2, "deploy links every planned file");
        const QStringList deployedAssets =
            QDir(game + "/mods/parts").entryList({QStringLiteral("*.partsbnd.dcx")}, QDir::Files);
        check(deployedAssets.size() == 2, "deploy materialises every asset");
        const auto telemetry = CoreService::deploymentTelemetry(game);
        check(telemetry.files == 2 && telemetry.bytes > 0, "telemetry reads the manifest");
        check(CoreService::deploymentTelemetry({}).files == 0, "empty game path has no telemetry");

        // --- cancellation is honoured ---------------------------------------
        bool cancelled = false;
        try {
            CoreService::deploy(staging, game, {}, [&cancelled]() { return true; });
        } catch (const smm::SmmError& error) {
            cancelled = error.code() == smm::ErrorCode::Cancelled;
        }
        check(cancelled, "cancellation surfaces as SmmError(Cancelled)");

        // --- restore ---------------------------------------------------------
        const auto restored = CoreService::restore(game, {}, {});
        check(restored.removedFiles == 2, "restore unlinks what was deployed");
        check(CoreService::deploymentTelemetry(game).files == 0, "restore clears the telemetry");

        // --- removal ---------------------------------------------------------
        check(CoreService::removeMods(staging, {"beta", "beta", "missing"}) == 1,
              "removal ignores duplicates and unknown ids");
        check(CoreService::scan(staging).view.mods.size() == 1, "removal is reflected by a scan");

        QDir(root).removeRecursively();
        std::cout << "all core service tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
