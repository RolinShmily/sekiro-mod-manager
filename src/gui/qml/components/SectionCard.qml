pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HuskarUI.Basic

/// 设置/信息类页面的分组。
///
/// 分组不再画一层卡片表面：页面背景就是内容的背景，靠"图标 + 标题 + 细分隔线 + 留白"
/// 划分区域。之前的 surface + 投影在背景上又套了一个矩形，看起来像在页面里另开一块
/// DOM 区域；现在内容直接落在背景上，层级只由排版元素承担。
ColumnLayout {
    id: root

    property int iconSource: 0
    property string title: ""
    property string description: ""
    /// 分组内容（调用方直接写子项即可）。
    ///
    /// 别名不能叫 `contentData`：`Control` 已把它声明为 FINAL，QML 会拒绝覆盖。
    default property alias bodyData: bodyColumn.data
    /// 右上角操作区。
    property Component extra: null
    /// 分组内部的项间距。内容密集的页面（如设置页）调大它，比在子项里逐个加 spacer 干净。
    property int bodySpacing: 12

    Layout.fillWidth: true
    spacing: 12

    // ------------------------------- 分组标题 -------------------------------
    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        HusIconText {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 2
            iconSource: root.iconSource
            iconSize: 17
            colorIcon: HusTheme.Primary.colorPrimary
            visible: root.iconSource !== 0
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3

            HusText {
                Layout.fillWidth: true
                text: root.title
                font.pixelSize: 15
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
            }

            HusText {
                Layout.fillWidth: true
                visible: root.description !== ""
                text: root.description
                font.pixelSize: 12
                color: HusTheme.Primary.colorTextTertiary
                wrapMode: Text.WordWrap
            }
        }

        Loader {
            Layout.alignment: Qt.AlignTop
            active: root.extra !== null
            sourceComponent: root.extra
        }
    }

    HusDivider {
        Layout.fillWidth: true
        colorSplit: HusTheme.Primary.colorSplit
    }

    ColumnLayout {
        id: bodyColumn
        Layout.fillWidth: true
        spacing: root.bodySpacing
    }
}
