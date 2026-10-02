pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Templates as T
import HuskarUI.Basic

HusModal {
    id: rootModal

    width: 640
    title: qsTr("全局配置")
    description: qsTr("指定只狼游戏目录与模组暂存区，二者位于同一 NTFS 卷时可启用零拷贝硬链接部署。")

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
                    smmBackend.saveSettings(stagingDirInput.text, gameDirInput.text);
                    rootModal.close();
                }
            }
        }
    }

    bodyDelegate: ColumnLayout {
        width: parent.width
        spacing: 20

        FolderDialog {
            id: gameDirDialog
            title: qsTr("选择只狼游戏安装根目录")
            onAccepted: {
                if (selectedFolder)
                    gameDirInput.text = selectedFolder.toString().replace("file:///", "");
            }
        }

        FolderDialog {
            id: stagingDirDialog
            title: qsTr("选择模组暂存区目录")
            onAccepted: {
                if (selectedFolder)
                    stagingDirInput.text = selectedFolder.toString().replace("file:///", "");
            }
        }

        // ------------------------------ 游戏目录 ------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            HusText {
                text: qsTr("只狼游戏安装根目录")
                font.pixelSize: 13
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
            }

            HusText {
                Layout.fillWidth: true
                text: qsTr("目录中应包含 sekiro.exe 与 dinput8.dll 注入钩子。")
                font.pixelSize: 11
                color: HusTheme.Primary.colorTextTertiary
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                HusInput {
                    id: gameDirInput
                    Layout.fillWidth: true
                    text: smmBackend ? smmBackend.sekiroDir : ""
                }

                HusButton {
                    text: qsTr("浏览")
                    onClicked: gameDirDialog.open()
                }

                HusButton {
                    text: qsTr("打开")
                    visible: gameDirInput.text.trim() !== ""
                    onClicked: {
                        if (smmBackend) smmBackend.openFolder(gameDirInput.text);
                    }
                }

                HusButton {
                    text: qsTr("自动探测")
                    onClicked: smmBackend.autoDetectGameDir()
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
                font.pixelSize: 13
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
            }

            HusText {
                Layout.fillWidth: true
                text: qsTr("解压与归一化后的模组资产保存在此。建议与游戏置于同一 NTFS 驱动器。")
                font.pixelSize: 11
                color: HusTheme.Primary.colorTextTertiary
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                HusInput {
                    id: stagingDirInput
                    Layout.fillWidth: true
                    text: smmBackend ? smmBackend.stagingDir : ""
                }

                HusButton {
                    text: qsTr("浏览")
                    onClicked: stagingDirDialog.open()
                }

                HusButton {
                    text: qsTr("打开")
                    visible: stagingDirInput.text.trim() !== ""
                    onClicked: {
                        if (smmBackend) smmBackend.openFolder(stagingDirInput.text);
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
                    font.pixelSize: 11
                    color: HusTheme.Primary.colorTextTertiary
                    wrapMode: Text.WordWrap
                }
            }
        }

        HusDivider {
            Layout.fillWidth: true
        }

        // ------------------------------ 界面语言 ------------------------------
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
                // 语言名称必须写原生字面量（不可加 qsTr），避免翻译时重入叠印导致字符错乱
                options: [
                    { label: "简体中文", value: "zh-CN" },
                    { label: "English", value: "en-US" }
                ]
                currentIndex: smmBackend && smmBackend.language === "en-US" ? 1 : 0
                onCurrentIndexChanged: {
                    if (!smmBackend) return;
                    const targetLang = currentIndex === 1 ? "en-US" : "zh-CN";
                    if (smmBackend.language !== targetLang) {
                        smmBackend.language = targetLang;
                    }
                }
            }
        }
    }
}
