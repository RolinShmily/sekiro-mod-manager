pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HuskarUI.Basic
// The page composes the shared header/card components; the relative import keeps the QML
// runnable straight from the source tree (`smm_gui --dev`), not only from the module build.
import "../components"

/// 环境检测页。
///
/// 从弹窗改成分页之后，诊断不再打断用户的操作：检测结果、修复入口与"什么时候会检测"
/// 都留在同一页面上。检测项本身被压缩成五项，每项对应一个用户实际能处理的对象，
/// 而不是把 ini 的每个键、每个进程各拆成一行。
ColumnLayout {
    id: rootView
    objectName: "diagnosticsView"
    spacing: 14

    readonly property int contentWidth: 900
    readonly property string overall: smmBackend ? smmBackend.healthOverall : ""
    readonly property bool passed: overall === "healthy" && smmBackend.healthErrorCount === 0
                                   && smmBackend.healthWarnCount === 0
    readonly property bool unchecked: overall === ""

    function statusColor(status) {
        if (status === "ok") return HusTheme.Primary.colorSuccess;
        if (status === "warning") return HusTheme.Primary.colorWarning;
        if (status === "info") return HusTheme.Primary.colorInfo;
        return HusTheme.Primary.colorError;
    }

    function statusIcon(status) {
        if (status === "ok") return HusIcon.CheckCircleOutlined;
        if (status === "warning") return HusIcon.ExclamationCircleOutlined;
        if (status === "info") return HusIcon.InfoCircleOutlined;
        return HusIcon.CloseCircleOutlined;
    }

    function statusLabel(status) {
        if (status === "ok") return qsTr("通过");
        if (status === "warning") return qsTr("警告");
        if (status === "info") return qsTr("提示");
        return qsTr("异常");
    }

    // ---------------------------------- 页头 ----------------------------------
    PageHeader {
        iconSource: HusIcon.SafetyCertificateOutlined
        title: qsTr("环境检测")
        subtitle: qsTr("五项检查覆盖启动只狼的全部前提。只在打开启动器、点击「启动只狼」与本页「检测环境」时运行。")

        extra: Component {
            RowLayout {
                spacing: 10

                HusIconButton {
                    text: smmBackend.isDoctorChecking ? qsTr("检测中…") : qsTr("检测环境")
                    iconSource: HusIcon.ReloadOutlined
                    iconSize: 14
                    enabled: !smmBackend.isDoctorChecking && !smmBackend.isBusy
                    onClicked: smmBackend.refreshDoctor()
                }

                HusIconButton {
                    text: qsTr("一键装配 / 修复 ModEngine")
                    iconSource: HusIcon.ToolOutlined
                    iconSize: 14
                    type: HusButton.Type_Primary
                    enabled: !smmBackend.isDoctorChecking && !smmBackend.isBusy
                    onClicked: smmBackend.setupEngine()
                }
            }
        }
    }

    // ---------------------------------- 检测结果 ----------------------------------
    ScrollView {
        id: doctorScrollView
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.maximumWidth: rootView.contentWidth
        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
        contentWidth: width
        contentHeight: bodyColumn.implicitHeight
        clip: true

        ScrollBar.vertical: HusScrollBar {
            policy: ScrollBar.AsNeeded
        }

        ColumnLayout {
            id: bodyColumn
            width: parent.width - (doctorScrollView.ScrollBar.vertical.visible ? 12 : 4)
            spacing: 24

            // 综合评级
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: bannerColumn.implicitHeight + 32
                radius: HusTheme.Primary.radiusPrimaryLG
                color: HusTheme.Primary.colorFillQuaternary
                border.width: 1
                border.color: rootView.passed ? HusTheme.Primary.colorSuccessBorder
                                              : HusTheme.Primary.colorBorderSecondary

                ColumnLayout {
                    id: bannerColumn
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusTag {
                            text: smmBackend.isDoctorChecking ? qsTr("正在检测…")
                                  : rootView.unchecked ? qsTr("尚未检测")
                                  : rootView.passed ? qsTr("环境健康")
                                  : smmBackend.healthErrorCount > 0 ? qsTr("需修复")
                                  : qsTr("有提示")
                            colorText: smmBackend.isDoctorChecking || rootView.unchecked
                                       ? HusTheme.Primary.colorTextTertiary
                                       : rootView.passed ? HusTheme.Primary.colorSuccess
                                       : smmBackend.healthErrorCount > 0 ? HusTheme.Primary.colorError
                                       : HusTheme.Primary.colorWarning
                        }

                        HusText {
                            text: qsTr("通过 %1 · 警告 %2 · 异常 %3")
                                  .arg(smmBackend.healthOkCount)
                                  .arg(smmBackend.healthWarnCount)
                                  .arg(smmBackend.healthErrorCount)
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextTertiary
                        }

                        Item { Layout.fillWidth: true }

                        HusTag {
                            text: smmBackend.isElevated ? qsTr("管理员权限") : qsTr("未提权")
                            colorText: smmBackend.isElevated ? HusTheme.Primary.colorSuccess
                                                             : HusTheme.Primary.colorWarning
                        }
                    }

                    HusText {
                        Layout.fillWidth: true
                        text: smmBackend.healthSummary
                        font.pixelSize: 13
                        color: HusTheme.Primary.colorTextPrimary
                        wrapMode: Text.WordWrap
                    }

                    HusText {
                        Layout.fillWidth: true
                        visible: text !== ""
                        text: smmBackend.healthRemediation
                        font.pixelSize: 12
                        color: HusTheme.Primary.colorWarning
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // 检测项
            SectionCard {
                iconSource: HusIcon.UnorderedListOutlined
                title: qsTr("检测项")
                description: qsTr("每项对应一个可以直接处理的对象；异常项会带出修复建议。")

                HusText {
                    Layout.fillWidth: true
                    visible: rootView.unchecked && !smmBackend.isDoctorChecking
                    text: qsTr("还没有检测结果，点击右上角「检测环境」立即检查。")
                    font.pixelSize: 12
                    color: HusTheme.Primary.colorTextTertiary
                    wrapMode: Text.WordWrap
                }

                Repeater {
                    model: smmBackend ? smmBackend.healthItems : []

                    delegate: ColumnLayout {
                        id: itemRow
                        required property var modelData
                        required property int index

                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            HusIconText {
                                Layout.alignment: Qt.AlignVCenter
                                iconSource: rootView.statusIcon(itemRow.modelData.status)
                                iconSize: 14
                                colorIcon: rootView.statusColor(itemRow.modelData.status)
                            }

                            HusText {
                                Layout.alignment: Qt.AlignVCenter
                                text: itemRow.modelData.title || ""
                                font.pixelSize: 13
                                font.bold: true
                                color: HusTheme.Primary.colorTextPrimary
                            }

                            HusTag {
                                Layout.alignment: Qt.AlignVCenter
                                text: rootView.statusLabel(itemRow.modelData.status)
                                colorText: rootView.statusColor(itemRow.modelData.status)
                            }

                            Item { Layout.fillWidth: true }
                        }

                        HusText {
                            Layout.fillWidth: true
                            Layout.leftMargin: 22
                            text: itemRow.modelData.detail || ""
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextSecondary
                            wrapMode: Text.WordWrap
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 22
                            visible: (itemRow.modelData.remediation || "").length > 0
                                     && itemRow.modelData.status !== "ok"
                            implicitHeight: remediationText.implicitHeight + 14
                            radius: HusTheme.Primary.radiusPrimary
                            color: HusTheme.isDark ? Qt.rgba(0.96, 0.62, 0.04, 0.12)
                                                   : Qt.rgba(0.96, 0.62, 0.04, 0.08)
                            border.width: 1
                            border.color: HusTheme.Primary.colorWarningBorder

                            HusText {
                                id: remediationText
                                anchors.fill: parent
                                anchors.margins: 7
                                text: qsTr("修复建议：") + (itemRow.modelData.remediation || "")
                                font.pixelSize: 12
                                color: HusTheme.Primary.colorWarning
                                wrapMode: Text.WordWrap
                            }
                        }

                        HusDivider {
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            visible: itemRow.index < (smmBackend.healthItems.length - 1)
                        }
                    }
                }
            }

            // 检测时机
            SectionCard {
                iconSource: HusIcon.ClockCircleOutlined
                title: qsTr("什么时候检测")
                description: qsTr("诊断是只读操作，不做轮询：没有打开游戏也没有点击检测时，SMM 不会为了诊断去扫描磁盘或进程。")

                Repeater {
                    model: [
                        qsTr("打开启动器时检测一次，结果留在本页与侧栏状态卡。"),
                        qsTr("点击「启动只狼」时重新检测一次，通过后才启动游戏。"),
                        qsTr("在本页点击「检测环境」随时重新检测。")
                    ]

                    delegate: RowLayout {
                        id: timingRow
                        required property string modelData

                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            Layout.alignment: Qt.AlignVCenter
                            implicitWidth: 5
                            implicitHeight: 5
                            radius: 2.5
                            color: HusTheme.Primary.colorPrimary
                        }

                        HusText {
                            Layout.fillWidth: true
                            text: timingRow.modelData
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextSecondary
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }
}
