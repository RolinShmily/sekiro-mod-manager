pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HuskarUI.Basic

import "views"
import "components"

HusWindow {
    id: mainWindow

    width: 1280
    height: 840
    minimumWidth: 1040
    minimumHeight: 680
    visible: true
    title: qsTr("只狼模组管理器")

    color: HusTheme.Primary.colorBgBase

    Behavior on color {
        enabled: HusTheme.animationEnabled
        ColorAnimation { duration: 180; easing.type: Easing.OutQuad }
    }

    property string currentNav: "hub" // "hub" | "armoury" | "packs"

    // =========================================================================
    // 标题栏：品牌标识 + 主题切换 + 系统按钮（全部交由 HusCaptionBar 处理）
    // =========================================================================
    captionBar.visible: true
    captionBar.height: 40
    captionBar.showThemeButton: true
    // 置空默认图标，使自定义墨宝标题紧贴左侧边缘对齐
    captionBar.winIconDelegate: null

    captionBar.winTitleDelegate: RowLayout {
        spacing: 9

        // 隻狼黑底真迹墨宝 Icon
        Image {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: 26
            Layout.preferredHeight: 26
            source: "qrc:/images/sekiro_icon_64.png"
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
        }

        HusText {
            text: "SEKIRO MOD MANAGER"
            font.pixelSize: 12
            font.bold: true
            font.letterSpacing: 0.5
            color: HusTheme.Primary.colorTextPrimary
            verticalAlignment: Text.AlignVCenter
        }
    }

    // =========================================================================
    // 主体：左侧导航导轨 + 右侧内容舞台
    // =========================================================================
    RowLayout {
        anchors.top: mainWindow.captionBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 0

        // ------------------------------ 导航导轨 ------------------------------
        Rectangle {
            Layout.fillHeight: true
            implicitWidth: 208
            color: HusTheme.Primary.colorFillQuaternary

            Behavior on color {
                enabled: HusTheme.animationEnabled
                ColorAnimation { duration: 180; easing.type: Easing.OutQuad }
            }

            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: HusTheme.Primary.colorSplit
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 4

                HusText {
                    Layout.leftMargin: 12
                    Layout.topMargin: 2
                    Layout.bottomMargin: 6
                    text: qsTr("导航")
                    font.pixelSize: 13
                    font.bold: true
                    color: HusTheme.Primary.colorTextQuaternary
                }

                NavItem {
                    Layout.fillWidth: true
                    label: qsTr("启动与工作台")
                    iconSource: HusIcon.HomeOutlined
                    active: mainWindow.currentNav === "hub"
                    onClicked: mainWindow.currentNav = "hub"
                }

                NavItem {
                    Layout.fillWidth: true
                    label: qsTr("模组管理")
                    iconSource: HusIcon.AppstoreOutlined
                    active: mainWindow.currentNav === "armoury"
                    onClicked: mainWindow.currentNav = "armoury"
                }

                NavItem {
                    Layout.fillWidth: true
                    label: qsTr("整合包预设")
                    iconSource: HusIcon.InboxOutlined
                    active: mainWindow.currentNav === "packs"
                    onClicked: mainWindow.currentNav = "packs"
                }

                Item { Layout.fillHeight: true }

                // 运行状态摘要（点击唤起全景健康诊断）
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: statusCol.implicitHeight + 24
                    radius: HusTheme.Primary.radiusPrimary
                    color: HusTheme.Primary.colorFillSecondary
                    border.width: 1
                    border.color: HusTheme.Primary.colorBorderSecondary

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: doctorModal.open()
                    }

                    ColumnLayout {
                        id: statusCol
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true

                            HusText {
                                text: qsTr("运行状态")
                                font.pixelSize: 13
                                font.bold: true
                                color: HusTheme.Primary.colorTextTertiary
                            }

                            Item { Layout.fillWidth: true }

                            HusText {
                                text: qsTr("诊断 ↗")
                                font.pixelSize: 12
                                color: HusTheme.Primary.colorPrimary
                            }
                        }

                        Repeater {
                            model: [
                                {
                                    k: qsTr("ModEngine 钩子"),
                                    v: qsTr("已就绪"),
                                    ok: true
                                },
                                {
                                    k: qsTr("NTFS 零拷贝卷"),
                                    v: smmBackend && smmBackend.isNtfsMatched ? qsTr("同卷匹配") : qsTr("回退复制"),
                                    ok: smmBackend ? smmBackend.isNtfsMatched : false
                                },
                                {
                                    k: qsTr("语义冲突仲裁"),
                                    v: qsTr("%1 项").arg(smmBackend ? smmBackend.conflictCount : 0),
                                    ok: smmBackend ? smmBackend.conflictCount === 0 : true
                                }
                            ]

                            delegate: RowLayout {
                                id: statusRow
                                required property var model

                                Layout.fillWidth: true
                                spacing: 8

                                Rectangle {
                                    Layout.alignment: Qt.AlignVCenter
                                    implicitWidth: 6
                                    implicitHeight: 6
                                    radius: 3
                                    color: statusRow.model.ok
                                           ? HusTheme.Primary.colorSuccess
                                           : HusTheme.Primary.colorWarning
                                }

                                HusText {
                                    Layout.fillWidth: true
                                    text: statusRow.model.k
                                    font.pixelSize: 12
                                    color: HusTheme.Primary.colorTextTertiary
                                    elide: Text.ElideRight
                                }

                                HusText {
                                    text: statusRow.model.v
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: HusTheme.Primary.colorTextSecondary
                                }
                            }
                        }
                    }
                }

                NavItem {
                    Layout.fillWidth: true
                    label: qsTr("关于")
                    iconSource: HusIcon.InfoCircleOutlined
                    onClicked: aboutModal.open()
                }

                NavItem {
                    Layout.fillWidth: true
                    label: qsTr("全局配置")
                    iconSource: HusIcon.SettingOutlined
                    onClicked: settingsModal.open()
                }
            }
        }

        // ------------------------------ 内容舞台 ------------------------------
        StackLayout {
            id: stageStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 20
            currentIndex: mainWindow.currentNav === "hub" ? 0 : (mainWindow.currentNav === "armoury" ? 1 : 2)

            function animatePage() {
                viewAnim.stop();
                opacity = 1.0;
                if (HusTheme.animationEnabled) viewAnim.restart();
            }
            onCurrentIndexChanged: animatePage()

            NumberAnimation {
                id: viewAnim
                target: stageStack
                property: "opacity"
                from: 0.80
                to: 1.0
                duration: 180
                easing.type: Easing.OutQuad
            }

            Loader {
                sourceComponent: LaunchHubView {
                    onNavigateToArmoury: mainWindow.currentNav = "armoury"
                    onNavigateToModPacks: mainWindow.currentNav = "packs"
                }
            }

            Loader {
                id: armouryLoader
                objectName: "armouryLoader"
                property bool visited: false
                active: visited || StackLayout.isCurrentItem
                asynchronous: true
                onLoaded: {
                    visited = true;
                    if (StackLayout.isCurrentItem) stageStack.animatePage();
                }
                sourceComponent: ArmouryView {
                    onRequestOpenDrawer: (modId) => {
                        smmBackend.openModDetail(modId);
                        modDrawer.open();
                    }
                    onRequestSavePack: savePackModal.open()
                }
            }

            Loader {
                objectName: "packsLoader"
                property bool visited: false
                active: visited || StackLayout.isCurrentItem
                asynchronous: true
                onLoaded: {
                    visited = true;
                    if (StackLayout.isCurrentItem) stageStack.animatePage();
                }
                sourceComponent: ModPackView {
                    onRequestSavePack: savePackModal.open()
                }
            }
        }
    }

    ModDrawer {
        id: modDrawer
    }

    SettingsModal {
        id: settingsModal
    }

    AboutModal {
        id: aboutModal
    }

    DoctorModal {
        id: doctorModal
    }

    UpdateModal {
        id: updateModal
    }

    SavePackModal {
        id: savePackModal
    }

    // -------------------------------------------------------------------------
    // 全局消息浮层 (Toast Notification)
    // -------------------------------------------------------------------------
    Rectangle {
        id: toastBanner
        z: 99999
        anchors.top: mainWindow.captionBar.bottom
        anchors.topMargin: 12
        transform: Translate {
            y: toastBanner.toastVisible || !HusTheme.animationEnabled ? 0 : -16
            Behavior on y {
                enabled: HusTheme.animationEnabled
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }
        }
        anchors.horizontalCenter: parent.horizontalCenter
        implicitHeight: 36
        implicitWidth: toastContent.implicitWidth + 32
        radius: 18
        color: HusTheme.isDark ? Qt.rgba(0.12, 0.12, 0.15, 0.95) : Qt.rgba(0.98, 0.98, 0.98, 0.95)
        border.width: 1
        border.color: toastType === "error" ? HusTheme.Primary.colorError :
                      toastType === "warning" ? HusTheme.Primary.colorWarning :
                      toastType === "success" ? HusTheme.Primary.colorSuccess : HusTheme.Primary.colorPrimaryBorder
        opacity: toastVisible ? 1.0 : 0.0
        visible: opacity > 0

        property bool toastVisible: false
        property string toastType: "info"
        property string toastMessage: ""

        Behavior on opacity {
            enabled: HusTheme.animationEnabled
            NumberAnimation { duration: 160; easing.type: Easing.OutQuad }
        }

        Timer {
            id: toastTimer
            interval: 3200
            onTriggered: toastBanner.toastVisible = false
        }

        function show(type, msg) {
            toastType = type;
            toastMessage = msg;
            toastVisible = true;
            toastTimer.restart();
        }

        RowLayout {
            id: toastContent
            anchors.centerIn: parent
            spacing: 8

            HusIconText {
                Layout.alignment: Qt.AlignVCenter
                iconSize: 14
                iconSource: toastBanner.toastType === "success" ? HusIcon.CheckCircleOutlined :
                            toastBanner.toastType === "error" ? HusIcon.CloseCircleOutlined :
                            toastBanner.toastType === "warning" ? HusIcon.ExclamationCircleOutlined : HusIcon.InfoCircleOutlined
                colorIcon: toastBanner.border.color
            }

            HusText {
                Layout.alignment: Qt.AlignVCenter
                text: toastBanner.toastMessage
                font.pixelSize: 12
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
            }
        }
    }

    Connections {
        target: HusTheme
        function onAnimationEnabledChanged() {
            if (!HusTheme.animationEnabled) {
                viewAnim.stop();
                stageStack.opacity = 1.0;
            }
        }
    }

    Connections {
        target: smmBackend
        function onNotification(type, message) {
            toastBanner.show(type, message);
        }
    }
}
