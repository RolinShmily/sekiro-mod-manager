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
            text: qsTr("导入 .smmpack")
            onClicked: importPackDialog.open()
        }

        HusButton {
            type: HusButton.Type_Primary
            text: qsTr("当前配置存为整合包")
            onClicked: rootView.requestSavePack()
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
        cellHeight: 236
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

            HusCard {
                anchors.fill: parent
                anchors.margins: 6
                radiusBg.all: HusTheme.Primary.radiusPrimaryLG
                colorBg: HusTheme.Primary.colorFillQuaternary
                borderBg.color: packWrapper.model.isEquipped
                                ? HusTheme.Primary.colorPrimary
                                : HusTheme.Primary.colorBorderSecondary

                title: packWrapper.model.name
                colorTitle: packWrapper.model.isEquipped
                            ? HusTheme.Primary.colorPrimary
                            : HusTheme.Primary.colorTextPrimary

                extraDelegate: HusTag {
                    anchors.verticalCenter: parent.verticalCenter
                    tagState: packWrapper.model.isEquipped ? HusTag.State_Success : HusTag.State_Default
                    text: packWrapper.model.isEquipped ? qsTr("已装配") : qsTr("就绪")
                }

                bodyDelegate: ColumnLayout {
                    spacing: 8

                    HusText {
                        Layout.fillWidth: true
                        text: packWrapper.model.description
                        font.pixelSize: 14
                        color: HusTheme.Primary.colorTextSecondary
                        elide: Text.ElideRight
                    }

                    HusText {
                        Layout.fillWidth: true
                        text: packWrapper.model.descriptionEn
                        font.pixelSize: 13
                        color: HusTheme.Primary.colorTextQuaternary
                        elide: Text.ElideRight
                    }
                }

                actionDelegate: Item {
                    implicitHeight: 46

                    HusDivider {
                        anchors.top: parent.top
                        width: parent.width
                    }

                    RowLayout {
                        anchors.top: parent.top
                        anchors.topMargin: 9
                        width: parent.width
                        spacing: 8

                        HusButton {
                            Layout.fillWidth: true
                            implicitHeight: 30
                            type: packWrapper.model.isEquipped ? HusButton.Type_Primary : HusButton.Type_Default
                            text: packWrapper.model.isEquipped ? qsTr("正在装配中") : qsTr("激活此整合包")
                            enabled: !packWrapper.model.isEquipped
                            onClicked: smmBackend.applyModPack(packWrapper.model.id)
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
                            implicitHeight: 30
                            text: qsTr("删除")
                            onClicked: {
                                if (smmBackend) smmBackend.deleteModPack(packWrapper.model.id);
                            }
                        }
                    }
                }
            }
        }
    }
}
