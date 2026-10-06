pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import HuskarUI.Basic

ColumnLayout {
    id: rootView
    spacing: 14

    /// 右侧为滚动条预留的宽度
    readonly property int scrollBarSpace: 12

    signal requestSavePack()

    FileDialog {
        id: importPackDialog
        title: qsTr("选择导入 .smmpack 整合包")
        nameFilters: ["SMM Pack (*.smmpack)"]
        onAccepted: {
            if (selectedFile) {
                const path = selectedFile.toString().replace("file:///", "");
                smmBackend.importModPack(path);
            }
        }
    }

    FileDialog {
        id: exportPackDialog
        title: qsTr("导出 .smmpack 整合包")
        fileMode: FileDialog.SaveFile
        nameFilters: ["SMM Pack (*.smmpack)"]
        property string targetPresetId: ""
        onAccepted: {
            if (selectedFile && targetPresetId !== "") {
                const path = selectedFile.toString().replace("file:///", "");
                smmBackend.exportModPack(targetPresetId, path);
            }
        }
    }

    // ---------------------------------- 工具栏 ----------------------------------
    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        ColumnLayout {
            spacing: 2

            HusText {
                text: qsTr("整合包预设")
                font.pixelSize: 20
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
            }

            HusText {
                text: qsTr("把当前模组的启用状态与裁决顺位打包为一体化预设，一键切换整套配装。")
                font.pixelSize: 14
                color: HusTheme.Primary.colorTextTertiary
            }
        }

        Item { Layout.fillWidth: true }

        HusButton {
            type: HusButton.Type_Primary
            text: qsTr("导入 .smmpack")
            onClicked: importPackDialog.open()
        }
    }

    // ---------------------------------- 空状态 ----------------------------------
    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: !smmBackend || smmBackend.presetListModel.count === 0
        spacing: 0

        Item { Layout.fillHeight: true }

        HusEmpty {
            Layout.fillWidth: true
            Layout.preferredHeight: 170
            imageStyle: HusEmpty.Style_Default
            description: qsTr("尚无整合包预设。将当前配置存为整合包后，即可一键切换整套配装方案。")
        }

        HusButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 16
            type: HusButton.Type_Primary
            text: qsTr("当前配置存为整合包")
            onClicked: rootView.requestSavePack()
        }

        Item { Layout.fillHeight: true }
    }

    // ---------------------------------- 整合包列表 ----------------------------------
    GridView {
        id: packGrid
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        visible: smmBackend && smmBackend.presetListModel.count > 0
        cellWidth: Math.max(1, (packGrid.width - rootView.scrollBarSpace) / 2)
        cellHeight: 156
        rightMargin: rootView.scrollBarSpace
        model: smmBackend ? smmBackend.presetListModel : null

        ScrollBar.vertical: HusScrollBar {
            policy: ScrollBar.AsNeeded
        }

        delegate: Item {
            id: packWrapper
            required property var model
            required property int index

            width: packGrid.cellWidth
            height: packGrid.cellHeight

            Rectangle {
                anchors.fill: parent
                anchors.margins: 6
                radius: HusTheme.Primary.radiusPrimaryLG
                color: HusTheme.Primary.colorFillQuaternary
                border.width: packWrapper.model.isEquipped ? 2 : 1
                border.color: packWrapper.model.isEquipped
                              ? HusTheme.Primary.colorPrimary
                              : (cardHover.containsMouse ? HusTheme.Primary.colorPrimaryBorder : HusTheme.Primary.colorBorderSecondary)

                Behavior on border.color {
                    enabled: HusTheme.animationEnabled
                    ColorAnimation { duration: HusTheme.Primary.durationFast }
                }

                MouseArea {
                    id: cardHover
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    // 顶部标题与状态标签
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusIconText {
                            iconSource: HusIcon.InboxOutlined
                            iconSize: 17
                            colorIcon: packWrapper.model.isEquipped ? HusTheme.Primary.colorPrimary : HusTheme.Primary.colorTextSecondary
                        }

                        HusText {
                            Layout.fillWidth: true
                            text: packWrapper.model.name
                            font.pixelSize: 16
                            font.bold: true
                            color: packWrapper.model.isEquipped ? HusTheme.Primary.colorPrimary : HusTheme.Primary.colorTextPrimary
                            elide: Text.ElideRight
                        }

                        HusTag {
                            Layout.alignment: Qt.AlignVCenter
                            text: qsTr("包含 %1 个模组").arg(packWrapper.model.modCount)
                        }

                        HusTag {
                            Layout.alignment: Qt.AlignVCenter
                            tagState: packWrapper.model.isEquipped ? HusTag.State_Success : HusTag.State_Default
                            text: packWrapper.model.isEquipped ? qsTr("已装配") : qsTr("就绪")
                        }
                    }

                    // 中间说明文本（若空则提供典雅的占位说明，彻底杜绝大面积空洞）
                    HusText {
                        Layout.fillWidth: true
                        text: {
                            if (packWrapper.model.description && packWrapper.model.description.trim() !== "")
                                return packWrapper.model.description;
                            if (packWrapper.model.descriptionEn && packWrapper.model.descriptionEn.trim() !== "")
                                return packWrapper.model.descriptionEn;
                            return qsTr("已保存的整合包预设方案，支持一键载入整套模组顺位与启用状态。");
                        }
                        font.pixelSize: 13
                        color: (packWrapper.model.description && packWrapper.model.description.trim() !== "")
                               ? HusTheme.Primary.colorTextSecondary
                               : HusTheme.Primary.colorTextTertiary
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        wrapMode: Text.WordWrap
                    }

                    Item { Layout.fillHeight: true }

                    // 底部操作按钮栏
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        HusButton {
                            Layout.fillWidth: true
                            implicitHeight: 32
                            visible: !packWrapper.model.isEquipped
                            type: HusButton.Type_Primary
                            text: qsTr("激活此整合包")
                            onClicked: smmBackend.applyModPack(packWrapper.model.id)
                        }

                        HusButton {
                            Layout.fillWidth: true
                            implicitHeight: 32
                            visible: packWrapper.model.isEquipped
                            type: HusButton.Type_Default
                            text: qsTr("取消整合包应用")
                            onClicked: smmBackend.deactivateModPack()
                        }

                        HusButton {
                            implicitHeight: 32
                            text: qsTr("导出")
                            onClicked: {
                                exportPackDialog.targetPresetId = packWrapper.model.id;
                                exportPackDialog.currentFile = "file:///" + (packWrapper.model.name ? (packWrapper.model.name + ".smmpack") : "SekiroModPack.smmpack");
                                exportPackDialog.open();
                            }
                        }

                        HusButton {
                            id: deletePackBtn
                            implicitHeight: 32
                            text: qsTr("删除")
                            onClicked: deleteConfirmPop.open()

                            HusPopconfirm {
                                id: deleteConfirmPop
                                width: 230
                                x: -width + deletePackBtn.width
                                y: -height - 6
                                title: qsTr("确认删除此整合包？")
                                description: qsTr("确定要删除预设「%1」吗？此操作不可撤销。").arg(packWrapper.model.name)
                                confirmText: qsTr("删除")
                                cancelText: qsTr("取消")
                                onConfirm: {
                                    if (smmBackend) smmBackend.deleteModPack(packWrapper.model.id);
                                    deleteConfirmPop.close();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
