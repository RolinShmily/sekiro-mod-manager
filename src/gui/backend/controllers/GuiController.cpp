#include "backend/controllers/GuiController.hpp"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <memory>
#include <utility>

#include "backend/adapters/CoreMapping.hpp"
#include "backend/providers/ModPreviewImageProvider.hpp"

#include "smm/doctor.hpp"
#include "smm/error.hpp"
#include "smm/executor.hpp"

#include <theme/hustheme.h>

namespace smm::gui {

namespace {

constexpr const char* kSettingsScope = "SekiroModManager";
constexpr const char* kSettingsApp = "SMM";

QString megabytes(quint64 bytes) {
    return QString::number(static_cast<double>(bytes) / (1024.0 * 1024.0), 'f', 1);
}

} // namespace

GuiController::GuiController(QObject* parent) : QObject(parent) {
    QSettings settings(kSettingsScope, kSettingsApp);
    stagingDir_ = settings.value(QStringLiteral("stagingDir")).toString();
    sekiroDir_ = settings.value(QStringLiteral("sekiroDir")).toString();
    language_ = settings.value(QStringLiteral("language"), QStringLiteral("zh-CN")).toString();
    fontFamily_ = settings.value(QStringLiteral("fontFamily"), defaultFontFamily()).toString().trimmed();
    if (fontFamily_.isEmpty()) {
        fontFamily_ = defaultFontFamily();
    }
    reducedMotion_ = settings.value(QStringLiteral("reducedMotion"), false).toBool();
    HusTheme::instance()->setAnimationEnabled(!reducedMotion_);

    themeMode_ = settings.value(QStringLiteral("themeMode"), QStringLiteral("dark")).toString();
    if (themeMode_.trimmed().isEmpty()) {
        themeMode_ = QStringLiteral("dark");
    }

    equippedPackId_ = settings.value(QStringLiteral("equippedPresetId")).toString();
    if (!equippedPackId_.isEmpty()) {
        presetListModel_.setEquippedId(equippedPackId_);
    }

    // Resolve the two roots once, before any background work reads them. Environment
    // resolution also creates the staging directory, so a first run has somewhere to write.
    const auto environment = CoreService::environment(stagingDir_, sekiroDir_);
    stagingDir_ = environment.stagingDir;
    sekiroDir_ = environment.gameDir;

    connect(qGuiApp, &QGuiApplication::fontDatabaseChanged, this, [this]() {
        availableFontsCache_.clear();
        emit availableFontsChanged();
    });

    applyFontFamily(fontFamily_);
    applyThemeMode(themeMode_);
    initConnections();

    publishPreviewStaging();
    checkVolumeMatch();
    updateEquippedModIds();
    refreshAll();

    // Process state is the only health input that changes while SMM sits idle, so it is
    // re-observed on a timer; a full staging scan is never repeated on a schedule.
    auto* runtimeTimer = new QTimer(this);
    runtimeTimer->setInterval(10000);
    connect(runtimeTimer, &QTimer::timeout, this, [this]() {
        if (!isBusy() && !launchPending()) {
            requestRefresh(RefreshScope::RuntimeOnly);
        }
    });
    runtimeTimer->start();

    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
            [this](Qt::ApplicationState state) {
                if (state == Qt::ApplicationActive && !isBusy()) {
                    requestRefresh(RefreshScope::RuntimeOnly);
                }
            });
}

void GuiController::initConnections() {
    // --- background runners ---------------------------------------------------
    connect(&tasks_, &TaskRunner::busyChanged, this, [this](bool busy, const QString& label) {
        statusText_ = busy ? label : QStringLiteral("Ready");
        emit busyChanged();
        emit statusTextChanged();
        emit doctorChanged();
    });
    connect(&tasks_, &TaskRunner::progress, this, [this](int done, int total, const QString&) {
        if (total <= 0) {
            return;
        }
        statusText_ = QStringLiteral("%1 %2/%3").arg(tasks_.currentLabel()).arg(done).arg(total);
        emit statusTextChanged();
    });
    connect(&scans_, &TaskRunner::busyChanged, this, [this](bool busy, const QString&) {
        if (busy) {
            return;
        }
        if (refreshQueued_) {
            const RefreshScope scope = queuedScope_;
            refreshQueued_ = false;
            requestRefresh(scope);
            return;
        }
        if (!queuedDetailId_.isEmpty()) {
            const QString pending = queuedDetailId_;
            queuedDetailId_.clear();
            openModDetail(pending);
        }
    });

    // --- model requests -------------------------------------------------------
    // Models only express intent. Persistence, recovery and refreshes happen here, so a
    // failed write rolls the optimistic view change back through the refresh below.
    connect(&modListModel_, &ModListModel::enabledToggleRequested, this,
            [this](const QString& modId, bool enabled) {
                if (!stagingWritable()) {
                    requestRefresh(RefreshScope::Scan);
                    return;
                }
                try {
                    CoreService::setModEnabled(stagingDir_, modId, enabled);
                } catch (const std::exception& error) {
                    reportFailure(tr("Update Failed"), error);
                }
                requestRefresh(RefreshScope::Scan);
            });

    connect(&modListModel_, &ModListModel::prioritySwapRequested, this,
            [this](const QString& modIdA, quint32 priorityA, const QString& modIdB,
                   quint32 priorityB) {
                if (!stagingWritable()) {
                    requestRefresh(RefreshScope::Scan);
                    return;
                }
                try {
                    CoreService::swapModPriorities(stagingDir_, modIdA, priorityA, modIdB,
                                                   priorityB);
                } catch (const std::exception& error) {
                    reportFailure(tr("Priority Swap Failed"), error);
                }
                requestRefresh(RefreshScope::Scan);
            });

    connect(&modListModel_, &ModListModel::reorderRequested, this,
            [this](const QStringList& orderedIds) {
                if (!stagingWritable()) {
                    requestRefresh(RefreshScope::Scan);
                    return;
                }
                try {
                    CoreService::reorderMods(stagingDir_, orderedIds);
                } catch (const std::exception& error) {
                    reportFailure(tr("Priority Update Failed"), error);
                }
                requestRefresh(RefreshScope::Scan);
            });

    connect(&assetListModel_, &AssetListModel::assetToggleRequested, this,
            [this](const QString& modId, const QString& relativePath, bool enabled) {
                if (!stagingWritable()) {
                    requestRefresh(RefreshScope::Scan);
                    return;
                }
                try {
                    CoreService::setAssetEnabled(stagingDir_, modId, relativePath, enabled);
                } catch (const std::exception& error) {
                    reportFailure(tr("Asset Update Failed"), error);
                }
                requestRefresh(RefreshScope::Scan);
            });

    connect(&updater_, &UpdateService::updateNotification, this, &GuiController::notification);
}

void GuiController::checkVolumeMatch() {
    const bool both = !stagingDir_.trimmed().isEmpty() && !sekiroDir_.trimmed().isEmpty();
    const bool matched =
        both && smm::is_same_volume(toStdPath(stagingDir_), toStdPath(sekiroDir_));
    if (matched == isNtfsMatched_) {
        return;
    }
    isNtfsMatched_ = matched;
    emit ntfsMatchedChanged();
}

// ---------------------------------------------------------------------------
// Refresh pipeline: one scan, one job, one GUI thread update
// ---------------------------------------------------------------------------

void GuiController::requestRefresh(RefreshScope scope) {
    if (refreshQueued_) {
        // Coalesce bursts of requests; a diagnosis request is stronger than a plain scan.
        if (scope == RefreshScope::Diagnose) {
            queuedScope_ = RefreshScope::Diagnose;
        }
    }

    if (scans_.isBusy()) {
        if (!refreshQueued_) {
            queuedScope_ = scope;
        }
        refreshQueued_ = true;
        return;
    }

    if (scope == RefreshScope::RuntimeOnly) {
        if (healthInfo_.overall.isEmpty()) {
            return; // nothing to annotate yet; the initial diagnose is still pending
        }
        auto result = std::make_shared<HealthInfo>(healthInfo_);
        TaskRunner::Completion completion;
        completion.success = [this, result]() {
            healthInfo_ = *result;
            emit doctorChanged();
        };
        completion.failure = [this](const QString& message) {
            reportFailure(tr("Runtime Check Failed"), message);
        };
        scans_.run(
            tr("Checking runtime..."),
            [base = healthInfo_, result](TaskContext& context) {
                context.report(0, 1, {});
                *result = CoreService::withRuntimeProbe(base);
            },
            completion);
        return;
    }

    const QString staging = stagingDir_;
    const QString game = sekiroDir_;
    const bool diagnose = scope == RefreshScope::Diagnose;
    diagnoseInFlight_ = diagnose;
    if (diagnose) {
        emit doctorChanged();
    }

    auto result = std::make_shared<RefreshResult>();
    TaskRunner::Completion completion;
    completion.success = [this, result, scope]() { applyRefresh(*result, scope); };
    completion.failure = [this, diagnose](const QString& message) {
        if (diagnose) {
            diagnoseInFlight_ = false;
            emit doctorChanged();
        }
        reportFailure(tr("Refresh Failed"), message);
    };

    const bool accepted = scans_.run(
        diagnose ? tr("Checking environment...") : tr("Refreshing mods..."),
        [staging, game, diagnose, result](TaskContext& context) {
            const auto snapshot = CoreService::scan(staging);
            result->snapshot = snapshot.view;
            result->presets = CoreService::presets(staging);
            if (diagnose) {
                result->health = CoreService::withRuntimeProbe(
                    CoreService::diagnose(game, staging, &snapshot));
                result->diagnosed = true;
            }
            const auto telemetry = CoreService::deploymentTelemetry(game);
            result->deployedFiles = telemetry.files;
            result->bytesSaved = telemetry.bytes;
            context.report(1, 1, {});
        },
        completion);

    if (!accepted) {
        refreshQueued_ = true;
        queuedScope_ = scope;
    }
}

void GuiController::applyRefresh(const RefreshResult& result, RefreshScope scope) {
    modListModel_.setMods(result.snapshot.mods);
    presetListModel_.setPresets(result.presets);
    presetListModel_.setEquippedId(equippedPackId_);
    if (!equippedPackId_.isEmpty()) {
        updateEquippedModIds();
    }
    modScanFailures_ = result.snapshot.failures;
    conflictCount_ = result.snapshot.conflicts.size();
    deployedFiles_ = result.deployedFiles;
    bytesSaved_ = result.bytesSaved;
    emit telemetryChanged();

    if (scope == RefreshScope::RuntimeOnly) {
        healthInfo_ = result.health;
        emit doctorChanged();
        return;
    }

    if (result.diagnosed) {
        healthInfo_ = result.health;
        diagnoseInFlight_ = false;
        emit doctorChanged();
        if (launchPending_) {
            completeLaunchCheck();
        }
        return;
    }
    emit doctorChanged();
}

void GuiController::refreshAll() {
    requestRefresh(RefreshScope::Diagnose);
}

void GuiController::refreshDoctor() {
    requestRefresh(RefreshScope::Diagnose);
}

void GuiController::scheduleDoctor() {
    QTimer::singleShot(0, this, [this]() {
        if (!isBusy()) {
            refreshDoctor();
        }
    });
}

// ---------------------------------------------------------------------------
// Health presentation
// ---------------------------------------------------------------------------

bool GuiController::canLaunch() const {
    return !isBusy() && !isDoctorChecking() && modScanFailures_.isEmpty() &&
           healthInfo_.overall == QLatin1String("healthy");
}

QString GuiController::healthSummary() const {
    if (isDoctorChecking()) return tr("正在检查环境、部署文件与启动链…");
    const auto items = healthItems();
    for (const auto& value : items) {
        const auto item = value.toMap();
        if (item.value(QStringLiteral("id")).toString().startsWith(QLatin1String("mod-load-failure-"))) {
            return item.value(QStringLiteral("detail")).toString();
        }
    }
    if (healthInfo_.overall.isEmpty()) return tr("尚未完成诊断，启动前将重新检测。");
    if (healthInfo_.overall == QLatin1String("healthy") && modScanFailures_.isEmpty()) {
        return tr("诊断通过，启动前仍会重新检查。");
    }
    for (const auto& value : items) {
        const auto item = value.toMap();
        if (item.value(QStringLiteral("status")) == QLatin1String("error") ||
            item.value(QStringLiteral("status")) == QLatin1String("warning")) {
            return item.value(QStringLiteral("detail")).toString();
        }
    }
    return tr("诊断异常，请处理问题后重新检测。");
}

QString GuiController::healthRemediation() const {
    if (isDoctorChecking()) return {};
    for (const auto& value : healthItems()) {
        const auto item = value.toMap();
        if (item.value(QStringLiteral("id")).toString().startsWith(QLatin1String("mod-load-failure-"))) {
            return item.value(QStringLiteral("remediation")).toString();
        }
    }
    for (const auto& value : healthItems()) {
        const auto item = value.toMap();
        if (item.value(QStringLiteral("status")) == QLatin1String("error") ||
            item.value(QStringLiteral("status")) == QLatin1String("warning")) {
            return item.value(QStringLiteral("remediation")).toString();
        }
    }
    return {};
}

bool GuiController::engineReady() const {
    if (isDoctorChecking() || healthInfo_.overall.isEmpty()) return false;
    bool hookPresent = false;
    for (const auto& item : healthInfo_.items) {
        if (item.id == QLatin1String("dinput8") && item.status == QLatin1String("ok")) {
            hookPresent = true;
        }
        if ((item.category == QLatin1String("modengine") ||
             item.category == QLatin1String("runtime")) &&
            (item.status == QLatin1String("error") || item.status == QLatin1String("warning"))) {
            return false;
        }
    }
    return hookPresent;
}

QVariantList GuiController::healthItems() const {
    QVariantList list;
    for (const auto& item : healthInfo_.items) {
        QVariantMap map;
        map[QStringLiteral("id")] = item.id;
        map[QStringLiteral("category")] = item.category;
        map[QStringLiteral("title")] = item.title;
        map[QStringLiteral("detail")] = item.detail;
        map[QStringLiteral("status")] = item.status;
        map[QStringLiteral("remediation")] = item.remediation;
        if (language_ != QLatin1String("en-US") && item.category == QLatin1String("runtime")) {
            const bool hook = item.id.endsWith(QLatin1String("-hook"));
            const QString process = item.id.startsWith(QLatin1String("runtime-launcher-"))
                                        ? QStringLiteral("SMM")
                                    : item.id.startsWith(QLatin1String("runtime-steam-"))
                                        ? QStringLiteral("Steam")
                                        : QStringLiteral("Sekiro");
            map[QStringLiteral("title")] =
                hook ? tr("运行中的 ModEngine 钩子") : tr("%1 DLL 加载策略").arg(process);
            if (item.detail.contains(QLatin1String("PreferSystem32 is enabled"))) {
                map[QStringLiteral("detail")] =
                    tr("%1 优先加载系统 DLL（PreferSystem32），可能绕过游戏目录中的 ModEngine，"
                       "导致所有模组失效。")
                        .arg(process);
                map[QStringLiteral("remediation")] =
                    tr("完全退出游戏和 Steam，从不带该策略的启动入口重新启动 Steam 与 SMM，再次检测；"
                       "无需关闭全局 Windows 防护。");
            } else if (hook && item.status == QLatin1String("error")) {
                map[QStringLiteral("detail")] =
                    item.detail.contains(QLatin1String("not loaded"))
                        ? tr("游戏未加载本地 ModEngine 钩子。文件已部署不代表模组已生效。")
                        : tr("无法核验游戏加载的 DLL，请先退出游戏并确认安装路径。") +
                              QStringLiteral(" ") + item.detail;
                map[QStringLiteral("remediation")] =
                    tr("完全退出游戏和 Steam，检查启动链后重新启动并再次检测。");
            } else if (item.status == QLatin1String("ok")) {
                map[QStringLiteral("detail")] =
                    hook ? tr("游戏已加载本地 ModEngine 钩子；这不代表每个模组都已验证。")
                         : tr("%1 未启用优先加载系统 DLL 策略。").arg(process);
            } else {
                map[QStringLiteral("detail")] =
                    tr("无法核验启动链，不能视为诊断通过。") + QStringLiteral(" ") + item.detail;
                map[QStringLiteral("remediation")] =
                    tr("等待启动器完成启动，确认可以访问目标进程后重新检测。");
            }
        } else if (language_ != QLatin1String("en-US") && item.id == QLatin1String("deployment")) {
            map[QStringLiteral("title")] = tr("部署文件一致性");
            map[QStringLiteral("detail")] =
                item.status == QLatin1String("ok")
                    ? tr("当前启用文件与部署结果一致，或没有需要部署的文件。")
                    : tr("部署文件缺失、改变或已过期。") + QStringLiteral(" ") + item.detail;
            map[QStringLiteral("remediation")] =
                item.status == QLatin1String("ok") ? QString{} : tr("先部署当前模组组合，再重新检测。");
        }
        list.append(map);
    }
    for (qsizetype i = 0; i < modScanFailures_.size(); ++i) {
        list.append(QVariantMap{
            {QStringLiteral("id"), QStringLiteral("mod-load-failure-%1").arg(i)},
            {QStringLiteral("category"), tr("模组")},
            {QStringLiteral("title"), tr("模组读取失败")},
            {QStringLiteral("detail"), modScanFailures_.at(i)},
            {QStringLiteral("remediation"),
             tr("请重新导入该模组，或修复其元数据和文件后重新检测。")},
            {QStringLiteral("status"), QStringLiteral("error")}});
    }
    return list;
}

// ---------------------------------------------------------------------------
// Paths and preferences
// ---------------------------------------------------------------------------

void GuiController::setSekiroDir(const QString& dir) {
    if (sekiroDir_ == dir) {
        return;
    }
    sekiroDir_ = dir;
    checkVolumeMatch();
    emit sekiroDirChanged();
    launchPending_ = false;
    deployLaunchPending_ = false;
    requestRefresh(RefreshScope::Diagnose);
}

void GuiController::setStagingDir(const QString& dir) {
    if (stagingDir_ == dir) {
        return;
    }
    stagingDir_ = dir;
    publishPreviewStaging();
    checkVolumeMatch();
    emit stagingDirChanged();
    launchPending_ = false;
    deployLaunchPending_ = false;
    updateEquippedModIds();
    requestRefresh(RefreshScope::Diagnose);
}

void GuiController::publishPreviewStaging() {
    QMutexLocker locker(&previewStagingMutex_);
    previewStagingMirror_ = stagingDir_;
}

QString GuiController::previewStagingDir() const {
    QMutexLocker locker(&previewStagingMutex_);
    return previewStagingMirror_;
}

QString GuiController::localPath(const QUrl& url) const {
    if (url.isLocalFile()) {
        return QDir::toNativeSeparators(url.toLocalFile());
    }
    const QString text = url.toString();
    if (text.isEmpty()) {
        return {};
    }
    if (text.startsWith(QLatin1String("file:///"))) {
        return QDir::toNativeSeparators(QUrl::fromPercentEncoding(text.mid(8).toUtf8()));
    }
    if (text.startsWith(QLatin1String("file://"))) {
        return QDir::toNativeSeparators(QUrl::fromPercentEncoding(text.mid(7).toUtf8()));
    }
    return QDir::toNativeSeparators(QUrl::fromPercentEncoding(text.toUtf8()));
}

QString GuiController::categoryLabel(const QString& category) const {
    if (category.isEmpty() || category == QLatin1String("all")) return tr("全部模组");
    const QString key = category.toLower();
    if (key == QLatin1String("weapon_skin") || key == QLatin1String("weapon") ||
        key == QLatin1String("parts")) {
        return tr("武器外观");
    }
    if (key == QLatin1String("character_skin") || key == QLatin1String("chr")) {
        return tr("人物外观");
    }
    if (key == QLatin1String("gameplay_overhaul") || key == QLatin1String("param")) {
        return tr("玩法重构");
    }
    if (key == QLatin1String("ui") || key == QLatin1String("menu")) return tr("界面增强");
    if (key == QLatin1String("animation")) return tr("动作招式");
    if (key == QLatin1String("audio") || key == QLatin1String("sound")) return tr("音效音乐");
    if (key == QLatin1String("vfx") || key == QLatin1String("sfx")) return tr("特效光影");
    if (key == QLatin1String("map")) return tr("地图场景");
    if (key == QLatin1String("script")) return tr("脚本扩展");
    if (key == QLatin1String("loader")) return tr("前置钩子");
    if (key == QLatin1String("general") || key == QLatin1String("other")) return tr("其它扩展");
    return category.toUpper();
}

void GuiController::setLanguage(const QString& code) {
    if (language_ == code) {
        return;
    }
    language_ = code;
    QSettings settings(kSettingsScope, kSettingsApp);
    settings.setValue(QStringLiteral("language"), language_);
    availableFontsCache_.clear();
    emit languageChanged(language_);
    emit availableFontsChanged();
    emit doctorChanged();
}

void GuiController::setFontFamily(const QString& family) {
    const QString cleanFamily = family.trimmed();
    if (cleanFamily.isEmpty() || fontFamily_ == cleanFamily) {
        return;
    }
    fontFamily_ = cleanFamily;
    applyFontFamily(fontFamily_);
    QSettings settings(kSettingsScope, kSettingsApp);
    settings.setValue(QStringLiteral("fontFamily"), fontFamily_);
    emit fontFamilyChanged(fontFamily_);
}

void GuiController::setThemeMode(const QString& mode) {
    if (themeMode_ == mode) {
        return;
    }
    themeMode_ = mode;
    applyThemeMode(themeMode_);
    QSettings settings(kSettingsScope, kSettingsApp);
    settings.setValue(QStringLiteral("themeMode"), themeMode_);
    emit themeModeChanged(themeMode_);
}

void GuiController::setReducedMotion(bool reduced) {
    if (reducedMotion_ == reduced) {
        return;
    }
    reducedMotion_ = reduced;
    HusTheme::instance()->setAnimationEnabled(!reducedMotion_);
    QSettings settings(kSettingsScope, kSettingsApp);
    settings.setValue(QStringLiteral("reducedMotion"), reducedMotion_);
    emit reducedMotionChanged();
}

QString GuiController::defaultFontFamily() {
    const QStringList installed = QFontDatabase::families();
    const QStringList preferred = {QStringLiteral("Noto Sans SC"), QStringLiteral("Source Han Sans SC"),
                                   QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Segoe UI")};
    for (const QString& family : preferred) {
        if (installed.contains(family, Qt::CaseInsensitive)) return family;
    }
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
}

void GuiController::applyFontFamily(const QString& family) {
    const QString targetFamily =
        family.trimmed().isEmpty() ? defaultFontFamily() : family.trimmed();
    QStringList fontFamilies = {targetFamily, QStringLiteral("Noto Sans SC"),
                                QStringLiteral("Source Han Sans SC"),
                                QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Segoe UI"),
                                QStringLiteral("Microsoft YaHei")};
    fontFamilies.removeDuplicates();
    QFont defaultFont = QGuiApplication::font();
    defaultFont.setStyleHint(QFont::SansSerif);
    defaultFont.setWeight(QFont::Normal);
    defaultFont.setFamilies(fontFamilies);
    QGuiApplication::setFont(defaultFont);

    // HuskarUI resolves this family list against the installed font database, so the user's
    // exact selection has to stay first (Segoe UI included).
    fontFamilies.append(QStringLiteral("sans-serif"));
    HusTheme::instance()->installThemePrimaryFontFamiliesBase(
        fontFamilies.join(QStringLiteral(", ")));
    HusTheme::instance()->installThemePrimaryFontSizeBase(14);
}

void GuiController::applyThemeMode(const QString& mode) {
    const QString value = mode.trimmed().toLower();
    if (value == QLatin1String("light")) {
        HusTheme::instance()->setDarkMode(HusTheme::DarkMode::Light);
    } else if (value == QLatin1String("system")) {
        HusTheme::instance()->setDarkMode(HusTheme::DarkMode::System);
    } else {
        HusTheme::instance()->setDarkMode(HusTheme::DarkMode::Dark);
    }
}

QVariantList GuiController::availableFonts() const {
    if (!availableFontsCache_.isEmpty()) return availableFontsCache_;

    QStringList installed = QFontDatabase::families();
    installed.removeDuplicates();
    const QStringList preferred = {QStringLiteral("Noto Sans SC"), QStringLiteral("Source Han Sans SC"),
                                   QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Segoe UI")};
    QStringList ordered;
    for (const QString& preferredFamily : preferred) {
        const auto it = std::find_if(installed.cbegin(), installed.cend(),
                                     [&preferredFamily](const QString& family) {
                                         return family.compare(preferredFamily, Qt::CaseInsensitive) == 0;
                                     });
        if (it != installed.cend()) ordered.append(*it);
    }
    for (const QString& family : installed) {
        if (!ordered.contains(family)) ordered.append(family);
    }

    const bool isEn = language_ == QLatin1String("en-US");
    for (const QString& family : ordered) {
        if (family.startsWith(QLatin1Char('@')) || QFontDatabase::isPrivateFamily(family) ||
            family.startsWith(QLatin1String("HuskarUI"), Qt::CaseInsensitive)) {
            continue;
        }
        const bool recommended =
            family.compare(QLatin1String("Noto Sans SC"), Qt::CaseInsensitive) == 0;
        QString label = family;
        if (recommended) {
            label += isEn ? QStringLiteral(" (Recommended)") : QStringLiteral("（思源黑体系列 · 推荐）");
        } else if (family == QLatin1String("Source Han Sans SC")) {
            label += isEn ? QStringLiteral(" (Source Han Sans)") : QStringLiteral("（思源黑体）");
        } else if (!isEn && family == QLatin1String("Microsoft YaHei UI")) {
            label += QStringLiteral("（微软雅黑 UI）");
        } else if (!isEn && family == QLatin1String("Microsoft YaHei")) {
            label += QStringLiteral("（微软雅黑）");
        }
        availableFontsCache_.append(QVariantMap{{QStringLiteral("label"), label},
                                               {QStringLiteral("value"), family}});
    }
    return availableFontsCache_;
}

QVariantList GuiController::availableThemes() const {
    const bool isEn = language_ == QLatin1String("en-US");
    QVariantList result;
    const std::pair<const char*, QString> themes[] = {
        {"dark", isEn ? QStringLiteral("Dark Theme (Default)") : QStringLiteral("暗色主题 (默认)")},
        {"light", isEn ? QStringLiteral("Light Theme") : QStringLiteral("亮色主题")},
        {"system", isEn ? QStringLiteral("Follow System") : QStringLiteral("跟随系统")}};
    for (const auto& theme : themes) {
        result.append(QVariantMap{{QStringLiteral("label"), theme.second},
                                  {QStringLiteral("value"), QString::fromLatin1(theme.first)}});
    }
    return result;
}

void GuiController::saveSettings(const QString& staging, const QString& game, const QString& lang,
                                 const QString& font, const QString& theme, bool reducedMotion) {
    const QString cleanStaging = staging.trimmed();
    const QString cleanGame = game.trimmed();
    const bool pathsChanged = cleanStaging != stagingDir_ || cleanGame != sekiroDir_;

    setReducedMotion(reducedMotion);
    setStagingDir(cleanStaging);
    setSekiroDir(cleanGame);
    if (!lang.trimmed().isEmpty()) setLanguage(lang.trimmed());
    if (!font.trimmed().isEmpty()) setFontFamily(font.trimmed());
    if (!theme.trimmed().isEmpty()) setThemeMode(theme.trimmed());

    if (pathsChanged) {
        CoreService::ensureStagingDir(cleanStaging);
    }

    QSettings settings(kSettingsScope, kSettingsApp);
    settings.setValue(QStringLiteral("stagingDir"), cleanStaging);
    settings.setValue(QStringLiteral("sekiroDir"), cleanGame);
    settings.sync();

    if (pathsChanged) {
        refreshAll();
    }
    notify(QStringLiteral("success"), tr("Preferences saved successfully."));
}

// ---------------------------------------------------------------------------
// Mod detail and mutations
// ---------------------------------------------------------------------------

void GuiController::openModDetail(const QString& modId) {
    if (modId.isEmpty()) {
        return;
    }
    if (scans_.isBusy()) {
        // The read runner is serialized; remember the last request instead of dropping it.
        queuedDetailId_ = modId;
        return;
    }
    const QString staging = stagingDir_;
    auto result = std::make_shared<std::pair<ModEntry, QVector<AssetEntry>>>();

    TaskRunner::Completion completion;
    completion.success = [this, result]() { applyModDetail(result->first, result->second); };
    completion.failure = [this](const QString& message) {
        reportFailure(tr("Mod Detail Failed"), message);
    };

    scans_.run(
        tr("Loading mod..."),
        [staging, modId, result](TaskContext& context) {
            context.report(0, 1, {});
            result->first = CoreService::modDetail(staging, modId, &result->second);
        },
        completion);
}

void GuiController::applyModDetail(const ModEntry& mod, const QVector<AssetEntry>& assets) {
    currentModId_ = mod.id;
    currentModName_ = mod.name;
    currentModAuthor_ = mod.author;
    currentModVersion_ = mod.version;
    currentModDesc_ = mod.description;
    currentModPreview_ = mod.previewImage;
    currentModCategory_ = mod.category;
    currentModSourceUrl_ = mod.sourceUrl;
    currentModHomepage_ = mod.homepage;
    assetListModel_.setAssets(mod.id, assets);
    emit currentModChanged();
    emit assetCountsChanged();
}

bool GuiController::stagingWritable() const {
    // A quick metadata write must not race the background job that owns the staging tree.
    return stagingDir_.trimmed().isEmpty() || !tasks_.isBusy();
}

void GuiController::notify(const QString& type, const QString& message) {
    emit notification(type, message);
}

void GuiController::reportFailure(const QString& title, const std::exception& error) {
    QString detail = QString::fromUtf8(error.what());
    if (const auto* smmError = dynamic_cast<const smm::SmmError*>(&error)) {
        switch (smmError->code()) {
            case smm::ErrorCode::Cancelled:
                detail = tr("操作已取消。");
                break;
            case smm::ErrorCode::ModAlreadyExists:
                detail = tr("该模组 ID 已被占用，请换一个名称。");
                break;
            case smm::ErrorCode::ModNotFound:
                detail = tr("找不到该模组，可能已被移动或删除。");
                break;
            default:
                break;
        }
    }
    notify(QStringLiteral("error"), QStringLiteral("%1: %2").arg(title, detail));
    scheduleDoctor();
}

void GuiController::reportFailure(const QString& title, const QString& message) {
    notify(QStringLiteral("error"), QStringLiteral("%1: %2").arg(title, message));
    scheduleDoctor();
}

void GuiController::setModsEnabled(const QStringList& modIds, bool enabled) {
    if (modIds.isEmpty()) return;
    try {
        const int updated = CoreService::setModsEnabled(stagingDir_, modIds, enabled);
        if (updated > 0) {
            notify(QStringLiteral("success"),
                   tr("Mod Updated") + QStringLiteral(": ") + tr("Updated %1 mods.").arg(updated));
        }
    } catch (const std::exception& error) {
        reportFailure(tr("Update Failed"), error);
    }
    requestRefresh(RefreshScope::Scan);
}

void GuiController::deleteMod(const QString& modId) {
    if (modId.isEmpty()) return;
    deleteMods({modId});
}

void GuiController::deleteMods(const QStringList& modIds) {
    if (modIds.isEmpty()) return;
    try {
        const int removed = CoreService::removeMods(stagingDir_, modIds);
        if (removed > 0) {
            notify(QStringLiteral("success"),
                   tr("Mod Deleted") + QStringLiteral(": ") +
                       tr("Deleted %1 mods successfully.").arg(removed));
            if (modIds.contains(currentModId_)) {
                currentModId_.clear();
                currentModName_.clear();
                currentModAuthor_.clear();
                currentModVersion_.clear();
                currentModDesc_.clear();
                currentModPreview_.clear();
                currentModCategory_.clear();
                currentModSourceUrl_.clear();
                currentModHomepage_.clear();
                assetListModel_.setAssets({}, {});
                emit currentModChanged();
                emit assetCountsChanged();
            }
        }
    } catch (const std::exception& error) {
        reportFailure(tr("Delete Failed"), error);
    }
    requestRefresh(RefreshScope::Scan);
}

void GuiController::setModPreview(const QString& modId, const QString& imagePath) {
    try {
        CoreService::setModPreview(stagingDir_, modId, imagePath);
        ModPreviewImageProvider::clearCache();
        ++previewRevision_;
        emit previewRevisionChanged();
        // The preview file lives inside the mod directory, so its row has to be reloaded.
        openModDetail(modId);
        requestRefresh(RefreshScope::Scan);
    } catch (const std::exception& error) {
        reportFailure(tr("Preview Failed"), error);
    }
}

void GuiController::setModSourceUrl(const QString& modId, const QString& url) {
    if (modId.isEmpty()) return;
    try {
        const ModEntry mod = CoreService::modDetail(stagingDir_, modId, {});
        CoreService::editModMetadata(stagingDir_, modId, modId, mod.name, mod.author, mod.version,
                                     mod.category, mod.description, url);
        currentModSourceUrl_ = url;
        emit currentModChanged();
        notify(QStringLiteral("success"), tr("Source URL updated."));
        requestRefresh(RefreshScope::Scan);
    } catch (const std::exception& error) {
        reportFailure(tr("Save Failed"), error);
    }
}

void GuiController::updateModMetadata(const QString& modId, const QString& newId,
                                      const QString& name, const QString& author,
                                      const QString& version, const QString& category,
                                      const QString& description, const QString& sourceUrl) {
    try {
        const QString finalId = CoreService::editModMetadata(
            stagingDir_, modId, newId, name, author, version, category, description, sourceUrl);
        notify(QStringLiteral("success"), tr("Metadata Saved") + QStringLiteral(": ") +
                                              tr("Mod details updated."));
        if (finalId != modId) {
            clearCurrentMod();
        }
        if (!finalId.isEmpty()) {
            openModDetail(finalId);
        }
        requestRefresh(RefreshScope::Scan);
    } catch (const std::exception& error) {
        reportFailure(tr("Save Failed"), error);
    }
}

void GuiController::clearCurrentMod() {
    currentModId_.clear();
    currentModName_.clear();
    currentModAuthor_.clear();
    currentModVersion_.clear();
    currentModDesc_.clear();
    currentModPreview_.clear();
    currentModCategory_.clear();
    currentModSourceUrl_.clear();
    currentModHomepage_.clear();
    assetListModel_.setAssets({}, {});
    emit currentModChanged();
    emit assetCountsChanged();
}

// ---------------------------------------------------------------------------
// Import / export
// ---------------------------------------------------------------------------

void GuiController::importUrls(const QList<QUrl>& urls) {
    QStringList paths;
    paths.reserve(urls.size());
    for (const auto& url : urls) {
        const QString path = localPath(url);
        if (!path.isEmpty()) {
            paths.append(path);
        }
    }
    importArchives(paths);
}

void GuiController::importArchive(const QString& archivePath) {
    importArchives({archivePath});
}

void GuiController::importArchives(const QStringList& archivePaths) {
    QStringList cleanPaths = archivePaths;
    cleanPaths.removeAll(QString{});
    if (cleanPaths.isEmpty() || isBusy()) {
        return;
    }

    const QString staging = stagingDir_;

    TaskRunner::Completion completion;
    completion.success = [this]() {
        notify(QStringLiteral("success"),
               tr("Import Complete") + QStringLiteral(": ") +
                   tr("All mods imported successfully."));
        requestRefresh(RefreshScope::Scan);
    };
    completion.cancelled = [this]() {
        notify(QStringLiteral("info"), tr("Import cancelled."));
        requestRefresh(RefreshScope::Scan);
    };
    completion.failure = [this](const QString& message) { reportFailure(tr("Import Failed"), message); };

    tasks_.run(
        tr("Importing mods..."),
        [staging, cleanPaths](TaskContext& context) {
            CoreService::importPaths(
                staging, cleanPaths, {}, {}, true,
                [&context](int done, int total, const QString& current) {
                    context.report(done, total, current);
                },
                [&context]() { return context.cancelled(); },
                [](const ImportSummary&) {});
        },
        completion);
}

void GuiController::importModPack(const QString& packPath) {
    if (packPath.isEmpty() || isBusy()) {
        return;
    }
    const QString staging = stagingDir_;
    auto summary = std::make_shared<ModPackImportSummary>();

    TaskRunner::Completion completion;
    completion.success = [this, summary]() {
        notify(QStringLiteral("success"),
               tr("Pack Imported") + QStringLiteral(": ") +
                   tr("Imported '%1' with %2 mods.").arg(summary->name).arg(summary->modCount));
        requestRefresh(RefreshScope::Diagnose);
    };
    completion.failure = [this](const QString& message) {
        reportFailure(tr("Pack Import Failed"), message);
    };

    tasks_.run(
        tr("Importing mod pack..."),
        [staging, packPath, summary](TaskContext& context) {
            *summary = CoreService::importModPack(staging, packPath,
                                                  [&context]() { return context.cancelled(); });
            context.report(1, 1, {});
        },
        completion);
}

void GuiController::exportSingleMod(const QString& modId, const QString& outputPath) {
    if (modId.isEmpty() || isBusy()) {
        return;
    }
    const QString staging = stagingDir_;
    auto exported = std::make_shared<fs::path>();

    TaskRunner::Completion completion;
    completion.success = [this, exported]() {
        notify(QStringLiteral("success"),
               tr("Mod Exported") + QStringLiteral(": ") +
                   tr("Saved to %1").arg(toQString(*exported)));
    };
    completion.failure = [this](const QString& message) { reportFailure(tr("Export Failed"), message); };

    tasks_.run(
        tr("Exporting mod..."),
        [staging, modId, outputPath, exported](TaskContext& context) {
            context.report(0, 1, {});
            *exported = CoreService::exportSingleMod(staging, modId, outputPath);
        },
        completion);
}

void GuiController::exportModPack(const QString& packId, const QString& outputPath) {
    if (isBusy()) {
        return;
    }

    QString packName = QStringLiteral("SekiroModPack");
    QString packDescription = QStringLiteral("Custom Modpack exported from SMM");
    QStringList ids;
    try {
        const auto presets = CoreService::presets(stagingDir_);
        for (const auto& preset : presets) {
            if (preset.id != packId) {
                continue;
            }
            packName = preset.name;
            if (!preset.description.isEmpty()) {
                packDescription = preset.description;
            }
            ids = CoreService::presetModIds(stagingDir_, packId);
            break;
        }
    } catch (const std::exception& error) {
        reportFailure(tr("Export Failed"), error);
        return;
    }
    if (ids.isEmpty()) {
        for (const auto& mod : modListModel_.mods()) {
            if (mod.enabled) ids.append(mod.id);
        }
    }

    const QString staging = stagingDir_;
    auto exported = std::make_shared<fs::path>();

    TaskRunner::Completion completion;
    completion.success = [this, exported]() {
        notify(QStringLiteral("success"),
               tr("Export Succeeded") + QStringLiteral(": ") +
                   tr("Saved pack to %1").arg(toQString(*exported)));
    };
    completion.cancelled = [this]() { notify(QStringLiteral("info"), tr("Export cancelled.")); };
    completion.failure = [this](const QString& message) { reportFailure(tr("Export Failed"), message); };

    tasks_.run(
        tr("Exporting mod pack..."),
        [staging, ids, outputPath, packName, packDescription, exported](TaskContext& context) {
            *exported = CoreService::exportModPack(
                staging, ids, outputPath, packName, packDescription,
                [&context](int done, int total, const QString& current) {
                    context.report(done, total, current);
                },
                [&context]() { return context.cancelled(); });
        },
        completion);
}

// ---------------------------------------------------------------------------
// Presets
// ---------------------------------------------------------------------------

void GuiController::saveModPack(const QString& nameZh, const QString& descZh,
                                const QString& nameEn, const QString& descEn) {
    if (isBusy()) {
        return;
    }
    const QString staging = stagingDir_;
    TaskRunner::Completion completion;
    completion.success = [this, nameZh]() {
        notify(QStringLiteral("success"),
               tr("Preset Saved") + QStringLiteral(": ") + tr("Preset '%1' created.").arg(nameZh));
        requestRefresh(RefreshScope::Scan);
    };
    completion.failure = [this](const QString& message) {
        reportFailure(tr("Preset Save Failed"), message);
    };

    tasks_.run(
        tr("Saving preset..."),
        [staging, nameZh, descZh, nameEn, descEn](TaskContext& context) {
            CoreService::savePreset(staging, nameZh, descZh, nameEn, descEn);
            context.report(1, 1, {});
        },
        completion);
}

void GuiController::applyModPack(const QString& packId) {
    if (isBusy() || packId.isEmpty()) {
        return;
    }

    // The equipped badge follows the click immediately; core is the authority on the mods.
    presetListModel_.setEquippedId(packId);
    equippedPackId_ = packId;
    updateEquippedModIds();
    emit equippedPackChanged();
    QSettings settings(kSettingsScope, kSettingsApp);
    settings.setValue(QStringLiteral("equippedPresetId"), packId);

    const QString staging = stagingDir_;
    auto appliedName = std::make_shared<QString>();
    TaskRunner::Completion completion;
    completion.success = [this, appliedName]() {
        notify(QStringLiteral("success"), tr("Preset Applied") + QStringLiteral(": ") +
                                              tr("Preset '%1' equipped.").arg(*appliedName));
        requestRefresh(RefreshScope::Diagnose);
    };
    completion.failure = [this](const QString& message) {
        reportFailure(tr("Preset Apply Failed"), message);
    };

    tasks_.run(
        tr("Applying preset..."),
        [staging, packId, appliedName](TaskContext& context) {
            *appliedName = CoreService::applyPreset(staging, packId);
            context.report(1, 1, {});
        },
        completion);
}

void GuiController::deactivateModPack() {
    presetListModel_.setEquippedId({});
    equippedPackId_.clear();
    updateEquippedModIds();
    emit equippedPackChanged();

    QSettings settings(kSettingsScope, kSettingsApp);
    settings.remove(QStringLiteral("equippedPresetId"));
    notify(QStringLiteral("info"), tr("整合包已取消应用，当前进入自由模组管理状态。"));
}

void GuiController::deleteModPack(const QString& packId) {
    if (packId.isEmpty()) return;
    try {
        CoreService::deletePreset(stagingDir_, packId);
        if (equippedPackId_ == packId) {
            deactivateModPack();
        }
        notify(QStringLiteral("success"),
               tr("Preset Deleted") + QStringLiteral(": ") + tr("Preset removed."));
        requestRefresh(RefreshScope::Scan);
    } catch (const std::exception& error) {
        reportFailure(tr("Preset Delete Failed"), error);
    }
}

void GuiController::updateEquippedModIds() {
    QSet<QString> ids;
    if (!equippedPackId_.isEmpty()) {
        try {
            for (const auto& id : CoreService::presetModIds(stagingDir_, equippedPackId_)) {
                ids.insert(id);
            }
        } catch (const std::exception& error) {
            reportFailure(tr("Preset Scan Failed"), error);
        }
    }
    modListModel_.setEquippedModIds(ids);
}

// ---------------------------------------------------------------------------
// Deployment and launch
// ---------------------------------------------------------------------------

void GuiController::deploy() {
    if (isBusy() || launchPending()) return;
    const QString staging = stagingDir_;
    const QString game = sekiroDir_;

    auto summary = std::make_shared<DeploySummary>();
    TaskRunner::Completion completion;
    completion.success = [this, summary]() {
        ModPreviewImageProvider::clearCache();
        if (launchAfterDeploy_) {
            launchAfterDeploy_ = false;
            launchGame();
            return;
        }
        notify(QStringLiteral("success"),
               tr("Deployment succeeded: %1 files linked.")
                   .arg(summary->hardLinks + summary->copies));
        requestRefresh(RefreshScope::Diagnose);
    };
    completion.cancelled = [this]() {
        launchAfterDeploy_ = false;
        notify(QStringLiteral("info"), tr("Deployment cancelled; partially deployed files are "
                                          "removed by the next deploy or restore."));
        requestRefresh(RefreshScope::Diagnose);
    };
    completion.failure = [this](const QString& message) {
        launchAfterDeploy_ = false;
        if (deployLaunchPending_) {
            deployLaunchPending_ = false;
            emit doctorChanged();
            emit launchBlocked();
        }
        reportFailure(tr("Deploy Failed"), message);
    };

    tasks_.run(
        tr("Deploying mods..."),
        [staging, game, summary](TaskContext& context) {
            *summary = CoreService::deploy(
                staging, game,
                [&context](int done, int total, const QString& current) {
                    context.report(done, total, current);
                },
                [&context]() { return context.cancelled(); });
        },
        completion);
}

void GuiController::deployAndLaunch() {
    if (isBusy() || launchPending()) return;
    deployLaunchPending_ = true;
    launchPending_ = true;
    emit doctorChanged();
    // Deploying without a fresh health check could launch a game whose hook is broken.
    refreshDoctor();
}

void GuiController::restore() {
    if (isBusy() || launchPending()) return;
    const QString game = sekiroDir_;

    auto summary = std::make_shared<RestoreSummary>();
    TaskRunner::Completion completion;
    completion.success = [this, summary]() {
        notify(QStringLiteral("info"),
               tr("Restored: %1 links safely unlinked.").arg(summary->removedFiles));
        requestRefresh(RefreshScope::Diagnose);
    };
    completion.failure = [this](const QString& message) {
        reportFailure(tr("Restore Failed"), message);
    };

    tasks_.run(
        tr("Restoring clean game state..."),
        [game, summary](TaskContext& context) {
            *summary = CoreService::restore(
                game,
                [&context](int done, int total, const QString& current) {
                    context.report(done, total, current);
                },
                [&context]() { return context.cancelled(); });
        },
        completion);
}

void GuiController::launchGame() {
    if (isBusy() || launchPending()) return;
    launchPending_ = true;
    emit doctorChanged();
    // Always request a fresh complete check; a cached green status never authorises launch.
    refreshDoctor();
}

void GuiController::completeLaunchCheck() {
    launchPending_ = false;
    emit doctorChanged();

    const bool pathsMatch =
        QDir::cleanPath(QDir::fromNativeSeparators(healthInfo_.gameDir)) ==
            QDir::cleanPath(QDir::fromNativeSeparators(sekiroDir_)) &&
        QDir::cleanPath(QDir::fromNativeSeparators(healthInfo_.stagingDir)) ==
            QDir::cleanPath(QDir::fromNativeSeparators(stagingDir_));
    if (!canLaunch() || !pathsMatch) {
        deployLaunchPending_ = false;
        emit doctorChanged();
        notify(QStringLiteral("error"), tr("启动已阻止：%1").arg(healthSummary()));
        emit launchBlocked();
        return;
    }
    for (const auto& item : healthInfo_.items) {
        if (item.id.startsWith(QLatin1String("runtime-game-"))) {
            deployLaunchPending_ = false;
            emit doctorChanged();
            notify(QStringLiteral("info"), tr("只狼已在运行，请先退出游戏再启动。"));
            emit launchBlocked();
            return;
        }
    }
    if (deployLaunchPending_) {
        // Deploy first, then launch from the deploy completion once the files are in place.
        deployLaunchPending_ = false;
        launchAfterDeploy_ = true;
        deploy();
        return;
    }
    const QString executable = QDir(sekiroDir_).filePath(QStringLiteral("sekiro.exe"));
    if (!QFileInfo(executable).isFile() || !startGameProcess(executable, sekiroDir_)) {
        notify(QStringLiteral("error"), tr("启动失败，请检查游戏目录与访问权限。"));
        emit launchBlocked();
        return;
    }
    notify(QStringLiteral("success"), tr("启动前诊断通过，已启动只狼。"));
    // Steam may relaunch the executable. Re-observe the actual process instead of assuming
    // a successful CreateProcess means that ModEngine loaded.
    for (const int delay : {3000, 10000}) {
        QTimer::singleShot(delay, this, [this]() { requestRefresh(RefreshScope::RuntimeOnly); });
    }
}

bool GuiController::startGameProcess(const QString& executable, const QString& workingDirectory) {
    return QProcess::startDetached(executable, QStringList{}, workingDirectory);
}

void GuiController::setupEngine() {
    try {
        const auto setup = CoreService::setupEngine(sekiroDir_, stagingDir_);
        notify(QStringLiteral("success"), tr("ModEngine Setup") + QStringLiteral(": ") +
                                              tr("ModEngine hook and ini successfully configured."));
        (void)setup;
        requestRefresh(RefreshScope::Diagnose);
    } catch (const std::exception& error) {
        reportFailure(tr("ModEngine Setup Failed"), error);
    }
}

QString GuiController::detectSekiroDir() const {
    return toQString(smm::detect_game_dir());
}

void GuiController::autoDetectGameDir() {
    const QString detected = detectSekiroDir();
    if (!detected.isEmpty()) {
        setSekiroDir(detected);
        notify(QStringLiteral("success"),
               tr("Auto-detected Sekiro installation: %1").arg(detected));
        return;
    }
    notify(QStringLiteral("warning"),
           tr("Could not auto-detect Sekiro in default Steam libraries. Please select manually."));
}

// ---------------------------------------------------------------------------
// Maintenance and shell integration
// ---------------------------------------------------------------------------

void GuiController::optimizeAllPreviews() {
    if (stagingDir_.isEmpty() || isBusy()) return;
    const QString staging = stagingDir_;
    auto result = std::make_shared<CoreService::PreviewOptimization>();

    TaskRunner::Completion completion;
    completion.success = [this, result]() {
        ModPreviewImageProvider::clearCache();
        ++previewRevision_;
        emit previewRevisionChanged();
        notify(QStringLiteral("success"),
               tr("背景图压缩优化完成：共压缩 %1 个模组背景图，释放了 %2 MB 空间！")
                   .arg(result->optimized)
                   .arg(megabytes(result->bytesSaved)));
        requestRefresh(RefreshScope::Scan);
    };
    completion.failure = [this](const QString& message) {
        reportFailure(tr("Preview Optimization Failed"), message);
    };

    tasks_.run(
        tr("Optimizing previews..."),
        [staging, result](TaskContext& context) {
            context.report(0, 1, {});
            *result = CoreService::optimizePreviews(staging);
        },
        completion);
}

void GuiController::openFolder(const QString& path) {
    if (!path.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void GuiController::openModFolder(const QString& modId) {
    if (!stagingDir_.isEmpty() && !modId.isEmpty()) {
        openFolder(QDir(stagingDir_).filePath(modId));
    }
}

void GuiController::openGameFolder() {
    openFolder(sekiroDir_);
}

void GuiController::openStagingFolder() {
    openFolder(stagingDir_);
}

void GuiController::openUrl(const QString& url) {
    if (!url.trimmed().isEmpty()) {
        QDesktopServices::openUrl(QUrl(url.trimmed()));
    }
}

void GuiController::copyToClipboard(const QString& text) {
    if (text.isEmpty()) return;
    QGuiApplication::clipboard()->setText(text);
    notify(QStringLiteral("success"), tr("Copied to clipboard."));
}

} // namespace smm::gui
