pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HuskarUI.Basic

HusModal {
    id: rootModal

    width: 520
    title: qsTr("关于 只狼模组管理器")
    description: qsTr("Sekiro Mod Manager (SMM) · 高性能现代化模组装配工坊")

    footerDelegate: Item {
        width: parent.width
        implicitHeight: footerRow.implicitHeight

        RowLayout {
            id: footerRow
            anchors.right: parent.right
            spacing: 8

            HusButton {
                type: HusButton.Type_Primary
                text: qsTr("关闭")
                onClicked: rootModal.close()
            }
        }
    }

    bodyDelegate: ColumnLayout {
        width: parent.width
        spacing: 16

        // 品牌头部卡片
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 88
            radius: HusTheme.Primary.radiusPrimary
            color: HusTheme.Primary.colorFillQuaternary
            border.width: 1
            border.color: HusTheme.Primary.colorBorderSecondary

            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 14

                Image {
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 56
                    source: "qrc:/images/sekiro_icon_128.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    RowLayout {
                        spacing: 8
                        HusText {
                            text: qsTr("只狼模组管理器")
                            font.pixelSize: 16
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }
                        HusTag {
                            text: "v0.3.2"
                        }
                        HusButton {
                            implicitHeight: 22
                            text: qsTr("检查更新")
                            onClicked: {
                                if (smmBackend && smmBackend.updater) {
                                    smmBackend.updater.checkForUpdates(false);
                                    updateModal.open();
                                }
                            }
                        }
                    }

                    HusText {
                        text: "Sekiro Mod Manager · Powered by C++17 & HuskarUI"
                        font.pixelSize: 11
                        color: HusTheme.Primary.colorTextTertiary
                    }
                }
            }
        }

        // 项目与作者链接
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            // 作者行
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                HusText {
                    Layout.preferredWidth: 80
                    text: qsTr("软件作者")
                    font.pixelSize: 12
                    color: HusTheme.Primary.colorTextTertiary
                }

                HusText {
                    Layout.fillWidth: true
                    text: "RoL1n_SrP"
                    font.pixelSize: 12
                    font.bold: true
                    color: HusTheme.Primary.colorTextPrimary
                }
            }

            // 个人博客
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                HusText {
                    Layout.preferredWidth: 80
                    text: qsTr("个人博客")
                    font.pixelSize: 12
                    color: HusTheme.Primary.colorTextTertiary
                }

                HusText {
                    Layout.fillWidth: true
                    text: "blog.srprolin.top"
                    font.pixelSize: 12
                    color: HusTheme.Primary.colorTextSecondary
                }

                HusButton {
                    implicitHeight: 26
                    type: HusButton.Type_Text
                    text: qsTr("访问博客 ↗")
                    font.pixelSize: 11
                    onClicked: Qt.openUrlExternally("https://blog.srprolin.top")
                }
            }

            // GitHub 仓库
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                HusText {
                    Layout.preferredWidth: 80
                    text: qsTr("开源项目")
                    font.pixelSize: 12
                    color: HusTheme.Primary.colorTextTertiary
                }

                HusText {
                    Layout.fillWidth: true
                    text: "RolinShmily/sekiro-mod-manager"
                    font.pixelSize: 12
                    color: HusTheme.Primary.colorTextSecondary
                    elide: Text.ElideRight
                }

                HusButton {
                    implicitHeight: 26
                    type: HusButton.Type_Text
                    text: qsTr("GitHub ↗")
                    font.pixelSize: 11
                    onClicked: Qt.openUrlExternally("https://github.com/RolinShmily/sekiro-mod-manager")
                }
            }

            // 技术栈
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                HusText {
                    Layout.preferredWidth: 80
                    text: qsTr("核心架构")
                    font.pixelSize: 12
                    color: HusTheme.Primary.colorTextTertiary
                }

                HusText {
                    Layout.fillWidth: true
                    text: "Qt 6.11 · QML · HuskarUI · C++17 · NTFS Hardlink Engine"
                    font.pixelSize: 11
                    color: HusTheme.Primary.colorTextTertiary
                    elide: Text.ElideRight
                }
            }
        }
    }
}
