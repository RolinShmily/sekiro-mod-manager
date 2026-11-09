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
/// 分页化之后，长表单如果只靠字号和分割线分组，用户很难判断"哪些选项属于同一件事"。
/// 这里改为：页头（图标 + 标题 + 操作）+ 四张分组卡片（游戏与暂存区 / 外观 / 字体 / 动效），
/// 每组自带图标、说明与独立表面，分组信息不再只由排版层级承担。
ColumnLayout {
    id: rootView
    objectName: "settingsView"
    spacing: 14

    // --- 待保存的编辑态（“保存”之前不影响后台设置） -------------------------
    property string pendingStagingDir: ""
    property string pendingGameDir: ""
    property string pendingLanguage: "zh-CN"
    property string pendingFontFamily: ""
    property string pendingTheme: "dark"
    property bool pendingReducedMotion: false
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
        || pendingTheme !== smmBackend.themeMode
        || pendingReducedMotion !== smmBackend.reducedMotion)

    readonly property int contentWidth: 900

    function loadFromBackend() {
        if (!smmBackend) return;
        pendingStagingDir = smmBackend.stagingDir;
        pendingGameDir = smmBackend.sekiroDir;
        pendingLanguage = smmBackend.language;
        pendingFontFamily = smmBackend.fontFamily;
        pendingTheme = smmBackend.themeMode;
        pendingReducedMotion = smmBackend.reducedMotion;
        fontSearchText = "";
    }

    function save() {
        if (!smmBackend) return;
        smmBackend.saveSettings(
            smmBackend.localPath(rootView.pendingStagingDir),
            smmBackend.localPath(rootView.pendingGameDir),
            rootView.pendingLanguage,
            rootView.pendingFontFamily,
            rootView.pendingTheme,
            rootView.pendingReducedMotion
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
            spacing: 14

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
                description: qsTr("两个目录决定模组从哪里读、往哪里链接。建议放在同一块 NTFS 磁盘上。")
                entranceIndex: 0

                // 游戏目录
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusIconText {
                            iconSource: HusIcon.DesktopOutlined
                            iconSize: 14
                            colorIcon: HusTheme.Primary.colorTextTertiary
                        }

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
                            elide: Text.ElideRight
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusInput {
                            id: gameDirInput
                            Layout.fillWidth: true
                            Accessible.name: qsTr("只狼游戏安装根目录")
                            text: rootView.pendingGameDir
                            onTextChanged: {
                                if (text !== rootView.pendingGameDir) {
                                    rootView.pendingGameDir = text;
                                }
                            }
                        }

                        HusIconButton {
                            iconSource: HusIcon.FolderOpenOutlined
                            iconSize: 16
                            onClicked: gameDirDialog.open()
                            contentDescription: qsTr("浏览游戏目录")
                            HusToolTip.visible: hovered
                            HusToolTip.text: qsTr("浏览游戏目录")
                        }

                        HusIconButton {
                            iconSource: HusIcon.ExportOutlined
                            iconSize: 16
                            enabled: rootView.pendingGameDir.trim() !== ""
                            onClicked: {
                                if (smmBackend) smmBackend.openFolder(rootView.pendingGameDir);
                            }
                            contentDescription: qsTr("在资源管理器中打开")
                            HusToolTip.visible: hovered
                            HusToolTip.text: qsTr("在资源管理器中打开")
                        }

                        HusIconButton {
                            iconSource: HusIcon.ExperimentOutlined
                            iconSize: 16
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
                            contentDescription: qsTr("自动探测安装位置")
                            HusToolTip.visible: hovered
                            HusToolTip.text: qsTr("自动探测安装位置")
                        }
                    }
                }

                // 暂存区
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusIconText {
                            iconSource: HusIcon.HddOutlined
                            iconSize: 14
                            colorIcon: HusTheme.Primary.colorTextTertiary
                        }

                        HusText {
                            text: qsTr("模组暂存区目录")
                            font.pixelSize: 13
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusText {
                            Layout.fillWidth: true
                            text: qsTr("解压与归一化后的模组资产保存在此。")
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextTertiary
                            elide: Text.ElideRight
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusInput {
                            id: stagingDirInput
                            Layout.fillWidth: true
                            Accessible.name: qsTr("模组暂存区目录")
                            text: rootView.pendingStagingDir
                            onTextChanged: {
                                if (text !== rootView.pendingStagingDir) {
                                    rootView.pendingStagingDir = text;
                                }
                            }
                        }

                        HusIconButton {
                            iconSource: HusIcon.FolderOpenOutlined
                            iconSize: 16
                            onClicked: stagingDirDialog.open()
                            contentDescription: qsTr("浏览暂存区目录")
                            HusToolTip.visible: hovered
                            HusToolTip.text: qsTr("浏览暂存区目录")
                        }

                        HusIconButton {
                            iconSource: HusIcon.ExportOutlined
                            iconSize: 16
                            enabled: rootView.pendingStagingDir.trim() !== ""
                            onClicked: {
                                if (smmBackend) smmBackend.openFolder(rootView.pendingStagingDir);
                            }
                            contentDescription: qsTr("在资源管理器中打开")
                            HusToolTip.visible: hovered
                            HusToolTip.text: qsTr("在资源管理器中打开")
                        }
                    }

                    // 卷匹配状态：图标 + 颜色 + 文字三重提示，不只靠颜色
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusIconText {
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
            }

            // ------------------------------ 外观与语言 ------------------------------
            SectionCard {
                iconSource: HusIcon.BgColorsOutlined
                title: qsTr("外观与语言")
                description: qsTr("主题与界面语言在保存后立即生效，并会记住到下次启动。")
                entranceIndex: 1

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusText {
                            text: qsTr("界面主题")
                            font.pixelSize: 13
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusSegmented {
                            id: themeSegmented
                            Layout.fillWidth: true
                            block: true
                            // 就地更新文案，避免翻译重入把当前选中项重置掉。
                            options: [
                                { label: "", value: "dark" },
                                { label: "", value: "light" },
                                { label: "", value: "system" }
                            ]
                            function updateLabels() {
                                setProperty(0, "label", qsTr("暗色主题"));
                                setProperty(1, "label", qsTr("亮色主题"));
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

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusText {
                            text: qsTr("界面语言")
                            font.pixelSize: 13
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusSegmented {
                            id: languageSegmented
                            Layout.fillWidth: true
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
            }

            // ------------------------------ 界面字体 ------------------------------
            SectionCard {
                iconSource: HusIcon.FontColorsOutlined
                title: qsTr("界面字体")
                description: qsTr("推荐 Noto Sans SC（思源黑体系列），适合中英文界面；未安装时使用系统字体回退。")
                entranceIndex: 2

                HusInput {
                    id: fontSearchInput
                    objectName: "fontSearchInput"
                    Layout.fillWidth: true
                    implicitHeight: 32
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
                    implicitHeight: 32
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
                              ? qsTr("没有匹配的已安装字体，请尝试其他名称。")
                              : qsTr("匹配 %1 / %2 个已安装字体").arg(rootView.filteredFonts.length).arg(rootView.installedFonts.length)
                        font.pixelSize: 12
                        color: HusTheme.Primary.colorTextTertiary
                        wrapMode: Text.WordWrap
                    }
                }

                // 字体预览：真实渲染所选字体
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: fontPreview.implicitHeight + 26
                    radius: HusTheme.Primary.radiusPrimary
                    color: HusTheme.Primary.colorFillQuaternary
                    border.width: 1
                    border.color: HusTheme.Primary.colorBorderSecondary

                    Behavior on color {
                        enabled: HusTheme.animationEnabled
                        ColorAnimation { duration: HusTheme.Primary.durationMid }
                    }

                    HusText {
                        id: fontPreview
                        anchors.fill: parent
                        anchors.margins: 13
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

            // ------------------------------ 动效 ------------------------------
            SectionCard {
                iconSource: HusIcon.ThunderboltOutlined
                title: qsTr("动效")
                description: qsTr("页面切换、卡片入场与悬停过渡都遵循这里的设置。")
                entranceIndex: 3

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        HusText {
                            text: qsTr("减少动效")
                            font.pixelSize: 13
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusText {
                            Layout.fillWidth: true
                            text: qsTr("关闭页面、弹窗与悬停过渡，减少视觉移动和渲染开销。")
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextTertiary
                            wrapMode: Text.WordWrap
                        }
                    }

                    HusSwitch {
                        checked: rootView.pendingReducedMotion
                        onToggled: rootView.pendingReducedMotion = checked
                        contentDescription: qsTr("减少动效")
                    }
                }
            }
        }
    }
}
