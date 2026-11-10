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

    // 固定在布局中的高度：导航项被压缩会让命中区域与绘制区域分离。
    Layout.preferredHeight: implicitHeight
    Layout.minimumHeight: implicitHeight

    // 按压时的静态反馈：缩小与变淡立即生效，不做过渡。
    scale: navHover.pressed ? 0.985 : 1.0
    opacity: navHover.pressed ? 0.92 : 1.0

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

    activeFocusOnTab: true
    Keys.onSpacePressed: clicked()
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: navItem.radius
        border.width: navItem.activeFocus ? 2 : 0
        border.color: HusTheme.Primary.colorPrimary
    }

    Accessible.role: Accessible.Button
    Accessible.name: navItem.label
}
