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

    signal requestDiagnosis()
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
                    // Decode at card size and never at full resolution: a mipmap chain on a
                    // 1376x768 texture costs more than it buys once the image is already
                    // downscaled by sourceSize.
                    sourceSize.width: 1040
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    autoTransform: true
                    smooth: true
                    // 亮色模式保留更丰富的水墨山城对比度，避免泛白发虚
                    opacity: HusTheme.isDark ? 0.40 : 0.70

                    Behavior on opacity {
                        enabled: HusTheme.animationEnabled
                        NumberAnimation { duration: HusTheme.Primary.durationMid }
                    }
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
                //
                // 两个变体同时常驻，只切换透明度。以前 theme 变化会改写 source，QML 便要在
                // GUI 线程同步解码一张 940x1920 的 PNG 并重建 mipmap，主题切换的卡顿就来自
                // 这里；两张预先按显示尺寸解码后，切换只剩一次透明度插值。
                Image {
                    anchors.right: parent.right
                    anchors.rightMargin: 36
                    anchors.verticalCenter: parent.verticalCenter
                    height: 210
                    source: "qrc:/images/sekiro_calligraphy_white.png"
                    sourceSize.height: 420
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    smooth: true
                    opacity: HusTheme.isDark ? 0.18 : 0.0

                    Behavior on opacity {
                        enabled: HusTheme.animationEnabled
                        NumberAnimation { duration: HusTheme.Primary.durationMid }
                    }
                }

                Image {
                    anchors.right: parent.right
                    anchors.rightMargin: 36
                    anchors.verticalCenter: parent.verticalCenter
                    height: 210
                    source: "qrc:/images/sekiro_calligraphy_crimson.png"
                    sourceSize.height: 420
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    smooth: true
                    opacity: HusTheme.isDark ? 0.0 : 0.35

                    Behavior on opacity {
                        enabled: HusTheme.animationEnabled
                        NumberAnimation { duration: HusTheme.Primary.durationMid }
                    }
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
                            color: smmBackend.healthOverall === "" ? HusTheme.Primary.colorTextQuaternary
                                   : smmBackend.canLaunch ? HusTheme.Primary.colorSuccess
                                   : HusTheme.Primary.colorWarning
                        }

                        HusText {
                            text: smmBackend.isDoctorChecking ? qsTr("正在检测环境…")
                                  : smmBackend.healthOverall === "" ? qsTr("启动前会先检测环境")
                                  : smmBackend.canLaunch ? qsTr("检测通过 · 启动前会再检测一次")
                                  : qsTr("上次检测有提示 · 启动前会再检测一次")
                            font.pixelSize: 14
                            color: HusTheme.Primary.colorTextSecondary
                        }
                    }

                    HusText {
                        text: qsTr("隻狼：影逝二度")
                        font.pixelSize: 28
                        font.bold: true
                        color: HusTheme.Primary.colorTextPrimary
                    }

                    HusText {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 560
                        text: qsTr("已部署 %1 个文件。更换模组组合后请先部署；点击启动时先核验环境与文件一致性。")
                              .arg(smmBackend.deployedFiles)
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
                            objectName: "launchButton"
                            text: smmBackend.launchPending ? qsTr("正在检测…") : qsTr("启动只狼")
                            enabled: smmBackend.launchEnabled
                            onClicked: smmBackend.launchGame()
                        }

                        // 启动被拦住时，把"为什么"的入口放在灰掉的按钮旁边，而不是只留在侧栏。
                        HusButton {
                            implicitHeight: 40
                            type: HusButton.Type_Text
                            // 浅色主题下首屏是明亮的画作，默认文本按钮会糊进背景；跟随正文色
                            // 而不是主色，才在深浅两套主题里都够对比度。
                            colorText: HusTheme.Primary.colorTextPrimary
                            visible: smmBackend.healthOverall !== "" && !smmBackend.canLaunch
                                     && !smmBackend.launchPending
                            text: qsTr("查看检测")
                            onClicked: rootView.requestDiagnosis()
                        }

                        // 核心操作 2：部署模组生效
                        HusButton {
                            implicitHeight: 40
                            implicitWidth: 110
                            text: qsTr("仅部署")
                            enabled: smmBackend.launchEnabled
                            onClicked: smmBackend.deploy()
                        }

                        // 还原游戏原始纯净环境。
                        // 用默认按钮而不是描边按钮：首屏是明亮画作，描边按钮在浅色主题下几乎
                        // 没有边界；默认按钮在深浅两种主题里都有实底，可读性稳定。
                        HusButton {
                            implicitHeight: 40
                            text: qsTr("纯净还原")
                            enabled: smmBackend.launchEnabled
                            onClicked: smmBackend.restore()
                        }

                        // 模组军械库快捷导航
                        HusButton {
                            implicitHeight: 40
                            type: HusButton.Type_Text
                            colorText: HusTheme.Primary.colorTextPrimary
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
