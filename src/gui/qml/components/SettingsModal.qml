pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Templates as T
import HuskarUI.Basic

HusModal {
    id: rootModal

    width: 640
    title: qsTr("全局配置")
    description: qsTr("指定只狼游戏目录与模组暂存区，二者位于同一 NTFS 卷时可启用零拷贝硬链接部署。")

    property string pendingStagingDir: ""
    property string pendingGameDir: ""
    property string pendingLanguage: "zh-CN"
    property string pendingFontFamily: "Segoe UI"
    property string pendingTheme: "dark"

    function cleanLocalPath(urlVal) {
        if (!urlVal) return "";
        let s = urlVal.toString();
        if (s.startsWith("file:///")) {
            s = s.substring(8);
        } else if (s.startsWith("file://")) {
            s = s.substring(7);
        }
        s = decodeURIComponent(s);
        return s.replace(/\//g, "\\");
    }

    onOpened: {
        pendingStagingDir = cleanLocalPath(smmBackend ? smmBackend.stagingDir : "");
        pendingGameDir = cleanLocalPath(smmBackend ? smmBackend.sekiroDir : "");
        pendingLanguage = smmBackend ? smmBackend.language : "zh-CN";
        pendingFontFamily = smmBackend ? smmBackend.fontFamily : "Segoe UI";
        pendingTheme = smmBackend ? smmBackend.themeMode : "dark";
    }

    // HusPopup 主题把 colorShadow 取作 @colorTextBase，而暗色主题下 colorTextBase 接近白色，
    // 于是弹窗四周会出现一圈发白的“光晕”而不是投影。这里换成真正的黑色投影。
    colorShadow: Qt.rgba(0, 0, 0, HusTheme.isDark ? 0.62 : 0.20)

    // Qt 的 modal Popup 只拦截 press/release，不拦截滚轮与 hover，事件会继续下渗到
    // 弹窗背后的列表并使其滚动（鼠标操作穿透）。这里在遮罩层上加一个只吃滚轮的
    // MouseArea，把滚轮事件就地截断。
    T.Overlay.modal: Item {
        Rectangle {
            anchors.fill: parent
            color: rootModal.colorOverlay
            opacity: rootModal.opacity
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            hoverEnabled: true
            onWheel: (wheel) => wheel.accepted = true
        }
    }

    footerDelegate: Item {
        implicitHeight: 34
        height: implicitHeight
        width: parent.width

        RowLayout {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            HusButton {
                text: qsTr("取消")
                onClicked: rootModal.close()
            }

            HusButton {
                type: HusButton.Type_Primary
                text: qsTr("保存设置")
                onClicked: {
                    if (smmBackend) {
                        smmBackend.saveSettings(
                            rootModal.cleanLocalPath(rootModal.pendingStagingDir.trim()),
                            rootModal.cleanLocalPath(rootModal.pendingGameDir.trim()),
                            rootModal.pendingLanguage,
                            rootModal.pendingFontFamily,
                            rootModal.pendingTheme
                        );
                    }
                    rootModal.close();
                }
            }
        }
    }

    bodyDelegate: ScrollView {
        id: settingsScrollView
        width: parent.width
        implicitHeight: Math.min(bodyColumn.implicitHeight, Math.max(300, (rootModal.parent ? rootModal.parent.height : 760) - 200))
        height: implicitHeight
        contentWidth: width
        contentHeight: bodyColumn.implicitHeight
        clip: true

        ScrollBar.vertical: HusScrollBar {
            policy: ScrollBar.AsNeeded
        }

        ColumnLayout {
            id: bodyColumn
            width: parent.width - (settingsScrollView.ScrollBar.vertical.visible ? 12 : 4)
            spacing: 18

            FolderDialog {
                id: gameDirDialog
                title: qsTr("选择只狼游戏安装根目录")
                onAccepted: {
                    if (selectedFolder) {
                        rootModal.pendingGameDir = rootModal.cleanLocalPath(selectedFolder);
                    }
                }
            }

            FolderDialog {
                id: stagingDirDialog
                title: qsTr("选择模组暂存区目录")
                onAccepted: {
                    if (selectedFolder) {
                        rootModal.pendingStagingDir = rootModal.cleanLocalPath(selectedFolder);
                    }
                }
            }

            // ------------------------------ 游戏目录 ------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                HusText {
                    text: qsTr("只狼游戏安装根目录")
                    font.pixelSize: 15
                    font.bold: true
                    color: HusTheme.Primary.colorTextPrimary
                }

                HusText {
                    Layout.fillWidth: true
                    text: qsTr("目录中应包含 sekiro.exe 与 dinput8.dll 注入钩子。")
                    font.pixelSize: 13
                    color: HusTheme.Primary.colorTextTertiary
                    wrapMode: Text.WordWrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    HusInput {
                        id: gameDirInput
                        Layout.fillWidth: true
                        text: rootModal.pendingGameDir
                        onTextChanged: {
                            if (text !== rootModal.pendingGameDir) {
                                rootModal.pendingGameDir = text;
                            }
                        }
                    }

                    HusButton {
                        text: qsTr("浏览")
                        onClicked: gameDirDialog.open()
                    }

                    HusButton {
                        text: qsTr("打开")
                        visible: rootModal.pendingGameDir.trim() !== ""
                        onClicked: {
                            if (smmBackend) smmBackend.openFolder(rootModal.pendingGameDir);
                        }
                    }

                    HusButton {
                        text: qsTr("自动探测")
                        onClicked: {
                            if (smmBackend) {
                                const detected = smmBackend.detectSekiroDir();
                                if (detected && detected !== "") {
                                    rootModal.pendingGameDir = rootModal.cleanLocalPath(detected);
                                } else {
                                    smmBackend.autoDetectGameDir();
                                }
                            }
                        }
                    }
                }
            }

            HusDivider {
                Layout.fillWidth: true
            }

            // ------------------------------ 暂存区 ------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                HusText {
                    text: qsTr("模组暂存区目录")
                    font.pixelSize: 15
                    font.bold: true
                    color: HusTheme.Primary.colorTextPrimary
                }

                HusText {
                    Layout.fillWidth: true
                    text: qsTr("解压与归一化后的模组资产保存在此。建议与游戏置于同一 NTFS 驱动器。")
                    font.pixelSize: 13
                    color: HusTheme.Primary.colorTextTertiary
                    wrapMode: Text.WordWrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    HusInput {
                        id: stagingDirInput
                        Layout.fillWidth: true
                        text: rootModal.pendingStagingDir
                        onTextChanged: {
                            if (text !== rootModal.pendingStagingDir) {
                                rootModal.pendingStagingDir = text;
                            }
                        }
                    }

                    HusButton {
                        text: qsTr("浏览")
                        onClicked: stagingDirDialog.open()
                    }

                    HusButton {
                        text: qsTr("打开")
                        visible: rootModal.pendingStagingDir.trim() !== ""
                        onClicked: {
                            if (smmBackend) smmBackend.openFolder(rootModal.pendingStagingDir);
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

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
                        font.pixelSize: 13
                        color: HusTheme.Primary.colorTextTertiary
                        wrapMode: Text.WordWrap
                    }
                }
            }

            HusDivider {
                Layout.fillWidth: true
            }

            // ------------------------------ 外观与个性化 (主题与语言) ------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                // 主题模式选择
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    HusText {
                        text: qsTr("界面主题")
                        font.pixelSize: 15
                        font.bold: true
                        color: HusTheme.Primary.colorTextPrimary
                    }

                    HusSegmented {
                        id: themeSegmented
                        Layout.fillWidth: true
                        block: true
                        options: [
                            { label: qsTr("暗色主题"), value: "dark" },
                            { label: qsTr("亮色主题"), value: "light" },
                            { label: qsTr("跟随系统"), value: "system" }
                        ]
                        currentIndex: {
                            if (rootModal.pendingTheme === "light") return 1;
                            if (rootModal.pendingTheme === "system") return 2;
                            return 0;
                        }
                        onCurrentIndexChanged: {
                            if (currentIndex === 1) rootModal.pendingTheme = "light";
                            else if (currentIndex === 2) rootModal.pendingTheme = "system";
                            else rootModal.pendingTheme = "dark";
                        }
                    }
                }

                // 界面语言选择
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    HusText {
                        text: qsTr("界面语言")
                        font.pixelSize: 15
                        font.bold: true
                        color: HusTheme.Primary.colorTextPrimary
                    }

                    HusSegmented {
                        id: languageSegmented
                        Layout.fillWidth: true
                        block: true
                        // 语言名称必须写原生字面量（不可加 qsTr），避免翻译时重入叠印导致字符错乱
                        options: [
                            { label: "简体中文", value: "zh-CN" },
                            { label: "English", value: "en-US" }
                        ]
                        currentIndex: rootModal.pendingLanguage === "en-US" ? 1 : 0
                        onCurrentIndexChanged: {
                            rootModal.pendingLanguage = (currentIndex === 1 ? "en-US" : "zh-CN");
                        }
                    }
                }
            }

            HusDivider {
                Layout.fillWidth: true
            }

            // ------------------------------ 界面字体 ------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                HusText {
                    text: qsTr("界面字体")
                    font.pixelSize: 15
                    font.bold: true
                    color: HusTheme.Primary.colorTextPrimary
                }

                HusText {
                    Layout.fillWidth: true
                    text: qsTr("选用 Windows 系统原生字体排版。默认推荐 Segoe UI，兼顾优雅西文字形与原生中文字符渲染。")
                    font.pixelSize: 13
                    color: HusTheme.Primary.colorTextTertiary
                    wrapMode: Text.WordWrap
                }

                HusSelect {
                    id: fontSelect
                    Layout.fillWidth: true
                    implicitHeight: 32
                    textRole: "label"
                    valueRole: "value"
                    model: smmBackend ? smmBackend.availableFonts : []
                    displayText: {
                        if (!model || model.length === 0) return rootModal.pendingFontFamily;
                        for (let i = 0; i < model.length; ++i) {
                            const it = model[i];
                            if (it && it.value === rootModal.pendingFontFamily) {
                                return it.label;
                            }
                        }
                        if (currentIndex >= 0 && currentIndex < model.length && model[currentIndex]) {
                            return model[currentIndex].label;
                        }
                        return rootModal.pendingFontFamily;
                    }
                    currentIndex: {
                        if (!model || model.length === 0) return 0;
                        for (let i = 0; i < model.length; ++i) {
                            const it = model[i];
                            if (it && it.value === rootModal.pendingFontFamily) return i;
                        }
                        return 0;
                    }
                    onActivated: (index) => {
                        if (model && model[index]) {
                            rootModal.pendingFontFamily = model[index].value;
                        }
                    }
                }
            }
        }
    }
}
