pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HuskarUI.Basic

/// 设置项：左侧标签与说明，右侧控件。
///
/// 两列都随页面宽度伸缩，但各有边界：标签列封顶，宽窗口下说明不会被拉得离控件很远；控件列
/// 吃掉剩余宽度，路径输入框因此能显示更长的路径。窄窗口下两列都保留最小宽度，控件不会被压成
/// 一条缝。分组本身不画卡片表面，标签与控件直接落在页面背景上（见 SectionCard）。
RowLayout {
    id: root

    property string title: ""
    property string description: ""
    /// 右侧控件列的内容；调用方直接写子项即可。
    ///
    /// 别名避开 `contentData`：部分 Qt Quick Controls 基类把该名字声明为 FINAL。
    default property alias bodyData: controlColumn.data
    /// 标签列的最小 / 最大宽度。
    property int labelMinWidth: 200
    property int labelMaxWidth: 320
    /// 控件列的最小 / 最大宽度：窄窗口下优先压标签；超宽窗口下给输入框一个上限，
    /// 剩余宽度留在行尾，不再继续拉长控件。
    property int controlMinWidth: 300
    property int controlMaxWidth: 900
    /// 两列间距。
    property int gap: 28

    Layout.fillWidth: true
    spacing: root.gap

    ColumnLayout {
        Layout.fillWidth: true
        Layout.preferredWidth: root.labelMaxWidth
        Layout.minimumWidth: root.labelMinWidth
        Layout.maximumWidth: root.labelMaxWidth
        Layout.alignment: Qt.AlignTop
        spacing: 4

        HusText {
            Layout.fillWidth: true
            text: root.title
            font.pixelSize: 13
            font.bold: true
            color: HusTheme.Primary.colorTextPrimary
            wrapMode: Text.WordWrap
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

    ColumnLayout {
        id: controlColumn
        Layout.fillWidth: true
        Layout.preferredWidth: root.controlMinWidth
        Layout.minimumWidth: root.controlMinWidth
        Layout.maximumWidth: root.controlMaxWidth
        Layout.alignment: Qt.AlignTop
        spacing: 8
    }
}
