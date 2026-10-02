pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import HuskarUI.Basic
import "../components"

ColumnLayout {
    id: rootView
    spacing: 14

    property string viewMode: "grid" // "grid" | "list"
    property bool showBackdrop: true
    property bool batchMode: false
    property var selectedModIds: []

    function isModSelected(id) {
        return selectedModIds.indexOf(id) >= 0;
    }

    function toggleModSelected(id) {
        var arr = selectedModIds.slice();
        var idx = arr.indexOf(id);
        if (idx >= 0) {
            arr.splice(idx, 1);
        } else {
            arr.push(id);
        }
        selectedModIds = arr;
    }

    function selectAll() {
        if (smmBackend) {
            selectedModIds = smmBackend.modListModel.allModIds();
        }
    }

    function clearSelection() {
        selectedModIds = [];
    }

    /// 右侧为滚动条预留的宽度，避免卡片压在滚动条下方
    readonly property int scrollBarSpace: 12

    function getCategoryLabel(cat) {
        if (!cat || cat === "all") return qsTr("全部模组");
        const c = cat.toLowerCase();
        if (c === "weapon_skin" || c === "weapon" || c === "parts") return qsTr("武器外观");
        if (c === "character_skin" || c === "chr") return qsTr("人物外观");
        if (c === "gameplay_overhaul" || c === "param") return qsTr("玩法重构");
        if (c === "ui" || c === "menu") return qsTr("界面增强");
        if (c === "animation") return qsTr("动作招式");
        if (c === "audio" || c === "sound") return qsTr("音效音乐");
        if (c === "vfx" || c === "sfx") return qsTr("特效光影");
        if (c === "map") return qsTr("地图场景");
        if (c === "script") return qsTr("脚本扩展");
        if (c === "loader") return qsTr("前置钩子");
        if (c === "general" || c === "other") return qsTr("其它扩展");
        return cat.toUpperCase();
    }

    signal requestOpenDrawer(string modId)

    FileDialog {
        id: archiveDialog
        title: qsTr("选择要导入的模组压缩包")
        nameFilters: ["Mod Archives (*.zip *.7z *.rar)"]
        onAccepted: {
            if (selectedFile) {
                const path = selectedFile.toString().replace("file:///", "");
                smmBackend.importArchive(path);
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
                text: qsTr("模组管理")
                font.pixelSize: 20
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
            }

            HusText {
                text: qsTr("调整裁决顺位，或进入详情精细控制单个资产文件。")
                font.pixelSize: 14
                color: HusTheme.Primary.colorTextTertiary
            }
        }

        Item { Layout.fillWidth: true }

        HusInput {
            implicitWidth: 210
            placeholderText: qsTr("搜索模组、作者…")
            onTextChanged: {
                if (smmBackend)
                    smmBackend.modListModel.filterText = text;
            }
        }

        HusSegmented {
            id: viewModeSegmented
            options: [
                {
                    label: qsTr("卡片"),
                    value: "grid",
                    iconSource: HusIcon.AppstoreOutlined
                },
                {
                    label: qsTr("列表"),
                    value: "list",
                    iconSource: HusIcon.BarsOutlined
                }
            ]
            currentIndex: 0
            onCurrentIndexChanged: {
                rootView.viewMode = (currentIndex === 0 ? "grid" : "list");
            }
        }

        HusSwitch {
            text: qsTr("背景图")
            checked: rootView.showBackdrop
            onCheckedChanged: rootView.showBackdrop = checked
        }

        HusButton {
            type: rootView.batchMode ? HusButton.Type_Primary : HusButton.Type_Default
            text: rootView.batchMode ? qsTr("退出批量") : qsTr("批量管理")
            onClicked: {
                rootView.batchMode = !rootView.batchMode;
                if (!rootView.batchMode) rootView.clearSelection();
            }
        }

        HusButton {
            type: HusButton.Type_Primary
            text: qsTr("导入压缩包")
            onClicked: archiveDialog.open()
        }
    }

    // ---------------------------------- 分类过滤胶囊栏 ----------------------------------
    Flickable {
        id: categoryFlickable
        Layout.fillWidth: true
        Layout.preferredHeight: 32
        contentWidth: categoryRow.implicitWidth
        contentHeight: 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        RowLayout {
            id: categoryRow
            spacing: 6

            Repeater {
                model: smmBackend ? smmBackend.modListModel.availableCategories : ["all"]

                delegate: HusButton {
                    id: catBtn
                    required property string modelData
                    implicitHeight: 28
                    type: (smmBackend && smmBackend.modListModel.selectedCategory === catBtn.modelData)
                          ? HusButton.Type_Primary
                          : HusButton.Type_Default
                    text: rootView.getCategoryLabel(catBtn.modelData)
                    onClicked: {
                        if (smmBackend) {
                            smmBackend.modListModel.selectedCategory = catBtn.modelData;
                        }
                    }
                }
            }
        }
    }

    // ---------------------------------- 批量操作浮动工具条 ----------------------------------
    Rectangle {
        visible: rootView.batchMode && smmBackend && smmBackend.modListModel.count > 0
        Layout.fillWidth: true
        implicitHeight: 44
        radius: HusTheme.Primary.radiusPrimary
        color: HusTheme.isDark ? Qt.rgba(0.18, 0.18, 0.22, 0.85) : Qt.rgba(0.94, 0.94, 0.96, 0.95)
        border.color: HusTheme.Primary.colorPrimary
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 12

            HusButton {
                implicitHeight: 28
                text: (rootView.selectedModIds.length === (smmBackend ? smmBackend.modListModel.count : 0))
                      ? qsTr("全不选") : qsTr("全选")
                onClicked: {
                    if (rootView.selectedModIds.length === (smmBackend ? smmBackend.modListModel.count : 0)) {
                        rootView.clearSelection();
                    } else {
                        rootView.selectAll();
                    }
                }
            }

            HusText {
                text: qsTr("已选择 %1 / %2 项").arg(rootView.selectedModIds.length).arg(smmBackend ? smmBackend.modListModel.count : 0)
                font.pixelSize: 13
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
            }

            Item { Layout.fillWidth: true }

            HusButton {
                implicitHeight: 28
                enabled: rootView.selectedModIds.length > 0
                text: qsTr("批量启用")
                onClicked: smmBackend.setModsEnabled(rootView.selectedModIds, true)
            }

            HusButton {
                implicitHeight: 28
                enabled: rootView.selectedModIds.length > 0
                text: qsTr("批量禁用")
                onClicked: smmBackend.setModsEnabled(rootView.selectedModIds, false)
            }

            HusButton {
                implicitHeight: 28
                enabled: rootView.selectedModIds.length > 0
                type: HusButton.Type_Primary
                text: qsTr("批量删除 (%1)").arg(rootView.selectedModIds.length)
                onClicked: batchDeleteConfirmModal.open()
            }
        }
    }

    // ---------------------------------- 空状态 ----------------------------------
    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: !smmBackend || smmBackend.modListModel.count === 0
        spacing: 0

        Item { Layout.fillHeight: true }

        HusEmpty {
            Layout.fillWidth: true
            Layout.preferredHeight: 170
            imageStyle: HusEmpty.Style_Default
            description: qsTr("尚未纳管任何模组。请先在全局配置中指定游戏目录与暂存区，然后导入模组压缩包。")
        }

        HusButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 16
            type: HusButton.Type_Primary
            text: qsTr("导入模组压缩包")
            onClicked: archiveDialog.open()
        }

        Item { Layout.fillHeight: true }
    }

    // ---------------------------------- 模组网格 ----------------------------------
    GridView {
        id: modGrid
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        visible: smmBackend && smmBackend.modListModel.count > 0
        cellWidth: rootView.viewMode === "grid"
                   ? Math.max(1, (modGrid.width - rootView.scrollBarSpace) / 2)
                   : Math.max(1, modGrid.width - rootView.scrollBarSpace)
        cellHeight: rootView.viewMode === "grid" ? 180 : 66
        rightMargin: rootView.scrollBarSpace
        model: smmBackend ? smmBackend.modListModel : null

        // 模型 moveRow() 走 beginMoveRows/endMoveRows，这两个过渡把重排动画补上
        move: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: 180
                easing.type: Easing.OutCubic
            }
        }
        moveDisplaced: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: 180
                easing.type: Easing.OutCubic
            }
        }

        ScrollBar.vertical: HusScrollBar {
            policy: ScrollBar.AsNeeded
        }

        delegate: Item {
            id: cardWrapper
            required property var model
            required property int index

            width: modGrid.cellWidth
            height: modGrid.cellHeight
            z: card.isDragging ? 9999 : 1

            ModCard {
                id: card
                width: cardWrapper.width - 10
                height: cardWrapper.height - 10
                x: 5
                y: 5

                modId: cardWrapper.model.id
                modName: cardWrapper.model.name
                modVersion: cardWrapper.model.version
                modAuthor: cardWrapper.model.author
                modCategory: cardWrapper.model.category
                modDesc: cardWrapper.model.description
                modSourceUrl: cardWrapper.model.sourceUrl || ""
                modPriority: cardWrapper.model.priority
                modEnabled: cardWrapper.model.enabled
                modRank: cardWrapper.model.rank
                rankCount: smmBackend ? smmBackend.modListModel.count : 1
                previewImagePath: cardWrapper.model.previewImagePath
                showBackdrop: rootView.showBackdrop
                compact: rootView.viewMode === "list"
                batchMode: rootView.batchMode
                isSelected: rootView.isModSelected(cardWrapper.model.id)

                onToggleSelected: rootView.toggleModSelected(cardWrapper.model.id)
                onToggleActive: (active) => smmBackend.modListModel.toggleEnabled(cardWrapper.index)
                onRequestDetails: rootView.requestOpenDrawer(cardWrapper.model.id)
                onPrioritySelected: (targetRank) => smmBackend.modListModel.swapPriority(cardWrapper.index, targetRank - 1)

                onDragEnded: (finalX, finalY) => {
                    const cols = Math.max(1, Math.floor(modGrid.width / modGrid.cellWidth));
                    const globalX = cardWrapper.x + finalX;
                    const globalY = cardWrapper.y + finalY;
                    const col = Math.max(0, Math.min(cols - 1, Math.floor((globalX + card.width / 2) / modGrid.cellWidth)));
                    const row = Math.max(0, Math.floor((globalY + card.height / 2) / modGrid.cellHeight));
                    const total = smmBackend ? smmBackend.modListModel.count : 1;
                    const target = Math.max(0, Math.min(total - 1, row * cols + col));
                    if (target !== cardWrapper.index) {
                        smmBackend.modListModel.moveRow(cardWrapper.index, target);
                    }
                    card.x = 5;
                    card.y = 5;
                }
            }
        }
    }

    // ---------------------------------- 批量删除确认弹窗 ----------------------------------
    HusModal {
        id: batchDeleteConfirmModal
        implicitWidth: 480
        title: qsTr("确认批量删除模组")
        description: qsTr("此操作将从暂存目录彻底删除选中的 %1 个模组文件，无法撤销。您确定要继续吗？").arg(rootView.selectedModIds.length)

        footerDelegate: Item {
            implicitHeight: 34
            width: parent.width

            RowLayout {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10

                HusButton {
                    text: qsTr("取消")
                    onClicked: batchDeleteConfirmModal.close()
                }

                HusButton {
                    type: HusButton.Type_Primary
                    text: qsTr("彻底删除")
                    onClicked: {
                        if (smmBackend) {
                            smmBackend.deleteMods(rootView.selectedModIds);
                            rootView.clearSelection();
                        }
                        batchDeleteConfirmModal.close();
                    }
                }
            }
        }
    }
}
