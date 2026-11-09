pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HuskarUI.Basic

/// 设置/信息类页面的分组卡片。
///
/// 之前的分组只靠字号与一条分割线，长表单里很难判断"哪些选项属于同一件事"。
/// 卡片把一组的标题、说明、内容圈在同一个表面里，标题前再给一个图标，
/// 于是分组同时具备形状、颜色与图标三重线索，而不只是字体大小。
Item {
    id: root

    property int iconSource: 0
    property string title: ""
    property string description: ""
    /// 卡片内容（调用方直接写子项即可）。
    ///
    /// 别名不能叫 `contentData`：`Control` 已把它声明为 FINAL，QML 会拒绝覆盖，整个卡片
    /// 在 Qt 6.9+ 将无法实例化。
    default property alias bodyData: bodyColumn.data
    /// 右上角操作区。
    property Component extra: null
    /// 入场动画的序号：同页内按 45ms 递增形成轻微的错落。
    property int entranceIndex: 0
    property bool animateEntrance: true

    Layout.fillWidth: true
    implicitHeight: surface.implicitHeight

    // 真实高斯投影：MultiEffect 需要源纹理，因此把卡片表面作为 source，
    // 投影层压在下面、卡片本体再画一遍盖住中间区域。
    HusShadow {
        anchors.fill: surface
        source: surface
        shadowEnabled: true
        shadowOpacity: HusTheme.isDark ? 0.55 : 0.16
        shadowScale: 1.0
        shadowVerticalOffset: HusTheme.isDark ? 6 : 4
        shadowHorizontalOffset: 0
        shadowColor: HusTheme.isDark ? "#000000" : "#6b7280"
        z: -1

        Behavior on shadowOpacity {
            enabled: HusTheme.animationEnabled
            NumberAnimation { duration: HusTheme.Primary.durationMid }
        }
    }

    HusFrame {
        id: surface
        anchors.fill: parent
        implicitHeight: layoutColumn.implicitHeight + 40
        // 比页面底色再抬一档：深色下靠"更亮的半透明白"，浅色下靠纯白 + 细边框，
        // 这样即使没有投影也能读出卡片边界。
        colorBg: HusTheme.isDark ? Qt.rgba(1, 1, 1, 0.055) : Qt.rgba(1, 1, 1, 0.92)
        borderBg.color: HusTheme.isDark ? Qt.rgba(1, 1, 1, 0.10)
                                        : HusTheme.Primary.colorBorderSecondary
        borderBg.width: 1
        radiusBg.all: HusTheme.Primary.radiusPrimaryLG

        Behavior on colorBg {
            enabled: HusTheme.animationEnabled
            ColorAnimation { duration: HusTheme.Primary.durationMid }
        }
    }

    // 入场：淡入 + 轻微上浮，用 transform 而不是 y，避免触发布局重算
    opacity: animateEntrance && HusTheme.animationEnabled ? 0 : 1
    transform: Translate { id: slide; y: 0 }

    SequentialAnimation {
        id: entrance
        PauseAnimation { duration: root.entranceIndex * 45 }
        ParallelAnimation {
            NumberAnimation {
                target: root
                property: "opacity"
                from: 0
                to: 1
                duration: 200
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: slide
                property: "y"
                from: 10
                to: 0
                duration: 220
                easing.type: Easing.OutCubic
            }
        }
    }

    Component.onCompleted: {
        if (root.animateEntrance && HusTheme.animationEnabled) {
            entrance.start();
        } else {
            root.opacity = 1;
            slide.y = 0;
        }
    }

    ColumnLayout {
        id: layoutColumn
        anchors.left: surface.left
        anchors.right: surface.right
        anchors.top: surface.top
        anchors.margins: 20
        spacing: 14

        // ------------------------------- 卡片头 -------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: 32
                implicitHeight: 32
                radius: HusTheme.Primary.radiusPrimary
                color: HusTheme.Primary.colorFillTertiary
                visible: root.iconSource !== 0

                HusIconText {
                    anchors.centerIn: parent
                    iconSource: root.iconSource
                    iconSize: 16
                    colorIcon: HusTheme.Primary.colorPrimary
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

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
            spacing: 12
        }
    }
}
