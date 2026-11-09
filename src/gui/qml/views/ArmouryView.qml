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
    readonly property var rankOptions: {
        const count = smmBackend ? smmBackend.modListModel.count : 1;
        const options = [];
        for (let rank = 1; rank <= Math.max(1, count); ++rank)
            options.push({ label: qsTr("第 %1 顺位").arg(rank), value: rank });
        return options;
    }

    /// 是否处于筛选/搜索状态：空态文案要区分"暂存区为空"和"筛选无结果"。
    readonly property bool isFiltering: smmBackend !== null
        && (smmBackend.modListModel.filterText.trim().length > 0
            || smmBackend.modListModel.selectedCategory !== "all"
            || smmBackend.modListModel.onlyEquippedPackMods)

    /// 首次扫描尚未返回：此时列表为空并不代表暂存区为空。
    readonly property bool firstScanPending: !smmBackend || smmBackend.healthOverall === ""

    /// 由 Main.qml 的 Ctrl+F 调用。
    function focusSearch() {
        modSearchInput.forceActiveFocus();
        modSearchInput.selectAll();
    }

    function clearFilters() {
        modSearchInput.text = "";
        if (smmBackend) {
            smmBackend.modListModel.filterText = "";
            smmBackend.modListModel.selectedCategory = "all";
            smmBackend.modListModel.onlyEquippedPackMods = false;
        }
    }

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

    signal requestOpenDrawer(string modId)
    signal requestSavePack()

    FileDialog {
        id: archiveDialog
        title: qsTr("选择要导入的模组压缩包（支持多选批量导入）")
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Mod Archives (*.zip *.7z *.rar)", "All Files (*.*)"]
        onAccepted: {
            // The backend converts dialog urls to local paths; QML does not re-implement it.
            if (selectedFiles && selectedFiles.length > 0) {
                smmBackend.importUrls(selectedFiles);
            }
        }
    }

    // ---------------------------------- 工具栏第一行：标题与全局操作 ----------------------------------
    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        ColumnLayout {
            spacing: 4

            HusText {
                text: qsTr("模组管理")
                font.pixelSize: 20
                font.bold: true
                color: HusTheme.Primary.colorTextPrimary
            }

            HusText {
                text: qsTr("调整裁决顺位，或进入详情精细控制单个资产文件。")
                font.pixelSize: 13
                color: HusTheme.Primary.colorTextTertiary
            }
        }

        Item { Layout.fillWidth: true }

        HusButton {
            visible: smmBackend && smmBackend.hasEquippedPack
            implicitHeight: 32
            type: (smmBackend && smmBackend.modListModel.onlyEquippedPackMods)
                  ? HusButton.Type_Primary
                  : HusButton.Type_Default
            text: (smmBackend && smmBackend.modListModel.onlyEquippedPackMods)
                  ? qsTr("显示全部暂存区模组")
                  : qsTr("仅显示整合包模组")
            onClicked: {
                if (smmBackend) {
                    smmBackend.modListModel.onlyEquippedPackMods = !smmBackend.modListModel.onlyEquippedPackMods;
                }
            }
        }

        HusButton {
            implicitHeight: 32
            text: qsTr("当前配置存为整合包")
            onClicked: rootView.requestSavePack()
        }

        HusButton {
            implicitHeight: 32
            type: rootView.batchMode ? HusButton.Type_Primary : HusButton.Type_Default
            text: rootView.batchMode ? qsTr("退出批量") : qsTr("批量管理")
            onClicked: {
                rootView.batchMode = !rootView.batchMode;
                if (!rootView.batchMode) rootView.clearSelection();
            }
        }

        HusButton {
            implicitHeight: 32
            type: HusButton.Type_Primary
            text: qsTr("导入压缩包")
            onClicked: archiveDialog.open()
        }
    }

    // ---------------------------------- 工具栏第二行：搜索与视图控制 ----------------------------------
    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        HusInput {
            id: modSearchInput
            objectName: "modSearchInput"
            Layout.preferredWidth: 240
            implicitHeight: 30
            placeholderText: qsTr("搜索模组、作者… (Ctrl+F)")
            clearEnabled: "active"
            onTextChanged: searchDebounce.restart()

            Timer {
                id: searchDebounce
                interval: 120
                onTriggered: {
                    if (smmBackend) smmBackend.modListModel.filterText = modSearchInput.text;
                }
            }
        }

        HusText {
            Layout.alignment: Qt.AlignVCenter
            text: qsTr("%1 个").arg(smmBackend ? smmBackend.modListModel.count : 0)
            font.pixelSize: 12
            color: HusTheme.Primary.colorTextQuaternary
        }

        Item { Layout.fillWidth: true }

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
    }

    // ---------------------------------- 工具栏第三行：分类胶囊 ----------------------------------
    // 用 Flow 换行而不是横向 Flickable：分类最多十来个，全部可见比藏在横向滚动里更好发现，
    // 也避免"新分类把内容推到末尾"的滚动状态问题。
    Flow {
        id: categoryFlow
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        spacing: 6

        Repeater {
            model: smmBackend ? smmBackend.modListModel.availableCategories : ["all"]

            delegate: HusButton {
                id: catBtn
                required property string modelData
                implicitHeight: 30
                type: (smmBackend && smmBackend.modListModel.selectedCategory === catBtn.modelData)
                      ? HusButton.Type_Primary
                      : HusButton.Type_Default
                text: (smmBackend ? smmBackend.categoryLabel(catBtn.modelData) : catBtn.modelData)
                onClicked: {
                    if (smmBackend) {
                        smmBackend.modListModel.selectedCategory = catBtn.modelData;
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
                implicitHeight: 30
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
                implicitHeight: 30
                enabled: rootView.selectedModIds.length > 0
                text: qsTr("批量启用")
                onClicked: smmBackend.setModsEnabled(rootView.selectedModIds, true)
            }

            HusButton {
                implicitHeight: 30
                enabled: rootView.selectedModIds.length > 0
                text: qsTr("批量禁用")
                onClicked: smmBackend.setModsEnabled(rootView.selectedModIds, false)
            }

            HusButton {
                implicitHeight: 30
                enabled: rootView.selectedModIds.length > 0
                text: qsTr("批量删除 (%1)").arg(rootView.selectedModIds.length)
                // 删除是危险操作：红色文字提示风险，但不抢走主按钮的位置。
                colorText: enabled ? HusTheme.Primary.colorError : HusTheme.Primary.colorTextDisabled
                onClicked: batchDeleteConfirmModal.open()
            }
        }
    }

    // ---------------------------------- 空状态 ----------------------------------
    // 列表为空有三种完全不同的原因，不能都用同一句话解释：
    //   1. 首次扫描还没回来  -> 显示加载，而不是"没有模组"
    //   2. 搜索/筛选没有命中 -> 给出"清除筛选"的出口
    //   3. 暂存区确实为空    -> 引导去配置目录或导入
    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: !smmBackend || smmBackend.modListModel.count === 0
        spacing: 0

        Item { Layout.fillHeight: true }

        // 1) 首次扫描中
        ColumnLayout {
            Layout.fillWidth: true
            visible: rootView.firstScanPending
            spacing: 12

            HusSpin {
                Layout.alignment: Qt.AlignHCenter
                spinSize: 26
                indicatorItemCount: 4
            }

            HusText {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("正在读取暂存区…")
                font.pixelSize: 13
                color: HusTheme.Primary.colorTextTertiary
            }
        }

        // 2) 筛选无结果
        ColumnLayout {
            Layout.fillWidth: true
            visible: !rootView.firstScanPending && rootView.isFiltering
            spacing: 12

            HusEmpty {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                imageStyle: HusEmpty.Style_Simple
                description: qsTr("没有符合当前筛选条件的模组。换一个关键词，或清除筛选后再看。")
            }

            HusButton {
                Layout.alignment: Qt.AlignHCenter
                type: HusButton.Type_Primary
                text: qsTr("清除筛选条件")
                onClicked: rootView.clearFilters()
            }
        }

        // 3) 暂存区为空
        ColumnLayout {
            Layout.fillWidth: true
            visible: !rootView.firstScanPending && !rootView.isFiltering
            spacing: 12

            HusEmpty {
                Layout.fillWidth: true
                Layout.preferredHeight: 170
                imageStyle: HusEmpty.Style_Default
                description: qsTr("尚未纳管任何模组。请先在全局配置中指定游戏目录与暂存区，然后导入模组压缩包。")
            }

            HusButton {
                Layout.alignment: Qt.AlignHCenter
                type: HusButton.Type_Primary
                text: qsTr("导入模组压缩包")
                onClicked: archiveDialog.open()
            }
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
        cellHeight: rootView.viewMode === "grid" ? 184 : 64
        rightMargin: rootView.scrollBarSpace
        model: smmBackend ? smmBackend.modListModel : null

        // 模型 moveRow() 走 beginMoveRows/endMoveRows，这两个过渡把重排动画补上
        move: Transition {
            enabled: HusTheme.animationEnabled
            NumberAnimation {
                properties: "x,y"
                duration: 180
                easing.type: Easing.OutCubic
            }
        }
        moveDisplaced: Transition {
            enabled: HusTheme.animationEnabled
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
                rankModel: rootView.rankOptions
                previewImagePath: {
                    const source = cardWrapper.model.previewImagePath;
                    const revision = smmBackend ? smmBackend.previewRevision : 0;
                    return source ? source + "?rev=" + revision : "";
                }
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
                    id: batchDeleteConfirmButton
                    colorBg: batchDeleteConfirmButton.active
                             ? Qt.darker(HusTheme.Primary.colorError, 1.15)
                             : batchDeleteConfirmButton.hovered
                               ? Qt.lighter(HusTheme.Primary.colorError, 1.08)
                               : HusTheme.Primary.colorError
                    colorText: "#ffffff"
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
