pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HuskarUI.Basic

ScrollView {
    id: rootView
    clip: true
    contentWidth: availableWidth

    ScrollBar.vertical: HusScrollBar {
        policy: ScrollBar.AsNeeded
    }

    signal navigateToArmoury()
    signal navigateToModPacks()

    function formatBytes(bytes) {
        if (!bytes || bytes <= 0)
            return "0 B";
        const units = ["B", "KB", "MB", "GB", "TB"];
        let value = bytes;
        let index = 0;
        while (value >= 1024 && index < units.length - 1) {
            value /= 1024;
            index++;
        }
        return (index === 0 ? value.toFixed(0) : value.toFixed(1)) + " " + units[index];
    }

    ColumnLayout {
        width: rootView.availableWidth
        spacing: 16

        // =====================================================================
        // 部署面板（沉浸式只狼概念画卷背景 + 真迹水墨书法水印，100% 边缘铺满）
        // =====================================================================
        Rectangle {
            id: heroCard
            Layout.fillWidth: true
            implicitHeight: 250
            radius: HusTheme.Primary.radiusPrimaryLG
            clip: true
            color: HusTheme.Primary.colorFillQuaternary
            border.width: 1
            border.color: HusTheme.isDark ? HusTheme.Primary.colorBorderSecondary : Qt.rgba(0, 0, 0, 0.12)

            // 沉浸式只狼背景画卷（真正铺满全卡片边界）
            Item {
                anchors.fill: parent
                clip: true
                z: 0

                Image {
                    id: heroBgImage
                    anchors.fill: parent
                    source: "qrc:/images/sekiro_hero_art.png"
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    autoTransform: true
                    mipmap: true
                    smooth: true
                    // 亮色模式保留更丰富的水墨山城对比度，避免泛白发虚
                    opacity: HusTheme.isDark ? 0.40 : 0.70
                }

                // 渐变蒙版：深色模式暗色遮罩保清晰，亮色模式宣纸水墨过渡显层次
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop {
                            position: 0.0
                            color: HusTheme.isDark ? Qt.rgba(0.08, 0.08, 0.10, 0.95) : Qt.rgba(0.96, 0.96, 0.97, 0.88)
                        }
                        GradientStop {
                            position: 0.50
                            color: HusTheme.isDark ? Qt.rgba(0.08, 0.08, 0.10, 0.80) : Qt.rgba(0.96, 0.96, 0.97, 0.50)
                        }
                        GradientStop {
                            position: 1.0
                            color: HusTheme.isDark ? Qt.rgba(0.08, 0.08, 0.10, 0.25) : Qt.rgba(0.96, 0.96, 0.97, 0.05)
                        }
                    }
                }

                // 右侧「隻狼」真迹水墨书法水印（亮色模式加深朱砂印泥饱和度）
                Image {
                    anchors.right: parent.right
                    anchors.rightMargin: 36
                    anchors.verticalCenter: parent.verticalCenter
                    height: 210
                    fillMode: Image.PreserveAspectFit
                    source: HusTheme.isDark
                            ? "qrc:/images/sekiro_calligraphy_white.png"
                            : "qrc:/images/sekiro_calligraphy_crimson.png"
                    opacity: HusTheme.isDark ? 0.18 : 0.35
                    smooth: true
                    mipmap: true
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 24
                z: 1

                // 强调色装饰条
                Rectangle {
                    Layout.fillHeight: true
                    implicitWidth: 3
                    radius: 1.5
                    color: HusTheme.Primary.colorPrimary
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    RowLayout {
                        spacing: 7

                        Rectangle {
                            Layout.alignment: Qt.AlignVCenter
                            implicitWidth: 7
                            implicitHeight: 7
                            radius: 3.5
                            color: HusTheme.Primary.colorSuccess
                        }

                        HusText {
                            text: qsTr("已就绪 · 当前方案处于活跃投影状态")
                            font.pixelSize: 14
                            color: HusTheme.Primary.colorTextSecondary
                        }
                    }

                    RowLayout {
                        spacing: 10

                        HusText {
                            text: qsTr("隻狼：影逝二度")
                            font.pixelSize: 28
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusTag {
                            Layout.alignment: Qt.AlignVCenter
                            text: "STEAM v1.06"
                        }
                    }

                    HusText {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 560
                        text: qsTr("装配方案「剑圣孤影 · 断绝不死」已装载。物理硬链接零拷贝投影就绪，随时可出征苇名。")
                        font.pixelSize: 14
                        color: HusTheme.Primary.colorTextTertiary
                        wrapMode: Text.WordWrap
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Layout.topMargin: 6
                        spacing: 10

                        // 核心操作 1：启动游戏
                        HusButton {
                            implicitHeight: 40
                            implicitWidth: 140
                            type: HusButton.Type_Primary
                            text: qsTr("启动只狼")
                            onClicked: smmBackend.launchGame()
                        }

                        // 核心操作 2：部署模组生效
                        HusButton {
                            implicitHeight: 40
                            implicitWidth: 110
                            text: qsTr("仅部署")
                            onClicked: smmBackend.deploy()
                        }

                        // 还原游戏原始纯净环境
                        HusButton {
                            implicitHeight: 40
                            text: qsTr("纯净还原")
                            onClicked: smmBackend.restore()
                        }

                        // 模组军械库快捷导航
                        HusButton {
                            implicitHeight: 40
                            type: HusButton.Type_Text
                            text: qsTr("管理模组")
                            onClicked: rootView.navigateToArmoury()
                        }
                    }
                }
            }
        }

        // =====================================================================
        // 指标面板
        // =====================================================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            HusFrame {
                Layout.fillWidth: true
                implicitHeight: 132
                padding: 18
                colorBg: HusTheme.Primary.colorFillQuaternary
                borderBg.color: HusTheme.Primary.colorBorderSecondary

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        HusText {
                            Layout.fillWidth: true
                            text: qsTr("物理投影效能")
                            font.pixelSize: 15
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusTag {
                            Layout.alignment: Qt.AlignVCenter
                            tagState: HusTag.State_Success
                            text: qsTr("零拷贝可用")
                        }
                    }

                    RowLayout {
                        spacing: 40

                        ColumnLayout {
                            spacing: 2

                            HusText {
                                text: smmBackend ? String(smmBackend.deployedFiles) : "0"
                                font.pixelSize: 28
                                font.bold: true
                                color: HusTheme.Primary.colorTextPrimary
                            }

                            HusText {
                                text: qsTr("胜出投影文件")
                                font.pixelSize: 13
                                color: HusTheme.Primary.colorTextQuaternary
                            }
                        }

                        ColumnLayout {
                            spacing: 2

                            HusText {
                                text: rootView.formatBytes(smmBackend ? smmBackend.bytesSaved : 0)
                                font.pixelSize: 28
                                font.bold: true
                                color: HusTheme.Primary.colorTextPrimary
                            }

                            HusText {
                                text: qsTr("磁盘空间节省")
                                font.pixelSize: 13
                                color: HusTheme.Primary.colorTextQuaternary
                            }
                        }
                    }
                }
            }

            HusFrame {
                Layout.fillWidth: true
                implicitHeight: 132
                padding: 18
                colorBg: HusTheme.Primary.colorFillQuaternary
                borderBg.color: HusTheme.Primary.colorBorderSecondary

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        HusText {
                            Layout.fillWidth: true
                            text: qsTr("语义冲突仲裁")
                            font.pixelSize: 15
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusTag {
                            Layout.alignment: Qt.AlignVCenter
                            tagState: smmBackend && smmBackend.conflictCount > 0
                                      ? HusTag.State_Warning
                                      : HusTag.State_Success
                            text: qsTr("%1 项已裁决").arg(smmBackend ? smmBackend.conflictCount : 0)
                        }
                    }

                    HusText {
                        Layout.fillWidth: true
                        text: qsTr("引擎按优先级自动排序投射，低顺位模组的重合文件被自动遮罩，无需手动干预。")
                        font.pixelSize: 14
                        color: HusTheme.Primary.colorTextTertiary
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
