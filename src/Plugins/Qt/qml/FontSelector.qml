// FontSelector.qml — 字体选择器正文。全部由 Phase 0 原子板块拼装
// （DialogShell / SelectableList / EnumCombo / PreviewPane / DialogButtons），
// 交互经 fontBridge（C++ FontSelectorBridge）转发到 scheme facade（spec 句柄）。
// 详见 record/qml/font-selector.md Phase 3。
//
// context property（C++ 注入）：fontBridge、closeBridge、dpScale、isDark、specsKey。
// fontBridge 的 setter 返回 {preview, styles/families} 等联动结果，QML 在同一
// handler 更新 model + 预览，省二次往返。单一防抖 Timer 汇聚预览刷新。

import QtQuick
import "." // DialogShell / SelectableList / EnumCombo / PreviewPane / DialogButtons / Theme

DialogShell {
    id: root
    implicitWidth: 980
    implicitHeight: 600
    onCancel: () => fontBridge.cancel()

    // 预览 data URL。各 setter 返回新 preview 时更新；防抖 Timer 汇聚手动拉取。
    property string previewUrl: fontBridge.requestPreview()
    // 固定 UI 文案（scheme 翻译注入）。
    property var labels: fontBridge.uiLabels()
    // 选项卡：右侧面板 Filter / Advanced 切换（整合，不开第二个 exec）。
    property string activeTab: "filter"

    content: Item {
        // 布局：上区 = 左三栏 + 右选项卡面板；下区 = 左预览图 + 右按钮组。
        // 选项卡整合 Filter 与 Advanced（不再用整页翻转）。

        // ---- 上区 ----
        Item {
            id: topArea
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 300 * Theme.scaleFactor

            // 左：三栏 family/style/size（缩窄）。
            Row {
                id: columns3
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 444 * Theme.scaleFactor
                spacing: 12 * Theme.scaleFactor

                Column {
                    width: 200 * Theme.scaleFactor
                    height: parent.height
                    spacing: 6 * Theme.scaleFactor
                    Text { text: root.labels.family; color: Theme.fg; font.bold: true
                           font.pixelSize: 14 * Theme.scaleFactor }
                    SelectableList {
                        width: parent.width; height: parent.height - 20 * Theme.scaleFactor
                        items: familyModel.value
                        currentValue: fontBridge.currentFamily()
                        onSelected: function(v) {
                            var d = fontBridge.setFamily(v)
                            styleModel.value = d.styles
                            root.previewUrl = d.preview
                        }
                    }
                }
                Column {
                    width: 140 * Theme.scaleFactor
                    height: parent.height
                    spacing: 6 * Theme.scaleFactor
                    Text { text: root.labels.style; color: Theme.fg; font.bold: true
                           font.pixelSize: 14 * Theme.scaleFactor }
                    SelectableList {
                        width: parent.width; height: parent.height - 20 * Theme.scaleFactor
                        items: styleModel.value
                        currentValue: fontBridge.currentStyle()
                        onSelected: function(v) { root.previewUrl = fontBridge.setStyle(v).preview }
                    }
                }
                Column {
                    width: 80 * Theme.scaleFactor
                    height: parent.height
                    spacing: 6 * Theme.scaleFactor
                    Text { text: root.labels.size; color: Theme.fg; font.bold: true
                           font.pixelSize: 14 * Theme.scaleFactor }
                    SelectableList {
                        width: parent.width; height: parent.height - 20 * Theme.scaleFactor
                        items: fontBridge.requestSizes()
                        currentValue: fontBridge.currentSize()
                        onSelected: function(v) { root.previewUrl = fontBridge.setSize(v).preview }
                    }
                }
            }

            // 右：选项卡面板（Filter / Advanced）。
            Item {
                anchors.left: columns3.right
                anchors.leftMargin: 16 * Theme.scaleFactor
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.bottom: parent.bottom

                // 选项卡行（自绘）。
                Row {
                    id: tabBar
                    spacing: 4 * Theme.scaleFactor
                    Repeater {
                        model: [ { key: "filter", label: root.labels.filter }, { key: "advanced", label: root.labels.advanced } ]
                        delegate: Rectangle {
                            width: tabText.width + 24 * Theme.scaleFactor
                            height: 28 * Theme.scaleFactor
                            radius: 6 * Theme.scaleFactor
                            color: root.activeTab === modelData.key ? Theme.fieldBg : "transparent"
                            border.width: 1 * Theme.scaleFactor
                            border.color: root.activeTab === modelData.key ? Theme.borderClr : "transparent"
                            Text {
                                id: tabText
                                anchors.centerIn: parent
                                text: modelData.label
                                color: root.activeTab === modelData.key ? Theme.fg : Theme.borderClr
                                font.pixelSize: 13 * Theme.scaleFactor
                                font.bold: root.activeTab === modelData.key
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.activeTab = modelData.key
                            }
                        }
                    }
                }

                // 选项卡内容。
                Rectangle {
                    anchors.top: tabBar.bottom
                    anchors.topMargin: 4 * Theme.scaleFactor
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    color: Theme.fieldBg
                    radius: 8 * Theme.scaleFactor
                    border.width: 1 * Theme.scaleFactor
                    border.color: Theme.borderClr
                    clip: true

                    // Filter 选项卡：9 项下拉，3 列。
                    Grid {
                        id: filterGrid
                        visible: root.activeTab === "filter"
                        anchors.fill: parent
                        anchors.margins: 8 * Theme.scaleFactor
                        columns: 3
                        columnSpacing: 12 * Theme.scaleFactor
                        rowSpacing: 6 * Theme.scaleFactor

                        Repeater {
                            model: fontBridge.filterMeta()
                            delegate: EnumCombo {
                                width: (filterGrid.width - 2 * filterGrid.columnSpacing) / 3
                                label: modelData.label
                                options: modelData.options
                                value: modelData.value
                                onChanged: function(v) {
                                    var d = fontBridge.setFilter(modelData.var, v)
                                    familyModel.value = d.families
                                    root.previewUrl = d.preview
                                }
                            }
                        }
                    }

                    // Advanced 选项卡：定制下拉，3 列。
                    Grid {
                        id: customizeGrid
                        visible: root.activeTab === "advanced"
                        anchors.fill: parent
                        anchors.margins: 8 * Theme.scaleFactor
                        columns: 3
                        columnSpacing: 12 * Theme.scaleFactor
                        rowSpacing: 6 * Theme.scaleFactor

                        Repeater {
                            model: fontBridge.customizeMeta()
                            delegate: EnumCombo {
                                width: (customizeGrid.width - 2 * customizeGrid.columnSpacing) / 3
                                label: modelData.label
                                options: modelData.options
                                value: modelData.value
                                onChanged: function(v) {
                                    root.previewUrl = fontBridge.setCustomize(modelData.which, v).preview
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- 下区：左预览 + 右按钮 ----
        Item {
            id: bottomArea
            anchors.top: topArea.bottom
            anchors.topMargin: 12 * Theme.scaleFactor
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            // 左：样本类型 + 预览图（宽高比跟随图片）。
            Column {
                id: previewCol
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 600 * Theme.scaleFactor
                spacing: 8 * Theme.scaleFactor

                EnumCombo {
                    width: 240 * Theme.scaleFactor
                    label: root.labels.sample
                    options: fontBridge.sampleKinds()
                    value: fontBridge.currentSampleKind()
                    onChanged: function(v) { root.previewUrl = fontBridge.setSampleKind(v).preview }
                }
                PreviewPane {
                    width: parent.width
                    imageSource: root.previewUrl
                    // 高度由 PreviewPane 据图片宽高比自算（implicitHeight = width/aspect）。
                    height: implicitHeight
                }
            }

            // 右：高级/导入/重置（顶部）+ 确认/取消（底部），分别锚定。
            Item {
                anchors.left: previewCol.right
                anchors.leftMargin: 16 * Theme.scaleFactor
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                // 高级/导入/重置（顶部）。
                DialogButtons {
                    id: actionBtns
                    anchors.top: parent.top
                    buttonLabels: [root.labels.advanced, root.labels["import"], root.labels.reset]
                    primaryIndex: -1
                    buttonWidth: 84 * Theme.scaleFactor
                    onClicked: function(i) {
                        if (i === 0) root.activeTab = "advanced"
                        else if (i === 1) fontBridge.importFont()
                        else if (i === 2) { fontBridge.reset(); root.refreshAll() }
                    }
                }
                // 确认/取消（底部，右对齐）。
                DialogButtons {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    buttonLabels: [root.labels.ok, root.labels.cancel]
                    primaryIndex: 0
                    buttonWidth: 84 * Theme.scaleFactor
                    onClicked: function(i) { i === 0 ? fontBridge.submit() : fontBridge.cancel() }
                }
            }
        }
    }

    // family/style model 用 QtObject 持可变 list（SelectableList.items 绑定）。
    // filter 改动刷新 family，family 改动刷新 style——这两个是动态的；size 静态。
    QtObject {
        id: familyModel
        property var value: fontBridge.requestFamilies()
    }
    QtObject {
        id: styleModel
        property var value: fontBridge.requestStyles(fontBridge.currentFamily())
    }

    // Reset 后整体重拉（family/style/preview 都可能变）。
    function refreshAll() {
        familyModel.value = fontBridge.requestFamilies()
        styleModel.value = fontBridge.requestStyles(fontBridge.currentFamily())
        previewUrl = fontBridge.requestPreview()
    }
}
