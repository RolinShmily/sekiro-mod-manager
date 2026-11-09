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

    // 页面顺序与导航栏一致；StackLayout 的下标由它推导，避免嵌套三元表达式随页面增加而失控。
    readonly property var navOrder: ["hub", "armoury", "packs", "settings", "about"]
    property string currentNav: "hub"

    /// 后台任务进行中（部署/导入/扫描/诊断），用于底部状态条
    readonly property bool backendBusy: smmBackend ? (smmBackend.isBusy || smmBackend.isDoctorChecking) : false

    /// Ctrl+F 要聚焦搜索框：视图可能尚未加载完成，短暂重试而不是静默失败
    function focusCurrentViewSearch() {
        const view = stageStack.currentItem;
        if (view && view.focusSearch) {
            view.focusSearch();
            return true;
        }
        return false;
    }

    // =========================================================================
    // 全局快捷键：页面切换、搜索、重新诊断（可在帮助提示中看到）
    // =========================================================================
    Shortcut { sequence: "Ctrl+1"; onActivated: mainWindow.currentNav = "hub" }
    Shortcut { sequence: "Ctrl+2"; onActivated: mainWindow.currentNav = "armoury" }
    Shortcut { sequence: "Ctrl+3"; onActivated: mainWindow.currentNav = "packs" }
    Shortcut { sequence: "Ctrl+4"; onActivated: mainWindow.currentNav = "settings" }
    Shortcut { sequence: "Ctrl+,"; onActivated: mainWindow.currentNav = "settings" }
    Shortcut {
        sequence: "Ctrl+R"
        onActivated: if (smmBackend && !smmBackend.isBusy) smmBackend.refreshDoctor()
    }
    Shortcut {
        sequence: "Ctrl+F"
        onActivated: {
            mainWindow.currentNav = "armoury";
            searchFocusTimer.attempts = 0;
            searchFocusTimer.start();
        }
    }
    Timer {
        id: searchFocusTimer
        interval: 60
        repeat: true
        property int attempts: 0
        onTriggered: {
            attempts++;
            if (mainWindow.focusCurrentViewSearch() || attempts >= 8) {
                stop();
            }
        }
    }

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

                NavItem {
                    Layout.fillWidth: true
                    label: qsTr("全局配置")
                    iconSource: HusIcon.SettingOutlined
                    active: mainWindow.currentNav === "settings"
                    onClicked: mainWindow.currentNav = "settings"
                }

                NavItem {
                    Layout.fillWidth: true
                    label: qsTr("关于")
                    iconSource: HusIcon.InfoCircleOutlined
                    active: mainWindow.currentNav === "about"
                    onClicked: mainWindow.currentNav = "about"
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

                        HusText {
                            Layout.fillWidth: true
                            text: smmBackend.isDoctorChecking ? qsTr("检查中…") :
                                  smmBackend.healthOverall === "healthy" ? qsTr("诊断通过") :
                                  qsTr("%1 项异常 · %2 项警告").arg(smmBackend.healthErrorCount).arg(smmBackend.healthWarnCount)
                            font.pixelSize: 13
                            font.bold: true
                            wrapMode: Text.WordWrap
                            color: smmBackend.canLaunch ? HusTheme.Primary.colorSuccess : HusTheme.Primary.colorWarning
                        }
                        HusText {
                            Layout.fillWidth: true
                            visible: smmBackend.healthOverall !== "healthy"
                            text: smmBackend.healthSummary
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                            maximumLineCount: 4
                            elide: Text.ElideRight
                            color: HusTheme.Primary.colorTextSecondary
                        }
                        HusButton {
                            Layout.fillWidth: true
                            text: qsTr("查看诊断")
                            onClicked: doctorModal.open()
                        }

                        Repeater {
                            model: [
                                {
                                    k: qsTr("ModEngine 钩子"),
                                    v: smmBackend.isDoctorChecking ? qsTr("检查中") : smmBackend.engineReady ? qsTr("已就绪") : qsTr("需处理"),
                                    ok: smmBackend.engineReady
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

            }
        }

        // ------------------------------ 内容舞台 ------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 20

        StackLayout {
            id: stageStack
            anchors.fill: parent
            currentIndex: Math.max(0, mainWindow.navOrder.indexOf(mainWindow.currentNav))

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
                    onRequestDiagnosis: doctorModal.open()
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

            Loader {
                objectName: "settingsLoader"
                property bool visited: false
                active: visited || StackLayout.isCurrentItem
                asynchronous: true
                onLoaded: {
                    visited = true;
                    if (StackLayout.isCurrentItem) stageStack.animatePage();
                }
                sourceComponent: SettingsView {}
            }

            Loader {
                objectName: "aboutLoader"
                property bool visited: false
                active: visited || StackLayout.isCurrentItem
                asynchronous: true
                onLoaded: {
                    visited = true;
                    if (StackLayout.isCurrentItem) stageStack.animatePage();
                }
                sourceComponent: AboutView {}
            }
        }

        // 后台任务徽标：只在忙碌时浮在内容右下角，不占用布局空间，也不推动列表。
        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 8
            z: 500
            visible: mainWindow.backendBusy
            implicitHeight: 30
            implicitWidth: busyRow.implicitWidth + 26
            radius: 15
            color: HusTheme.isDark ? Qt.rgba(0.12, 0.12, 0.15, 0.92)
                                   : Qt.rgba(0.98, 0.98, 0.98, 0.95)
            border.width: 1
            border.color: HusTheme.Primary.colorPrimaryBorder

            RowLayout {
                id: busyRow
                anchors.centerIn: parent
                spacing: 8

                HusSpin {
                    Layout.alignment: Qt.AlignVCenter
                    spinSize: 13
                    indicatorItemCount: 4
                }

                HusText {
                    Layout.alignment: Qt.AlignVCenter
                    text: smmBackend && smmBackend.statusText !== ""
                          ? smmBackend.statusText
                          : qsTr("处理中…")
                    font.pixelSize: 12
                    color: HusTheme.Primary.colorTextSecondary
                }
            }
        }
        }
    }

    ModDrawer {
        id: modDrawer
    }

    DoctorModal {
        id: doctorModal
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
        // 长错误信息不再撑出屏幕：限制最大宽度并换行，最多三行。
        implicitHeight: Math.max(36, toastContent.implicitHeight + 18)
        implicitWidth: Math.min(toastContent.implicitWidth + 34, mainWindow.width - 160)
        radius: 18
        Accessible.role: Accessible.AlertMessage
        Accessible.name: toastMessage
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

        // 错误/警告需要阅读时间：按严重程度决定停留时长。
        readonly property int toastDuration: toastType === "error" ? 7000
                                          : toastType === "warning" ? 5500
                                          : 3200

        Timer {
            id: toastTimer
            interval: toastBanner.toastDuration
            onTriggered: toastBanner.toastVisible = false
        }

        function show(type, msg) {
            toastType = type;
            toastMessage = msg;
            toastVisible = true;
            toastTimer.restart();
        }

        // 点击可立即关闭，不必等它自动消失。
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: toastBanner.toastVisible = false
        }

        RowLayout {
            id: toastContent
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.topMargin: 9
            anchors.bottomMargin: 9
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
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: toastBanner.toastMessage
                font.pixelSize: 12
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
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
        function onLaunchBlocked() { doctorModal.open(); }
        function onNotification(type, message) {
            toastBanner.show(type, message);
        }
    }
}
