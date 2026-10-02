pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import HuskarUI.Basic

HusDrawer {
    id: rootDrawer

    implicitWidth: 500
    title: smmBackend ? smmBackend.currentModName : ""

    function getDomainLabel(url) {
        if (!url) return "";
        const u = url.toLowerCase();
        if (u.includes("nexusmods")) return "Nexus Mods";
        if (u.includes("github")) return "GitHub";
        if (u.includes("bilibili")) return "Bilibili";
        return qsTr("官方/发布源");
    }

    FileDialog {
        id: imageFileDialog
        title: qsTr("选择模组预览背景图")
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: {
            if (selectedFile) {
                const path = selectedFile.toString().replace("file:///", "");
                smmBackend.setModPreview(smmBackend.currentModId, path);
            }
        }
    }

    HusModal {
        id: confirmDeleteModal
        width: 420
        title: qsTr("确认删除模组？")
        description: qsTr("删除后该模组及其所有资产文件将从暂存区中永久抹除，此操作不可逆。")
        confirmText: qsTr("确认删除")
        cancelText: qsTr("取消")
        onConfirm: {
            if (smmBackend) {
                smmBackend.deleteMod(smmBackend.currentModId);
                rootDrawer.close();
            }
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 18

        // ------------------------------ 预览背景图 ------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            RowLayout {
                Layout.fillWidth: true

                HusText {
                    Layout.fillWidth: true
                    text: qsTr("预览背景图")
                    font.pixelSize: 13
                    font.bold: true
                    color: HusTheme.Primary.colorTextPrimary
                }

                HusTag {
                    Layout.alignment: Qt.AlignVCenter
                    text: qsTr("随 .smmpack 打包")
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 140
                radius: HusTheme.Primary.radiusPrimary
                clip: true
                color: HusTheme.Primary.colorFillQuaternary
                border.width: 1
                border.color: HusTheme.Primary.colorBorderSecondary

                Image {
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    source: {
                        if (!smmBackend || !smmBackend.currentModPreview) return "";
                        const sDir = smmBackend.stagingDir.replace(/\\/g, "/");
                        return "file:///" + sDir + "/" + smmBackend.currentModId + "/" + smmBackend.currentModPreview;
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 1.0; color: Qt.rgba(0.0, 0.0, 0.0, 0.72) }
                    }
                }

                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10

                    HusText {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        text: smmBackend && smmBackend.currentModPreview !== ""
                              ? smmBackend.currentModPreview
                              : qsTr("暂无预览图")
                        font.pixelSize: 11
                        color: "#ffffff"
                        elide: Text.ElideMiddle
                    }

                    HusButton {
                        implicitHeight: 28
                        type: HusButton.Type_Primary
                        text: qsTr("更换图片")
                        onClicked: imageFileDialog.open()
                    }
                }
            }
        }

        HusDivider {
            Layout.fillWidth: true
        }

        // ------------------------------ 模组信息 ------------------------------
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 6
            columnSpacing: 16

            HusText {
                text: qsTr("模组 ID")
                font.pixelSize: 11
                color: HusTheme.Primary.colorTextTertiary
            }

            HusText {
                Layout.fillWidth: true
                text: smmBackend ? smmBackend.currentModId : ""
                font.pixelSize: 12
                color: HusTheme.Primary.colorTextPrimary
                elide: Text.ElideRight
            }

            HusText {
                text: qsTr("作者")
                font.pixelSize: 11
                color: HusTheme.Primary.colorTextTertiary
            }

            HusText {
                Layout.fillWidth: true
                text: smmBackend ? smmBackend.currentModAuthor : ""
                font.pixelSize: 12
                color: HusTheme.Primary.colorTextPrimary
                elide: Text.ElideRight
            }
        }

        // ------------------------------ 快捷操作条 ------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            HusButton {
                Layout.fillWidth: true
                implicitHeight: 28
                text: qsTr("打开目录")
                onClicked: {
                    if (smmBackend) smmBackend.openModFolder(smmBackend.currentModId);
                }
            }

            HusButton {
                Layout.fillWidth: true
                implicitHeight: 28
                text: qsTr("导出 (.zip)")
                onClicked: {
                    if (smmBackend) smmBackend.exportSingleMod(smmBackend.currentModId);
                }
            }

            HusButton {
                implicitHeight: 28
                text: qsTr("删除模组")
                onClicked: confirmDeleteModal.open()
            }
        }

        // ------------------------------ 发布与来源链接 ------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            RowLayout {
                Layout.fillWidth: true

                HusText {
                    Layout.fillWidth: true
                    text: qsTr("发布与来源链接")
                    font.pixelSize: 13
                    font.bold: true
                    color: HusTheme.Primary.colorTextPrimary
                }

                HusTag {
                    Layout.alignment: Qt.AlignVCenter
                    text: rootDrawer.getDomainLabel(smmBackend ? smmBackend.currentModSourceUrl : "")
                    visible: smmBackend && smmBackend.currentModSourceUrl !== ""
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                HusInput {
                    id: sourceUrlInput
                    Layout.fillWidth: true
                    implicitHeight: 30
                    text: smmBackend ? smmBackend.currentModSourceUrl : ""
                    placeholderText: qsTr("输入 Nexus Mods / GitHub / Bilibili 等发布网址…")
                }

                HusButton {
                    implicitHeight: 30
                    text: qsTr("前往")
                    visible: sourceUrlInput.text.trim() !== ""
                    onClicked: {
                        if (smmBackend) smmBackend.openUrl(sourceUrlInput.text.trim());
                    }
                }

                HusButton {
                    implicitHeight: 30
                    text: qsTr("保存")
                    onClicked: {
                        if (smmBackend) smmBackend.setModSourceUrl(smmBackend.currentModId, sourceUrlInput.text.trim());
                    }
                }
            }
        }

        HusDivider {
            Layout.fillWidth: true
        }

        // ------------------------------ 资产启停 ------------------------------
        RowLayout {
            Layout.fillWidth: true

            HusText {
                Layout.fillWidth: true
                text: qsTr("资产文件启停")
                font.pixelSize: 13
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
            }

            HusTag {
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("%1 / %2 已激活")
                      .arg(smmBackend ? smmBackend.activeAssetCount : 0)
                      .arg(smmBackend ? smmBackend.totalAssetCount : 0)
            }
        }

        HusInput {
            id: assetSearchInput
            Layout.fillWidth: true
            implicitHeight: 28
            placeholderText: qsTr("搜索资产文件名或路径…")
        }

        ListView {
            id: assetListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            model: smmBackend ? smmBackend.assetListModel : null

            ScrollBar.vertical: HusScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Rectangle {
                id: assetRow
                required property var model
                required property int index

                readonly property bool matchesFilter: assetSearchInput.text.trim() === "" ||
                                                      assetRow.model.relativePath.toLowerCase().indexOf(assetSearchInput.text.trim().toLowerCase()) !== -1

                visible: matchesFilter
                width: assetListView.width - 12
                implicitHeight: matchesFilter ? 54 : 0
                height: implicitHeight
                radius: HusTheme.Primary.radiusPrimary
                color: HusTheme.Primary.colorFillQuaternary
                border.width: 1
                border.color: HusTheme.Primary.colorBorderSecondary
                opacity: assetRow.model.enabled ? 1.0 : 0.55

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    HusTag {
                        Layout.alignment: Qt.AlignVCenter
                        text: assetRow.model.category
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        HusText {
                            Layout.fillWidth: true
                            text: assetRow.model.relativePath
                            font.pixelSize: 12
                            color: HusTheme.Primary.colorTextPrimary
                            elide: Text.ElideMiddle
                        }

                        HusText {
                            text: assetRow.model.formattedSize
                            font.pixelSize: 11
                            color: HusTheme.Primary.colorTextQuaternary
                        }
                    }

                    HusSwitch {
                        Layout.alignment: Qt.AlignVCenter
                        checked: assetRow.model.enabled
                        onCheckedChanged: {
                            if (checked !== assetRow.model.enabled)
                                smmBackend.assetListModel.toggleAsset(assetRow.index);
                        }
                    }
                }
            }
        }
    }
}
