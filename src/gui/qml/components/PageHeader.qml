pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HuskarUI.Basic

/// 页面标题区：图标 + 标题 + 副标题 + 右侧操作。
///
/// 分页之后各页面只靠字号区分层级不够，统一的图标瓦片与强调色下划线让每个页面有明确身份，
/// 也把"这一页是什么"从文字里解放出来，靠形状与图标先被识别。
RowLayout {
    id: root

    property int iconSource: 0
    property string title: ""
    property string subtitle: ""
    property Component extra: null

    Layout.fillWidth: true
    spacing: 14

    // 图标瓦片：无边框的表面 + 主色图标，比纯文字标题更容易被扫读
    Rectangle {
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: 44
        implicitHeight: 44
        radius: HusTheme.Primary.radiusPrimaryLG
        color: HusTheme.Primary.colorFillSecondary
        border.width: 1
        border.color: HusTheme.Primary.colorBorderSecondary

        Behavior on color {
            enabled: HusTheme.animationEnabled
            ColorAnimation { duration: HusTheme.Primary.durationMid }
        }

        HusIconText {
            anchors.centerIn: parent
            iconSource: root.iconSource
            iconSize: 21
            colorIcon: HusTheme.Primary.colorPrimary
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
        spacing: 3

        HusText {
            Layout.fillWidth: true
            text: root.title
            font.pixelSize: 20
            font.bold: true
            color: HusTheme.Primary.colorTextPrimary
            elide: Text.ElideRight
        }

        HusText {
            Layout.fillWidth: true
            visible: root.subtitle !== ""
            text: root.subtitle
            font.pixelSize: 13
            color: HusTheme.Primary.colorTextTertiary
            wrapMode: Text.WordWrap
        }
    }

    Loader {
        Layout.alignment: Qt.AlignVCenter
        active: root.extra !== null
        sourceComponent: root.extra
    }
}
