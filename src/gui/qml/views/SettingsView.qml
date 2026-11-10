pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import HuskarUI.Basic
// The page composes the shared header/card components; the relative import keeps the QML
// runnable straight from the source tree (`smm_gui --dev`), not only from the module build.
import "../components"

/// 全局配置页。
///
/// 每个设置项都是"标签在左、控件在右"的两列：说明文字拿到整行宽度可以自然换行，控件
/// 固定在同一列宽上，视线只需沿一条竖线扫下去，不必在每张卡片里重新找输入框的位置。
///
/// 竖向节奏分三层：页内分组间距 30 > 组内项间距 18 > 标签与说明 4。上一版把标签、说明、
/// 输入框、三个图标按钮全塞进一行，密度由"能塞下多少"决定；现在由这三层间距决定。
ColumnLayout {
    id: rootView
    objectName: "settingsView"
    spacing: 18

    // --- 待保存的编辑态（“保存”之前不影响后台设置） -------------------------
    property string pendingStagingDir: ""
    property string pendingGameDir: ""
    property string pendingLanguage: "zh-CN"
    property string pendingFontFamily: ""
    property string pendingTheme: "dark"
    property string fontSearchText: ""

    readonly property var installedFonts: smmBackend ? smmBackend.availableFonts : []
    readonly property var filteredFonts: {
        const query = fontSearchText.trim().toLowerCase();
        if (!query) return installedFonts;
        return installedFonts.filter(option => option.label.toLowerCase().includes(query)
                                             || option.value.toLowerCase().includes(query));
    }
    readonly property bool notoSansInstalled: installedFonts.some(option => option.value === "Noto Sans SC")
    readonly property bool selectedFontInstalled: installedFonts.some(option => option.value === pendingFontFamily)

    /// 是否有未保存的改动：保存按钮据此启用，避免无意义的写盘。
    readonly property bool dirty: smmBackend !== null && (
        pendingStagingDir !== smmBackend.stagingDir
        || pendingGameDir !== smmBackend.sekiroDir
        || pendingLanguage !== smmBackend.language
        || pendingFontFamily !== smmBackend.fontFamily
        || pendingTheme !== smmBackend.themeMode)

    readonly property int contentWidth: 900
    /// 右列控件宽度。所有设置项共用同一个值，右侧才会形成一条对齐的竖线。
    readonly property int controlWidth: 340
    /// 标签列与控件列之间的最小间距；标签说明越长，这里越不该被挤窄。
    readonly property int columnGap: 32

    function loadFromBackend() {
        if (!smmBackend) return;
        pendingStagingDir = smmBackend.stagingDir;
        pendingGameDir = smmBackend.sekiroDir;
        pendingLanguage = smmBackend.language;
        pendingFontFamily = smmBackend.fontFamily;
        pendingTheme = smmBackend.themeMode;
        fontSearchText = "";
    }

    function save() {
        if (!smmBackend) return;
        smmBackend.saveSettings(
            smmBackend.localPath(rootView.pendingStagingDir),
            smmBackend.localPath(rootView.pendingGameDir),
            rootView.pendingLanguage,
            rootView.pendingFontFamily,
            rootView.pendingTheme
        );
    }

    Component.onCompleted: loadFromBackend()
    // 每次进入页面都同步一次，防止在别处（如工作台）改了目录后这里显示旧值。
    onVisibleChanged: if (visible) loadFromBackend()

    // ---------------------------------- 页头 ----------------------------------
    PageHeader {
        iconSource: HusIcon.SettingOutlined
        title: qsTr("全局配置")
        subtitle: qsTr("指定只狼游戏目录与模组暂存区；二者位于同一 NTFS 卷时可启用零拷贝硬链接部署。")

        extra: Component {
            RowLayout {
                spacing: 10

                HusIconText {
                    Layout.alignment: Qt.AlignVCenter
                    visible: rootView.dirty
                    iconSource: HusIcon.ExclamationCircleOutlined
                    iconSize: 13
                    colorIcon: HusTheme.Primary.colorWarning
                }

                HusText {
                    Layout.alignment: Qt.AlignVCenter
                    visible: rootView.dirty
                    text: qsTr("有未保存的改动")
                    font.pixelSize: 12
                    color: HusTheme.Primary.colorWarning
                }

                HusIconButton {
                    text: qsTr("还原")
                    iconSource: HusIcon.UndoOutlined
                    iconSize: 14
                    enabled: rootView.dirty
                    onClicked: rootView.loadFromBackend()
                }

                HusIconButton {
                    text: qsTr("保存设置")
                    iconSource: HusIcon.SaveOutlined
                    iconSize: 14
                    type: HusButton.Type_Primary
                    enabled: rootView.dirty
                    onClicked: rootView.save()
                }
            }
        }
    }

    // ---------------------------------- 配置内容 ----------------------------------
    ScrollView {
        id: settingsScrollView
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.maximumWidth: rootView.contentWidth
        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
        contentWidth: width
        contentHeight: bodyColumn.implicitHeight
        clip: true

        ScrollBar.vertical: HusScrollBar {
            policy: ScrollBar.AsNeeded
        }

        ColumnLayout {
            id: bodyColumn
            width: parent.width - (settingsScrollView.ScrollBar.vertical.visible ? 12 : 4)
            spacing: 30

            FolderDialog {
                id: gameDirDialog
                title: qsTr("选择只狼游戏安装根目录")
                onAccepted: {
                    if (selectedFolder) {
                        rootView.pendingGameDir = smmBackend.localPath(selectedFolder);
                    }
                }
            }

            FolderDialog {
                id: stagingDirDialog
                title: qsTr("选择模组暂存区目录")
                onAccepted: {
                    if (selectedFolder) {
                        rootView.pendingStagingDir = smmBackend.localPath(selectedFolder);
                    }
                }
            }

            // ------------------------------ 游戏与暂存区 ------------------------------
            SectionCard {
                iconSource: HusIcon.FolderOpenOutlined
                title: qsTr("游戏与暂存区")
                description: qsTr("两个目录决定模组从哪里读、往哪里链接。")
                bodySpacing: 18

                // 游戏目录
                RowLayout {
                    Layout.fillWidth: true
                    spacing: rootView.columnGap

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        HusText {
                            text: qsTr("只狼游戏安装根目录")
                            font.pixelSize: 13
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusText {
                            Layout.fillWidth: true
                            text: qsTr("目录中应包含 sekiro.exe 与 dinput8.dll 注入钩子。")
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextTertiary
                            wrapMode: Text.WordWrap
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: rootView.controlWidth
                        Layout.alignment: Qt.AlignTop
                        spacing: 8

                        HusInput {
                            id: gameDirInput
                            Layout.fillWidth: true
                            implicitHeight: 36
                            Accessible.name: qsTr("只狼游戏安装根目录")
                            text: rootView.pendingGameDir
                            onTextChanged: {
                                if (text !== rootView.pendingGameDir) {
                                    rootView.pendingGameDir = text;
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            HusIconButton {
                                text: qsTr("浏览")
                                iconSource: HusIcon.FolderOpenOutlined
                                iconSize: 14
                                onClicked: gameDirDialog.open()
                            }

                            HusIconButton {
                                text: qsTr("打开")
                                iconSource: HusIcon.ExportOutlined
                                iconSize: 14
                                enabled: rootView.pendingGameDir.trim() !== ""
                                onClicked: {
                                    if (smmBackend) smmBackend.openFolder(rootView.pendingGameDir);
                                }
                            }

                            HusIconButton {
                                text: qsTr("自动探测")
                                iconSource: HusIcon.ExperimentOutlined
                                iconSize: 14
                                onClicked: {
                                    if (smmBackend) {
                                        const detected = smmBackend.detectSekiroDir();
                                        if (detected && detected !== "") {
                                            rootView.pendingGameDir = detected;
                                        } else {
                                            smmBackend.autoDetectGameDir();
                                        }
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }
                }

                // 暂存区
                RowLayout {
                    Layout.fillWidth: true
                    spacing: rootView.columnGap

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        HusText {
                            text: qsTr("模组暂存区目录")
                            font.pixelSize: 13
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusText {
                            Layout.fillWidth: true
                            text: qsTr("解压与归一化后的模组资产保存在此；与游戏目录同卷时才能硬链接。")
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextTertiary
                            wrapMode: Text.WordWrap
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: rootView.controlWidth
                        Layout.alignment: Qt.AlignTop
                        spacing: 8

                        HusInput {
                            id: stagingDirInput
                            Layout.fillWidth: true
                            implicitHeight: 36
                            Accessible.name: qsTr("模组暂存区目录")
                            text: rootView.pendingStagingDir
                            onTextChanged: {
                                if (text !== rootView.pendingStagingDir) {
                                    rootView.pendingStagingDir = text;
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            HusIconButton {
                                text: qsTr("浏览")
                                iconSource: HusIcon.FolderOpenOutlined
                                iconSize: 14
                                onClicked: stagingDirDialog.open()
                            }

                            HusIconButton {
                                text: qsTr("打开")
                                iconSource: HusIcon.ExportOutlined
                                iconSize: 14
                                enabled: rootView.pendingStagingDir.trim() !== ""
                                onClicked: {
                                    if (smmBackend) smmBackend.openFolder(rootView.pendingStagingDir);
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }
                }

                // 卷匹配状态：图标 + 颜色 + 文字三重提示，不只靠颜色。
                // 它描述的是"两个目录之间的关系"，所以横跨整行，不塞进右列。
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    HusIconText {
                        Layout.alignment: Qt.AlignVCenter
                        iconSource: smmBackend && smmBackend.isNtfsMatched
                                    ? HusIcon.CheckCircleOutlined
                                    : HusIcon.ExclamationCircleOutlined
                        iconSize: 14
                        colorIcon: smmBackend && smmBackend.isNtfsMatched
                                   ? HusTheme.Primary.colorSuccess
                                   : HusTheme.Primary.colorWarning
                    }

                    HusTag {
                        Layout.alignment: Qt.AlignVCenter
                        tagState: smmBackend && smmBackend.isNtfsMatched
                                  ? HusTag.State_Success
                                  : HusTag.State_Warning
                        text: smmBackend && smmBackend.isNtfsMatched
                              ? qsTr("同卷匹配")
                              : qsTr("未匹配")
                    }

                    HusText {
                        Layout.fillWidth: true
                        text: smmBackend && smmBackend.isNtfsMatched
                              ? qsTr("已启用 NTFS 零拷贝硬链接部署加速。")
                              : qsTr("驱动器卷不匹配，部署时将回退为普通文件复制。")
                        font.pixelSize: 12
                        color: HusTheme.Primary.colorTextTertiary
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // ------------------------------ 外观与语言 ------------------------------
            SectionCard {
                iconSource: HusIcon.BgColorsOutlined
                title: qsTr("外观与语言")
                description: qsTr("保存后立即生效，并记住到下次启动。")
                bodySpacing: 18

                // 界面主题
                RowLayout {
                    Layout.fillWidth: true
                    spacing: rootView.columnGap

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        HusText {
                            text: qsTr("界面主题")
                            font.pixelSize: 13
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusText {
                            Layout.fillWidth: true
                            text: qsTr("跟随系统会随 Windows 的深色 / 浅色设置切换。")
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextTertiary
                            wrapMode: Text.WordWrap
                        }
                    }

                    HusSegmented {
                        id: themeSegmented
                        Layout.preferredWidth: rootView.controlWidth
                        Layout.alignment: Qt.AlignTop
                        block: true
                        // 就地更新文案，避免翻译重入把当前选中项重置掉。
                        options: [
                            { label: "", value: "dark" },
                            { label: "", value: "light" },
                            { label: "", value: "system" }
                        ]
                        function updateLabels() {
                            setProperty(0, "label", qsTr("暗色"));
                            setProperty(1, "label", qsTr("亮色"));
                            setProperty(2, "label", qsTr("跟随系统"));
                        }
                        Component.onCompleted: updateLabels()
                        Connections {
                            target: smmBackend
                            function onLanguageChanged() { Qt.callLater(themeSegmented.updateLabels); }
                        }
                        currentIndex: {
                            if (rootView.pendingTheme === "light") return 1;
                            if (rootView.pendingTheme === "system") return 2;
                            return 0;
                        }
                        onCurrentIndexChanged: {
                            if (currentIndex === 1) rootView.pendingTheme = "light";
                            else if (currentIndex === 2) rootView.pendingTheme = "system";
                            else rootView.pendingTheme = "dark";
                        }
                    }
                }

                // 界面语言
                RowLayout {
                    Layout.fillWidth: true
                    spacing: rootView.columnGap

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        HusText {
                            text: qsTr("界面语言")
                            font.pixelSize: 13
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusText {
                            Layout.fillWidth: true
                            text: qsTr("语言名称保留原生写法，切换后菜单与提示立即跟随。")
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextTertiary
                            wrapMode: Text.WordWrap
                        }
                    }

                    HusSegmented {
                        id: languageSegmented
                        Layout.preferredWidth: rootView.controlWidth
                        Layout.alignment: Qt.AlignTop
                        block: true
                        // 语言名称必须写原生字面量（不可加 qsTr），否则翻译会互相叠印。
                        options: [
                            { label: "简体中文", value: "zh-CN" },
                            { label: "English", value: "en-US" }
                        ]
                        currentIndex: rootView.pendingLanguage === "en-US" ? 1 : 0
                        onCurrentIndexChanged: {
                            rootView.pendingLanguage = (currentIndex === 1 ? "en-US" : "zh-CN");
                        }
                    }
                }
            }

            // ------------------------------ 界面字体 ------------------------------
            SectionCard {
                iconSource: HusIcon.FontColorsOutlined
                title: qsTr("界面字体")
                description: qsTr("推荐 Noto Sans SC（思源黑体系列），适合中英文界面；未安装时使用系统字体回退。")
                bodySpacing: 18

                RowLayout {
                    Layout.fillWidth: true
                    spacing: rootView.columnGap

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        HusText {
                            text: qsTr("字体族")
                            font.pixelSize: 13
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusText {
                            Layout.fillWidth: true
                            text: qsTr("只影响界面文字，不改变游戏内字体。先搜索再选择，列表会随输入过滤。")
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextTertiary
                            wrapMode: Text.WordWrap
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: rootView.controlWidth
                        // HusSelect sizes itself to its longest model entry (here: a long font
                        // name), which would push this column ~50 px wider than every other row.
                        Layout.maximumWidth: rootView.controlWidth
                        Layout.alignment: Qt.AlignTop
                        spacing: 8

                        HusInput {
                            id: fontSearchInput
                            objectName: "fontSearchInput"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            placeholderText: qsTr("搜索字体名称，例如 Noto、思源、微软雅黑…")
                            text: rootView.fontSearchText
                            onTextChanged: rootView.fontSearchText = text
                            Accessible.name: qsTr("搜索已安装字体")
                            iconSource: HusIcon.SearchOutlined
                            iconSize: 14
                            iconPosition: HusInput.Position_Left
                            clearEnabled: "active"
                        }

                        HusSelect {
                            id: fontSelect
                            objectName: "fontSelect"
                            Layout.fillWidth: true
                            implicitHeight: 36
                            clearEnabled: false
                            Component.onCompleted: {
                                popup.colorShadow = Qt.binding(() => Qt.rgba(0, 0, 0, HusTheme.isDark ? 0.62 : 0.20));
                            }
                            enabled: rootView.filteredFonts.length > 0
                            textRole: "label"
                            valueRole: "value"
                            model: rootView.filteredFonts
                            displayText: rootView.pendingFontFamily
                            currentIndex: rootView.filteredFonts.findIndex(option => option.value === rootView.pendingFontFamily)
                            onActivated: (index) => {
                                const option = rootView.filteredFonts[index];
                                if (option) rootView.pendingFontFamily = option.value;
                            }
                            contentDescription: qsTr("选择界面字体")
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            HusIconButton {
                                text: qsTr("使用推荐字体")
                                iconSource: HusIcon.StarOutlined
                                iconSize: 14
                                enabled: rootView.notoSansInstalled
                                onClicked: {
                                    rootView.pendingFontFamily = "Noto Sans SC";
                                    rootView.fontSearchText = "";
                                }
                            }

                            HusText {
                                Layout.fillWidth: true
                                text: rootView.filteredFonts.length === 0
                                      ? qsTr("没有匹配的字体")
                                      : qsTr("匹配 %1 / %2").arg(rootView.filteredFonts.length).arg(rootView.installedFonts.length)
                                font.pixelSize: 12
                                color: rootView.filteredFonts.length === 0
                                       ? HusTheme.Primary.colorWarning
                                       : HusTheme.Primary.colorTextTertiary
                                horizontalAlignment: Text.AlignRight
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                // 字体预览：真实渲染所选字体。横跨整行，字号足够看出字形差异。
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: fontPreview.implicitHeight + 34
                    radius: HusTheme.Primary.radiusPrimary
                    color: HusTheme.Primary.colorFillQuaternary
                    border.width: 1
                    border.color: HusTheme.Primary.colorBorderSecondary

                    HusText {
                        id: fontPreview
                        anchors.fill: parent
                        anchors.margins: 17
                        text: qsTr("只狼：影逝二度 · 字体预览 Aa 0123456789")
                        font.family: rootView.pendingFontFamily
                        font.pixelSize: 16
                        color: HusTheme.Primary.colorTextPrimary
                        wrapMode: Text.WordWrap
                    }
                }

                HusText {
                    Layout.fillWidth: true
                    visible: !rootView.selectedFontInstalled
                    text: qsTr("当前字体未安装，显示时将回退到系统字体。")
                    font.pixelSize: 12
                    color: HusTheme.Primary.colorWarning
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
