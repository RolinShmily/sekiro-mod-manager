pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HuskarUI.Basic

HusModal {
    id: rootModal

    width: 720
    title: qsTr("只狼游戏环境与 ModEngine 全景诊断")
    description: qsTr("SEKIRO ENVIRONMENT HEALTH DOCTOR · 引擎装配与环境健康透视")

    footerDelegate: Item {
        width: parent.width
        implicitHeight: footerRow.implicitHeight

        RowLayout {
            id: footerRow
            anchors.right: parent.right
            spacing: 8

            HusButton {
                text: qsTr("重新检测")
                onClicked: {
                    if (smmBackend) smmBackend.refreshDoctor();
                }
            }

            HusButton {
                type: HusButton.Type_Primary
                text: qsTr("完成")
                onClicked: rootModal.close()
            }
        }
    }

    bodyDelegate: ColumnLayout {
        width: parent.width
        spacing: 14

        // 综合健康评级横幅
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 64
            radius: HusTheme.Primary.radiusPrimary
            color: HusTheme.Primary.colorFillQuaternary
            border.width: 1
            border.color: HusTheme.Primary.colorBorderSecondary

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    HusText {
                        text: qsTr("综合健康评级")
                        font.pixelSize: 12
                        color: HusTheme.Primary.colorTextTertiary
                    }

                    RowLayout {
                        spacing: 8

                        HusTag {
                            id: statusBadge
                            readonly property string s: smmBackend ? smmBackend.healthOverall : ""
                            text: s === "healthy" ? qsTr("环境健康 (Healthy)") :
                                  s === "degraded" ? qsTr("配置提示 (Degraded)") :
                                  qsTr("需修复 (Action Required)")
                            colorText: s === "healthy" ? HusTheme.Primary.colorSuccess :
                                       s === "degraded" ? HusTheme.Primary.colorWarning :
                                       HusTheme.Primary.colorError
                        }

                        HusText {
                            text: smmBackend ? qsTr("(通过: %1 · 警告: %2 · 异常: %3)")
                                  .arg(smmBackend.healthOkCount)
                                  .arg(smmBackend.healthWarnCount)
                                  .arg(smmBackend.healthErrorCount) : ""
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextTertiary
                        }
                    }
                }

                HusButton {
                    type: HusButton.Type_Primary
                    text: qsTr("一键装配 / 修复 ModEngine")
                    onClicked: {
                        if (smmBackend) smmBackend.setupEngine();
                    }
                }
            }
        }

        // 诊断项列表
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 320
            clip: true

            ScrollBar.vertical: HusScrollBar {
                policy: ScrollBar.AsNeeded
            }

            ColumnLayout {
                width: parent.width
                spacing: 8

                Repeater {
                    model: smmBackend ? smmBackend.healthItems : []

                    delegate: Rectangle {
                        id: itemCard
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: itemCol.implicitHeight + 20
                        radius: HusTheme.Primary.radiusPrimary
                        color: HusTheme.Primary.colorFillQuaternary
                        border.width: 1
                        border.color: HusTheme.Primary.colorBorderSecondary

                        ColumnLayout {
                            id: itemCol
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                HusTag {
                                    text: {
                                        const st = (itemCard.modelData.status || "").toLowerCase();
                                        if (st === "ok" || st === "pass") return qsTr("通过");
                                        if (st === "warning" || st === "warn") return qsTr("警告");
                                        if (st === "info") return qsTr("提示");
                                        return qsTr("失败");
                                    }
                                    colorText: {
                                        const st = (itemCard.modelData.status || "").toLowerCase();
                                        if (st === "ok" || st === "pass") return HusTheme.Primary.colorSuccess;
                                        if (st === "warning" || st === "warn") return HusTheme.Primary.colorWarning;
                                        if (st === "info") return HusTheme.Primary.colorInfo;
                                        return HusTheme.Primary.colorError;
                                    }
                                }

                                HusText {
                                    text: itemCard.modelData.title || ""
                                    font.pixelSize: 13
                                    font.bold: true
                                    color: HusTheme.Primary.colorTextPrimary
                                }

                                HusTag {
                                    text: itemCard.modelData.category || ""
                                }

                                Item { Layout.fillWidth: true }
                            }

                            HusText {
                                Layout.fillWidth: true
                                text: itemCard.modelData.detail || ""
                                font.pixelSize: 12
                                color: HusTheme.Primary.colorTextSecondary
                                wrapMode: Text.WordWrap
                            }

                            // 修复建议
                            Rectangle {
                                Layout.fillWidth: true
                                visible: itemCard.modelData.remediation && itemCard.modelData.remediation.length > 0
                                implicitHeight: remedText.implicitHeight + 12
                                radius: 4
                                color: HusTheme.isDark ? Qt.rgba(0.96, 0.62, 0.04, 0.12) : Qt.rgba(0.96, 0.62, 0.04, 0.08)
                                border.width: 1
                                border.color: HusTheme.Primary.colorWarning

                                HusText {
                                    id: remedText
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    text: qsTr("修复建议: ") + (itemCard.modelData.remediation || "")
                                    font.pixelSize: 12
                                    color: HusTheme.Primary.colorWarning
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
