pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HuskarUI.Basic

Rectangle {
    id: navItem

    property string label: ""
    property int iconSource: 0
    property bool active: false

    signal clicked()

    implicitHeight: 42
    radius: HusTheme.Primary.radiusPrimary

    scale: navHover.pressed ? 0.985 : 1.0
    opacity: navHover.pressed ? 0.92 : 1.0

    Behavior on scale {
        NumberAnimation { duration: 80; easing.type: Easing.OutQuad }
    }
    Behavior on opacity {
        NumberAnimation { duration: 80; easing.type: Easing.OutQuad }
    }

    // 采用与主题沉浸融合的半透明层叠底色，彻底移除刺眼的高亮白光闪烁
    color: {
        if (navItem.active) {
            return HusTheme.isDark ? Qt.rgba(1, 1, 1, 0.09) : Qt.rgba(0, 0, 0, 0.06);
        }
        if (navHover.pressed) {
            return HusTheme.isDark ? Qt.rgba(1, 1, 1, 0.13) : Qt.rgba(0, 0, 0, 0.10);
        }
        if (navHover.containsMouse) {
            return HusTheme.isDark ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(0, 0, 0, 0.04);
        }
        return "transparent";
    }

    Behavior on color {
        ColorAnimation { duration: HusTheme.Primary.durationFast }
    }

    // 激活态：左侧强调色指示条
    Rectangle {
        visible: navItem.active
        width: 3
        height: 16
        radius: 1.5
        color: HusTheme.Primary.colorPrimary
        anchors.left: parent.left
        anchors.leftMargin: 3
        anchors.verticalCenter: parent.verticalCenter
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 12
        spacing: 10

        HusIconText {
            Layout.alignment: Qt.AlignVCenter
            iconSource: navItem.iconSource
            iconSize: 17
            colorIcon: navItem.active
                       ? HusTheme.Primary.colorPrimary
                       : HusTheme.Primary.colorTextTertiary
        }

        HusText {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            text: navItem.label
            font.pixelSize: 15
            font.bold: navItem.active
            color: navItem.active
                   ? HusTheme.Primary.colorPrimary
                   : HusTheme.Primary.colorTextSecondary
            elide: Text.ElideRight
        }
    }

    MouseArea {
        id: navHover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: navItem.clicked()
    }

    Accessible.role: Accessible.Button
    Accessible.name: navItem.label
}
