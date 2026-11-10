pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HuskarUI.Basic

Rectangle {
    id: rootCard

    property string modId: ""
    property string modName: ""
    property string modVersion: ""
    property string modAuthor: ""
    property string modCategory: ""
    property string modDesc: ""
    property string modSourceUrl: ""
    property int modPriority: 10
    property bool modEnabled: true
    property int modRank: 1
    property int rankCount: 1
    property var rankModel: []
    property string previewImagePath: ""
    property bool showBackdrop: true
    property bool compact: false
    property bool batchMode: false
    property bool isSelected: false

    readonly property bool hasBackdrop: !rootCard.compact && rootCard.showBackdrop && rootCard.previewImagePath !== ""

    signal toggleActive(bool active)
    signal requestDetails()
    signal prioritySelected(int targetRank)
    signal dragEnded(real finalX, real finalY)
    signal toggleSelected()

    readonly property bool isDragging: (rootCard.compact ? compactGripArea.drag.active : gripArea.drag.active)

    // 拖动时的静态反馈：放大与变淡都是即时状态，不做过渡。
    scale: isDragging ? 1.025 : 1.0
    opacity: isDragging ? 0.90 : 1.0

    function getDomainInfo(url) {
        if (!url) return { label: "", icon: "", color: "" };
        const u = url.toLowerCase();
        if (u.includes("nexusmods.com")) {
            return { label: "Nexus", icon: "qrc:/images/icon_nexus.png", color: "#da8e35" };
        }
        if (u.includes("gamebanana.com")) {
            return { label: "Banana", icon: "qrc:/images/icon_banana.png", color: "#facc15" };
        }
        if (u.includes("3dmgame.com")) {
            return { label: "3DM", icon: "qrc:/images/icon_3dm.png", color: "#ef4444" };
        }
        if (u.includes("bilibili.com") || u.includes("b23.tv")) {
            return { label: "Bilibili", icon: "qrc:/images/icon_bilibili.svg", color: "#00aeec" };
        }
        if (u.includes("github.com")) {
            return { label: "GitHub", icon: "qrc:/images/icon_github.svg", color: "#cbd5e1" };
        }
        return { label: qsTr("来源"), icon: "", color: HusTheme.Primary.colorPrimary };
    }

    implicitHeight: rootCard.compact ? 52 : 156
    radius: rootCard.compact ? HusTheme.Primary.radiusPrimary : HusTheme.Primary.radiusPrimaryLG
    clip: true
    color: HusTheme.Primary.colorFillQuaternary
    border.width: rootCard.isSelected ? 2 : 1
    border.color: rootCard.isSelected
                  ? HusTheme.Primary.colorPrimary
                  : (cardMouseArea.containsMouse
                     ? HusTheme.Primary.colorPrimaryBorder
                     : HusTheme.Primary.colorBorderSecondary)

    // -------------------------------------------------------------------------
    // 批量管理复选框：优先采用 HuskarUI 原生 HusCheckBox 组件
    // -------------------------------------------------------------------------
    HusCheckBox {
        id: selectionBox
        visible: rootCard.batchMode
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: rootCard.compact ? 4 : 6
        z: 50
        checked: rootCard.isSelected
        onClicked: rootCard.toggleSelected()
    }

    // -------------------------------------------------------------------------
    // 卡片鼠标 Hover 边框高亮
    // -------------------------------------------------------------------------
    MouseArea {
        id: cardMouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        z: 0
    }

    // -------------------------------------------------------------------------
    // 键盘可达：Tab 可聚焦，Enter/Space 打开详情（与 NavItem 保持一致的模式）。
    // 批量模式下焦点留给复选框，避免和选择操作抢按键。
    // -------------------------------------------------------------------------
    activeFocusOnTab: !rootCard.batchMode

    Keys.onReturnPressed: rootCard.requestDetails()
    Keys.onEnterPressed: rootCard.requestDetails()
    Keys.onSpacePressed: rootCard.requestDetails()

    Accessible.role: Accessible.Grouping
    Accessible.name: rootCard.modName
    Accessible.description: rootCard.modEnabled
                            ? qsTr("已启用，顺位 %1").arg(rootCard.modRank)
                            : qsTr("已禁用，顺位 %1").arg(rootCard.modRank)

    Rectangle {
        anchors.fill: parent
        radius: rootCard.radius
        color: "transparent"
        border.width: rootCard.activeFocus ? 2 : 0
        border.color: HusTheme.Primary.colorPrimary
        z: 100
    }

    // -------------------------------------------------------------------------
    // 沉浸式预览背景（严格等比例裁剪居中 PreserveAspectCrop，杜绝画面拉伸形变）
    // -------------------------------------------------------------------------
    Item {
        anchors.fill: parent
        clip: true
        visible: rootCard.hasBackdrop
        z: 0

        Image {
            id: backdropImage
            anchors.fill: parent
            source: rootCard.hasBackdrop ? rootCard.previewImagePath : ""
            sourceSize.width: 480
            sourceSize.height: 270
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            autoTransform: true
            mipmap: true
            smooth: true
            opacity: cardMouseArea.containsMouse ? 0.35 : 0.22
        }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: HusTheme.isDark ? Qt.rgba(0.08, 0.08, 0.10, 0.60) : Qt.rgba(0.98, 0.98, 0.98, 0.70)
                }
                GradientStop {
                    position: 1.0
                    color: HusTheme.isDark ? Qt.rgba(0.08, 0.08, 0.10, 0.90) : Qt.rgba(0.98, 0.98, 0.98, 0.92)
                }
            }
        }
    }

    // 前景文字色：深色模式且有背景图时使用白字；浅色模式下始终使用深色主题文字，保障清晰对比度
    readonly property color textPrimary: (HusTheme.isDark && rootCard.hasBackdrop) ? "#ffffff" : HusTheme.Primary.colorTextPrimary
    readonly property color textSecondary: (HusTheme.isDark && rootCard.hasBackdrop) ? Qt.rgba(1, 1, 1, 0.82) : HusTheme.Primary.colorTextSecondary
    readonly property color textTertiary: (HusTheme.isDark && rootCard.hasBackdrop) ? Qt.rgba(1, 1, 1, 0.60) : HusTheme.Primary.colorTextTertiary

    // -------------------------------------------------------------------------
    // 内容：紧凑列表行 (compact === true)
    // -------------------------------------------------------------------------
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 12
        visible: rootCard.compact
        z: 1

        HusIconText {
            Layout.alignment: Qt.AlignVCenter
            iconSource: HusIcon.HolderOutlined
            iconSize: 14
            colorIcon: rootCard.textTertiary
            opacity: compactGripArea.containsMouse || compactGripArea.drag.active ? 1.0 : 0.60

            MouseArea {
                id: compactGripArea
                anchors.fill: parent
                anchors.margins: -10
                hoverEnabled: true
                cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                drag.target: rootCard
                drag.threshold: 4
                drag.smoothed: false

                property bool wasDragging: false
                onPressed: wasDragging = false
                onPositionChanged: {
                    if (drag.active)
                        wasDragging = true;
                }
                onReleased: {
                    if (wasDragging || drag.active) {
                        wasDragging = false;
                        rootCard.dragEnded(rootCard.x, rootCard.y);
                    }
                }
            }
        }

        HusTag {
            Layout.alignment: Qt.AlignVCenter
            text: rootCard.modRank === 1
                  ? qsTr("顺位 1 · P%1").arg(rootCard.modPriority)
                  : qsTr("顺位 %1 · P%2").arg(rootCard.modRank).arg(rootCard.modPriority)
        }

        HusSwitch {
            Layout.alignment: Qt.AlignVCenter
            checked: rootCard.modEnabled
            onToggled: rootCard.toggleActive(checked)
        }

        HusText {
            Layout.fillWidth: true
            Layout.maximumWidth: 260
            Layout.alignment: Qt.AlignVCenter
            text: rootCard.modName
            font.pixelSize: 14
            font.bold: true
            color: rootCard.textPrimary
            elide: Text.ElideRight

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: rootCard.requestDetails()
            }
        }

        HusTag {
            Layout.alignment: Qt.AlignVCenter
            text: (smmBackend ? smmBackend.categoryLabel(rootCard.modCategory) : rootCard.modCategory)
        }

        HusText {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            text: `${rootCard.modAuthor} · v${rootCard.modVersion}`
            font.pixelSize: 12
            color: rootCard.textTertiary
            elide: Text.ElideRight
        }

        HusSelect {
            implicitWidth: 120
            implicitHeight: 28
            textRole: "label"
            model: rootCard.rankModel
            currentIndex: Math.max(0, Math.min(rootCard.modRank - 1, rootCard.rankCount - 1))
            onActivated: (index) => rootCard.prioritySelected(index + 1)
        }

        HusButton {
            implicitHeight: 28
            type: HusButton.Type_Text
            text: qsTr("详情")
            onClicked: rootCard.requestDetails()
        }
    }

    // -------------------------------------------------------------------------
    // 内容：完整卡片 (compact === false)
    // -------------------------------------------------------------------------
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 13
        spacing: 7
        visible: !rootCard.compact
        z: 1

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // 拖拽手柄图标（提示该卡片可拖拽重排，支持抓取拖拽）
            HusIconText {
                id: dragGrip
                Layout.alignment: Qt.AlignVCenter
                iconSource: HusIcon.HolderOutlined
                iconSize: 14
                colorIcon: rootCard.textTertiary
                opacity: gripArea.containsMouse || gripArea.drag.active ? 1.0 : 0.60

                MouseArea {
                    id: gripArea
                    anchors.fill: parent
                    anchors.margins: -10
                    hoverEnabled: true
                    cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    drag.target: rootCard
                    drag.threshold: 4
                    drag.smoothed: false

                    property bool wasDragging: false

                    onPressed: wasDragging = false
                    onPositionChanged: {
                        if (drag.active)
                            wasDragging = true;
                    }

                    onReleased: {
                        if (wasDragging || drag.active) {
                            wasDragging = false;
                            rootCard.dragEnded(rootCard.x, rootCard.y);
                        }
                    }
                }
            }

            HusText {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: rootCard.modName
                font.pixelSize: 15
                font.bold: true
                color: rootCard.textPrimary
                elide: Text.ElideRight

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: rootCard.requestDetails()
                }
            }

            HusTag {
                Layout.alignment: Qt.AlignVCenter
                text: rootCard.modRank === 1
                      ? qsTr("顺位 1 · P%1").arg(rootCard.modPriority)
                      : qsTr("顺位 %1 · P%2").arg(rootCard.modRank).arg(rootCard.modPriority)
            }

            HusSwitch {
                Layout.alignment: Qt.AlignVCenter
                checked: rootCard.modEnabled
                onToggled: rootCard.toggleActive(checked)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            HusText {
                Layout.alignment: Qt.AlignVCenter
                text: `${rootCard.modAuthor} · v${rootCard.modVersion}`
                font.pixelSize: 12
                color: rootCard.textTertiary
            }

            HusTag {
                Layout.alignment: Qt.AlignVCenter
                text: (smmBackend ? smmBackend.categoryLabel(rootCard.modCategory) : rootCard.modCategory)
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                visible: rootCard.modSourceUrl !== ""
                spacing: 4
                readonly property var dInfo: rootCard.getDomainInfo(rootCard.modSourceUrl)

                Image {
                    Layout.preferredWidth: 13
                    Layout.preferredHeight: 13
                    Layout.alignment: Qt.AlignVCenter
                    source: parent.dInfo.icon
                    visible: parent.dInfo.icon !== ""
                }

                HusButton {
                    implicitHeight: 24
                    type: HusButton.Type_Text
                    text: parent.dInfo.icon !== "" ? parent.dInfo.label : ("🌐 " + parent.dInfo.label)
                    font.pixelSize: 12
                    onClicked: Qt.openUrlExternally(rootCard.modSourceUrl)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // 规整等比例微缩预览图
            Rectangle {
                visible: rootCard.previewImagePath !== ""
                implicitWidth: 64
                implicitHeight: 40
                radius: 4
                clip: true
                color: HusTheme.Primary.colorFillTertiary
                border.width: 1
                border.color: HusTheme.Primary.colorBorderSecondary

                Image {
                    anchors.fill: parent
                    source: !rootCard.compact ? rootCard.previewImagePath : ""
                    sourceSize.width: 128
                    sourceSize.height: 80
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    cache: true
                    autoTransform: true
                    mipmap: true
                    smooth: true
                }
            }

            HusText {
                Layout.fillWidth: true
                text: rootCard.modDesc
                font.pixelSize: 13
                lineHeight: 1.25
                color: rootCard.textSecondary
                elide: Text.ElideRight
                maximumLineCount: 2
                wrapMode: Text.WordWrap
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            HusText {
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("裁决顺位")
                font.pixelSize: 12
                color: rootCard.textTertiary
            }

            HusSelect {
                implicitWidth: 124
                implicitHeight: 28
                textRole: "label"
                model: rootCard.rankModel
                currentIndex: Math.max(0, Math.min(rootCard.modRank - 1, rootCard.rankCount - 1))
                onActivated: (index) => rootCard.prioritySelected(index + 1)
            }

            Item { Layout.fillWidth: true }

            HusButton {
                implicitHeight: 28
                type: HusButton.Type_Text
                text: qsTr("详情")
                font.pixelSize: 13
                onClicked: rootCard.requestDetails()
            }
        }
    }
}
