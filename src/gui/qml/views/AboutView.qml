pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HuskarUI.Basic
// The page composes the shared header/card components; the relative import keeps the QML
// runnable straight from the source tree (`smm_gui --dev`), not only from the module build.
import "../components"

/// 关于页：软件信息、版本更新、项目与作者。
///
/// 分组从"字号 + 分割线"升级为"页头 + 卡片 + 图标"：品牌卡、更新卡、来源卡各有独立表面，
/// 每张卡片头部都有图标，行内再用图标区分作者 / 博客 / 仓库 / 许可。
ColumnLayout {
    id: rootView
    objectName: "aboutView"
    spacing: 14

    readonly property var updater: smmBackend ? smmBackend.updater : null
    readonly property int contentWidth: 900

    // ---------------------------------- 页头 ----------------------------------
    PageHeader {
        iconSource: HusIcon.InfoCircleOutlined
        title: qsTr("关于")
        subtitle: qsTr("软件信息、项目链接与版本更新。")
    }

    ScrollView {
        id: aboutScrollView
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.maximumWidth: rootView.contentWidth
        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
        contentWidth: width
        contentHeight: aboutColumn.implicitHeight
        clip: true

        ScrollBar.vertical: HusScrollBar {
            policy: ScrollBar.AsNeeded
        }

        ColumnLayout {
            id: aboutColumn
            width: parent.width - (aboutScrollView.ScrollBar.vertical.visible ? 12 : 4)
            spacing: 24

            // ------------------------------ 品牌 ------------------------------
            SectionCard {
                iconSource: HusIcon.StarOutlined
                title: qsTr("软件信息")

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    Image {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 64
                        source: "qrc:/images/sekiro_icon_128.png"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        RowLayout {
                            spacing: 8

                            HusText {
                                text: qsTr("只狼模组管理器")
                                font.pixelSize: 17
                                font.bold: true
                                color: HusTheme.Primary.colorTextPrimary
                            }

                            HusTag {
                                text: "v" + (rootView.updater ? rootView.updater.currentVersion : "")
                                colorText: HusTheme.Primary.colorPrimary
                            }
                        }

                        HusText {
                            Layout.fillWidth: true
                            text: qsTr("Sekiro Mod Manager · 高性能现代化模组装配工坊")
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextSecondary
                            wrapMode: Text.WordWrap
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Repeater {
                                model: ["Qt 6", "QML", "HuskarUI", "C++17", "NTFS Hardlink"]

                                delegate: HusTag {
                                    required property string modelData
                                    text: modelData
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }
                }
            }

            // ------------------------------ 版本更新 ------------------------------
            SectionCard {
                iconSource: HusIcon.CloudDownloadOutlined
                title: qsTr("版本更新")
                description: qsTr("通过 GitHub 发布清单校验最新版本；检查结果会留在本页，不会弹出额外窗口。")

                extra: Component {
                    HusIconButton {
                        text: (rootView.updater && rootView.updater.isChecking)
                              ? qsTr("检查中…")
                              : qsTr("检查更新")
                        iconSource: HusIcon.ReloadOutlined
                        iconSize: 14
                        enabled: rootView.updater && !rootView.updater.isChecking
                        onClicked: if (rootView.updater) rootView.updater.checkForUpdates(false)
                    }
                }

                // 检查中
                RowLayout {
                    Layout.fillWidth: true
                    visible: rootView.updater && rootView.updater.isChecking
                    spacing: 10

                    HusSpin {
                        Layout.alignment: Qt.AlignVCenter
                        spinSize: 16
                        indicatorItemCount: 4
                    }

                    HusText {
                        Layout.fillWidth: true
                        text: qsTr("正在连接 GitHub 校验最新版本清单…")
                        font.pixelSize: 13
                        color: HusTheme.Primary.colorTextPrimary
                        wrapMode: Text.WordWrap
                    }
                }

                // 发现新版本
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: rootView.updater && !rootView.updater.isChecking && rootView.updater.updateAvailable
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusIconText {
                            iconSource: HusIcon.ExclamationCircleOutlined
                            iconSize: 15
                            colorIcon: HusTheme.Primary.colorPrimary
                        }

                        HusText {
                            text: rootView.updater ? ("v" + rootView.updater.latestVersion) : ""
                            font.pixelSize: 15
                            font.bold: true
                            color: HusTheme.Primary.colorTextPrimary
                        }

                        HusTag {
                            text: qsTr("新版本可用")
                            colorText: HusTheme.Primary.colorPrimary
                        }

                        Item { Layout.fillWidth: true }

                        HusText {
                            text: rootView.updater ? (qsTr("当前版本: v") + rootView.updater.currentVersion) : ""
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextTertiary
                        }
                    }

                    HusText {
                        Layout.fillWidth: true
                        text: qsTr("官方推荐通过安装程序平滑升级；便携版用户可前往 GitHub 发布页下载最新压缩包解压使用。")
                        font.pixelSize: 12
                        color: HusTheme.Primary.colorTextSecondary
                        wrapMode: Text.WordWrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusIconButton {
                            text: qsTr("下载安装程序 (.exe)")
                            iconSource: HusIcon.DownloadOutlined
                            iconSize: 14
                            onClicked: if (rootView.updater) rootView.updater.openSetupDownload()
                        }

                        HusIconButton {
                            text: qsTr("前往发布页面")
                            iconSource: HusIcon.ExportOutlined
                            iconSize: 14
                            type: HusButton.Type_Primary
                            onClicked: if (rootView.updater) rootView.updater.openReleasePage()
                        }

                        Item { Layout.fillWidth: true }
                    }

                    // 更新内容
                    HusText {
                        Layout.fillWidth: true
                        visible: rootView.updater && rootView.updater.releaseNotes.length > 0
                        text: qsTr("更新内容：")
                        font.pixelSize: 12
                        font.bold: true
                        color: HusTheme.Primary.colorTextTertiary
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        visible: rootView.updater && rootView.updater.releaseNotes.length > 0
                        radius: HusTheme.Primary.radiusPrimary
                        color: HusTheme.Primary.colorBgBase
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
                                text: rootView.updater ? rootView.updater.releaseNotes : ""
                                font.pixelSize: 12
                                color: HusTheme.Primary.colorTextSecondary
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                // 已是最新 / 尚未检查
                RowLayout {
                    Layout.fillWidth: true
                    visible: rootView.updater && !rootView.updater.isChecking && !rootView.updater.updateAvailable
                    spacing: 8

                    HusIconText {
                        iconSource: rootView.updater && rootView.updater.statusMessage !== ""
                                    ? HusIcon.CheckCircleOutlined
                                    : HusIcon.ClockCircleOutlined
                        iconSize: 15
                        colorIcon: rootView.updater && rootView.updater.statusMessage !== ""
                                   ? HusTheme.Primary.colorSuccess
                                   : HusTheme.Primary.colorTextTertiary
                    }

                    HusTag {
                        text: rootView.updater && rootView.updater.statusMessage !== ""
                              ? qsTr("已是最新")
                              : qsTr("尚未检查")
                        colorText: rootView.updater && rootView.updater.statusMessage !== ""
                                   ? HusTheme.Primary.colorSuccess
                                   : HusTheme.Primary.colorTextTertiary
                    }

                    HusText {
                        Layout.fillWidth: true
                        text: {
                            if (!rootView.updater) return "";
                            if (rootView.updater.statusMessage === "") {
                                return qsTr("点击右上角“检查更新”获取最新版本信息。");
                            }
                            return rootView.updater.statusMessage;
                        }
                        font.pixelSize: 13
                        color: HusTheme.Primary.colorTextSecondary
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // ------------------------------ 项目与作者 ------------------------------
            SectionCard {
                iconSource: HusIcon.UserOutlined
                title: qsTr("项目与作者")

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    HusIconText {
                        Layout.preferredWidth: 20
                        iconSource: HusIcon.UserOutlined
                        iconSize: 15
                        colorIcon: HusTheme.Primary.colorTextTertiary
                    }

                    HusText {
                        Layout.preferredWidth: 88
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

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    HusIconText {
                        Layout.preferredWidth: 20
                        iconSource: HusIcon.LinkOutlined
                        iconSize: 15
                        colorIcon: HusTheme.Primary.colorTextTertiary
                    }

                    HusText {
                        Layout.preferredWidth: 88
                        text: qsTr("个人博客")
                        font.pixelSize: 12
                        color: HusTheme.Primary.colorTextTertiary
                    }

                    HusText {
                        Layout.fillWidth: true
                        text: "blog.srprolin.com"
                        font.pixelSize: 12
                        color: HusTheme.Primary.colorTextSecondary
                        elide: Text.ElideRight
                    }

                    HusIconButton {
                        text: qsTr("访问博客")
                        iconSource: HusIcon.ExportOutlined
                        iconSize: 13
                        iconPosition: HusIconButton.Position_End
                        type: HusButton.Type_Text
                        onClicked: Qt.openUrlExternally("https://blog.srprolin.com")
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    HusIconText {
                        Layout.preferredWidth: 20
                        iconSource: HusIcon.GithubOutlined
                        iconSize: 15
                        colorIcon: HusTheme.Primary.colorTextTertiary
                    }

                    HusText {
                        Layout.preferredWidth: 88
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

                    HusIconButton {
                        text: qsTr("GitHub")
                        iconSource: HusIcon.ExportOutlined
                        iconSize: 13
                        iconPosition: HusIconButton.Position_End
                        type: HusButton.Type_Text
                        onClicked: Qt.openUrlExternally("https://github.com/RolinShmily/sekiro-mod-manager")
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    HusIconText {
                        Layout.preferredWidth: 20
                        iconSource: HusIcon.SafetyCertificateOutlined
                        iconSize: 15
                        colorIcon: HusTheme.Primary.colorTextTertiary
                    }

                    HusText {
                        Layout.preferredWidth: 88
                        text: qsTr("开源许可")
                        font.pixelSize: 12
                        color: HusTheme.Primary.colorTextTertiary
                    }

                    HusText {
                        Layout.fillWidth: true
                        text: qsTr("MIT License · 游戏美术与《只狼》相关内容版权归 FromSoftware 所有")
                        font.pixelSize: 12
                        color: HusTheme.Primary.colorTextQuaternary
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
