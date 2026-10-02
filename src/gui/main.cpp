#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QFileSystemWatcher>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

#ifdef BUILD_HUSKARUI_STATIC_LIBRARY
#include <QtQml/qqmlextensionplugin.h>
Q_IMPORT_QML_PLUGIN(HuskarUI_ImplPlugin)
Q_IMPORT_QML_PLUGIN(HuskarUI_BasicPlugin)
#endif

#include <QTimer>
#include <QTranslator>
#include <QFontDatabase>
#include "backend/Models.hpp"

#include <theme/hustheme.h>
#include <husapp.h>

int main(int argc, char* argv[]) {
    // 启用全域 Alpha 通道，支持磨砂亚克力玻璃质感
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/images/sekiro_icon_256.png")));
    app.setOrganizationName("SekiroModManager");
    app.setApplicationName("SMM");

    // 「苇名城」战术暗色调为默认启动基调（避免浅色主题下的白底不可读）

    bool devMode = false;
    // 默认「苇名城」战术暗色调；可用 --theme=dark|light|system 覆盖
    auto themeMode = HusTheme::DarkMode::Dark;
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--dev") {
            devMode = true;
        } else if (arg.startsWith("--theme=")) {
            const QString value = arg.mid(8).toLower();
            if (value == "light")
                themeMode = HusTheme::DarkMode::Light;
            else if (value == "system")
                themeMode = HusTheme::DarkMode::System;
            else
                themeMode = HusTheme::DarkMode::Dark;
        }
    }
#ifdef QT_DEBUG
    devMode = true;
#endif

    // 注册全局字体族（按角色严谨搭配）：
    // - Inter: 拉丁正文 / 界面排版 (Latin body/prose text)
    // - JetBrains Mono: 界面数据、版本号、路径与代码度量 (Latin/digits in code blocks and UI chrome)
    // - Instrument Serif: 品牌展示衬线体 (brand-only display serif)
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/JetBrainsMono.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/InstrumentSerif-Regular.ttf"));

    // 必须在 QML 引擎加载前完成主题安装，避免首帧闪烁与配色错乱
    HusTheme::instance()->setDarkMode(themeMode);
    // 水墨泥金：经典金碧水墨与和风素雅强调色（替代过艳的深红，呈现淡雅沉稳质感）
    HusTheme::instance()->installThemePrimaryColorBase(QColor(QStringLiteral("#c29f5d")));
    HusTheme::instance()->installThemePrimaryFontFamiliesBase(
        QStringLiteral("'Inter', 'Noto Sans SC', 'Segoe UI', 'Microsoft YaHei UI', sans-serif"));
    HusTheme::instance()->installThemePrimaryFontSizeBase(14);

    QQmlApplicationEngine engine;

    // 静态链接下插件初始化可能不触发，导致 HuskarUI-Icons 图标字体未注册
    HusApp::initialize(&engine);

    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.addImportPath(QStringLiteral(":/"));
#ifdef SMM_BUILD_DIR
    engine.addImportPath(QStringLiteral(SMM_BUILD_DIR "/huskarui/qml"));
#endif
#ifdef SMM_SOURCE_DIR
    engine.addImportPath(QStringLiteral(SMM_SOURCE_DIR "/qml"));
#endif

    smm::gui::GuiController controller;
    engine.rootContext()->setContextProperty("smmBackend", &controller);

    // 语言初始化与动态切换。采用 std::unique_ptr 智能指针管理，
    // 保证重复切换及程序退出时 100% RAII 自动安全回收，彻底杜绝内存泄漏。
    static std::unique_ptr<QTranslator> s_translator;
    const auto applyLanguage = [&app, &engine](const QString& code) {
        if (s_translator) {
            app.removeTranslator(s_translator.get());
            s_translator.reset();
        }
        if (code == QLatin1String("en-US")) {
            s_translator = std::make_unique<QTranslator>();
            if (s_translator->load(QStringLiteral(":/i18n/smm_en_US.qm"))) {
                app.installTranslator(s_translator.get());
            }
        }
        engine.retranslate();
    };

    // 首帧前加载初始化语言
    if (controller.language() == QLatin1String("en-US")) {
        applyLanguage(QStringLiteral("en-US"));
    }
    QObject::connect(&controller, &smm::gui::GuiController::languageChanged, &app, applyLanguage);

#ifdef SMM_SOURCE_DIR
    const QString sourceQmlDir = QStringLiteral(SMM_SOURCE_DIR "/qml");
#else
    const QString sourceQmlDir = QCoreApplication::applicationDirPath() + "/../../../src/gui/qml";
#endif

    QUrl mainQmlUrl;
    if (devMode && QDir(sourceQmlDir).exists()) {
        qInfo() << "[LiveReload] Dev mode active. Watching QML source directory:" << sourceQmlDir;
        mainQmlUrl = QUrl::fromLocalFile(sourceQmlDir + "/Main.qml");

        // 监听本地源码目录，实现 VS Code 保存即时热重载。
        // 编辑器保存往往连续触发多次目录变更，因此需要防抖；同时旧根对象必须先真正
        // 销毁再加载新场景，否则会出现双窗口以及 Qt 断言崩溃。
        auto* watcher = new QFileSystemWatcher(&app);
        QStringList watchList = { sourceQmlDir, sourceQmlDir + "/views", sourceQmlDir + "/components" };
        for (const auto& d : watchList) {
            if (QDir(d).exists()) watcher->addPath(d);
        }

        auto* reloadTimer = new QTimer(&app);
        reloadTimer->setSingleShot(true);
        reloadTimer->setInterval(300);

        QObject::connect(watcher, &QFileSystemWatcher::directoryChanged, &app, [reloadTimer](const QString& path) {
            qInfo() << "[LiveReload] Change detected in" << path << "-> scheduling reload";
            reloadTimer->start();
        });

        QObject::connect(reloadTimer, &QTimer::timeout, &app, [&engine, mainQmlUrl]() {
            qInfo() << "[LiveReload] Reloading QML scene...";
            engine.clearComponentCache();
            for (auto* obj : engine.rootObjects()) {
                obj->deleteLater();
            }
            // 延后到下一个事件循环再加载，确保旧根对象已完成销毁
            QTimer::singleShot(0, &engine, [&engine, mainQmlUrl]() {
                engine.load(mainQmlUrl);
            });
        });
    } else {
        mainQmlUrl = QUrl(QStringLiteral("qrc:/SmmGui/qml/Main.qml"));
    }

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(mainQmlUrl);

    return app.exec();
}
