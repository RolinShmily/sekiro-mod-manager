pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import HuskarUI.Basic

HusModal {
    id: rootModal

    width: 520
    title: qsTr("保存为整合包预设")
    description: qsTr("将当前所有模组的启用状态与优先级顺位打包归档，方便日后随时切换。")

    onOpened: {
        packNameInput.text = "";
        packDescInput.text = "";
        packNameEnInput.text = "";
        packDescEnInput.text = "";
    }

    colorShadow: Qt.rgba(0, 0, 0, HusTheme.isDark ? 0.62 : 0.20)

    T.Overlay.modal: Item {
        Rectangle {
            anchors.fill: parent
            color: rootModal.colorOverlay
            opacity: rootModal.opacity
        }
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            hoverEnabled: true
            onWheel: (wheel) => wheel.accepted = true
        }
    }

    footerDelegate: Item {
        implicitHeight: 34
        width: parent.width

        RowLayout {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            HusButton {
                text: qsTr("取消")
                onClicked: rootModal.close()
            }

            HusButton {
                type: HusButton.Type_Primary
                text: qsTr("保存预设")
                enabled: packNameInput.text.trim() !== ""
                onClicked: {
                    if (smmBackend) {
                        smmBackend.saveModPack(
                            packNameInput.text.trim(),
                            packDescInput.text.trim(),
                            packNameEnInput.text.trim(),
                            packDescEnInput.text.trim()
                        );
                    }
                    rootModal.close();
                }
            }
        }
    }

    bodyDelegate: ColumnLayout {
        width: parent.width
        spacing: 14

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            HusText {
                text: qsTr("整合包名称 (中文)")
                font.pixelSize: 13
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
            }

            HusInput {
                id: packNameInput
                Layout.fillWidth: true
                placeholderText: qsTr("例如：剑圣孤影 · 断绝不死")
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            HusText {
                text: qsTr("预设描述 (中文)")
                font.pixelSize: 13
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
            }

            HusInput {
                id: packDescInput
                Layout.fillWidth: true
                placeholderText: qsTr("简要说明包含的核心玩法或视觉模组特点…")
            }
        }

        HusDivider {
            Layout.fillWidth: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            HusText {
                text: qsTr("英文名称与描述 (可选)")
                font.pixelSize: 12
                color: HusTheme.Primary.colorTextTertiary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                HusInput {
                    id: packNameEnInput
                    Layout.fillWidth: true
                    placeholderText: qsTr("Pack Name (English)")
                }

                HusInput {
                    id: packDescEnInput
                    Layout.fillWidth: true
                    placeholderText: qsTr("Description (English)")
                }
            }
        }
    }
}
