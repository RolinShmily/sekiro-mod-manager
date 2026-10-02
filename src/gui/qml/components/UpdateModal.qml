pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HuskarUI.Basic

HusModal {
    id: rootModal

    width: 540
    title: qsTr("软件版本更新")
    description: qsTr("SEKIRO MOD MANAGER · 版本更新通知")

    readonly property var updater: smmBackend ? smmBackend.updater : null

    footerDelegate: Item {
        width: parent.width
        implicitHeight: footerRow.implicitHeight

        RowLayout {
            id: footerRow
            anchors.right: parent.right
            spacing: 8

            // 当有更新可用时
            HusButton {
                visible: rootModal.updater && rootModal.updater.updateAvailable
                text: qsTr("稍后再说")
                onClicked: rootModal.close()
            }

            HusButton {
                visible: rootModal.updater && rootModal.updater.updateAvailable
                text: qsTr("下载安装程序 (.exe)")
                onClicked: {
                    if (rootModal.updater) rootModal.updater.openSetupDownload();
                    rootModal.close();
                }
            }

            HusButton {
                visible: rootModal.updater && rootModal.updater.updateAvailable
                type: HusButton.Type_Primary
                text: qsTr("前往发布页面")
                onClicked: {
                    if (rootModal.updater) rootModal.updater.openReleasePage();
                    rootModal.close();
                }
            }

            // 当无更新或检查中时
            HusButton {
                visible: rootModal.updater && !rootModal.updater.updateAvailable && !rootModal.updater.isChecking
                text: qsTr("重新检查")
                onClicked: {
                    if (rootModal.updater) rootModal.updater.checkForUpdates(false);
                }
            }

            HusButton {
                visible: rootModal.updater && !rootModal.updater.updateAvailable
                type: HusButton.Type_Primary
                text: qsTr("完成")
                onClicked: rootModal.close()
            }
        }
    }

    bodyDelegate: ColumnLayout {
        width: parent.width
        spacing: 14

        // 状态卡片
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: statusCol.implicitHeight + 24
            radius: HusTheme.Primary.radiusPrimary
            color: HusTheme.Primary.colorFillQuaternary
            border.width: 1
            border.color: HusTheme.Primary.colorBorderSecondary

            ColumnLayout {
                id: statusCol
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                // 正在检查中
                RowLayout {
                    visible: rootModal.updater && rootModal.updater.isChecking
                    Layout.fillWidth: true
                    spacing: 10

                    HusTag {
                        text: qsTr("检查中")
                    }

                    HusText {
                        text: qsTr("正在连接 GitHub 校验最新版本清单...")
                        font.pixelSize: 13
                        color: HusTheme.Primary.colorTextPrimary
                    }
                }

                // 发现新版本
                ColumnLayout {
                    visible: rootModal.updater && rootModal.updater.updateAvailable
                    Layout.fillWidth: true
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusTag {
                            text: qsTr("新版本可用")
                            colorText: HusTheme.Primary.colorPrimary
                        }

                        HusText {
                            text: rootModal.updater ? ("v" + rootModal.updater.latestVersion) : ""
                            font.pixelSize: 16
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        Item { Layout.fillWidth: true }

                        HusText {
                            text: rootModal.updater ? (qsTr("当前版本: v") + rootModal.updater.currentVersion) : ""
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextTertiary
                        }
                    }

                    HusText {
                        Layout.fillWidth: true
                        text: qsTr("官方推荐通过安装程序进行平滑升级；便携版用户可前往 GitHub 发布页下载最新压缩包解压使用。")
                        font.pixelSize: 12
                        color: HusTheme.Primary.colorTextSecondary
                        wrapMode: Text.WordWrap
                    }
                }

                // 已是最新版本
                RowLayout {
                    visible: rootModal.updater && !rootModal.updater.isChecking && !rootModal.updater.updateAvailable
                    Layout.fillWidth: true
                    spacing: 8

                    HusTag {
                        text: qsTr("已是最新")
                        colorText: HusTheme.Primary.colorSuccess
                    }

                    HusText {
                        text: rootModal.updater ? (qsTr("当前版本 (v%1) 为最新发布版本，无需更新。").arg(rootModal.updater.currentVersion)) : ""
                        font.pixelSize: 13
                        color: HusTheme.Primary.colorTextPrimary
                    }
                }
            }
        }

        // 版本更新日志
        ColumnLayout {
            visible: rootModal.updater && rootModal.updater.updateAvailable && rootModal.updater.releaseNotes.length > 0
            Layout.fillWidth: true
            spacing: 6

            HusText {
                text: qsTr("更新内容详情:")
                font.pixelSize: 12
                font.bold: true
                color: HusTheme.Primary.colorTextTertiary
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 130
                radius: HusTheme.Primary.radiusPrimary
                color: HusTheme.Primary.colorFillQuaternary
                border.width: 1
                border.color: HusTheme.Primary.colorBorderSecondary

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 10
                    clip: true

                    ScrollBar.vertical: HusScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    HusText {
                        width: parent.width - 12
                        text: rootModal.updater ? rootModal.updater.releaseNotes : ""
                        font.pixelSize: 12
                        color: HusTheme.Primary.colorTextSecondary
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
