// FontSelector.qml — 字体选择器正文。全部由 Phase 0 原子板块拼装
// （DialogShell / SelectableList / EnumCombo / PreviewPane / DialogButtons），
// 交互经 fontBridge（C++ FontSelectorBridge）转发到 scheme facade（spec 句柄）。
// 详见 record/qml/font-selector.md Phase 3。
//
// context property（C++ 注入）：fontBridge、closeBridge、dpScale、isDark、specsKey。
// fontBridge 的 setter 返回 {preview, styles/families} 等联动结果，QML 在同一
// handler 更新 model + 预览，省二次往返。单一防抖 Timer 汇聚预览刷新。
//
// 布局（对齐 ai-docs/qml/qml-dialog.html 的 .font-shell 网格）：
//   左列（宽 620×sf）= font-top（三栏 family/style/size）+ font-bottom（预览）；
//   右列（宽 300×sf，跨全高）= 选项卡面板（Filter/Advanced）+ 底部按钮组。
//   右列以 grid-row: 1/span 2 的方式同时覆盖上下区。

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
    // 刷新节拍：reset 后自增，驱动「绑定到无参 bridge 函数」的 currentValue 重算
    //（这类 binding 不会因 scheme 状态变化自动重算）。重算后值可能未变，故
    // refreshAll 还需显式 syncActiveValue 强制选中框同步。
    property int refreshTick: 0

    // 左列固定宽度。左列尽量放大，右列压缩到刚够容纳底部并排四按钮。
    readonly property real leftColumnWidth: 540 * Theme.scaleFactor
    readonly property real columnGap: 16 * Theme.scaleFactor

    content: Item {
        // ---- 左列：三栏列表（上）+ 预览（下） ----
        Column {
            id: leftColumn
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.leftColumnWidth
            spacing: root.columnGap

            // 上：三栏 family/style/size（缩窄）。标题已并入 SelectableList 容器内顶部。
            Row {
                id: columns3
                width: parent.width
                height: 300 * Theme.scaleFactor
                spacing: 12 * Theme.scaleFactor

                SelectableList {
                    id: familyList
                    width: 229 * Theme.scaleFactor
                    height: parent.height
                    title: root.labels.family
                    items: familyModel.value
                    currentValue: { var _ = root.refreshTick; return fontBridge.currentFamily() }
                    onSelected: function(v) {
                        var d = fontBridge.setFamily(v)
                        styleModel.value = d.styles
                        root.previewUrl = d.preview
                    }
                }
                SelectableList {
                    id: styleList
                    width: 172 * Theme.scaleFactor
                    height: parent.height
                    title: root.labels.style
                    items: styleModel.value
                    currentValue: { var _ = root.refreshTick; return fontBridge.currentStyle() }
                    onSelected: function(v) { root.previewUrl = fontBridge.setStyle(v).preview }
                }
                SelectableList {
                    id: sizeList
                    width: 115 * Theme.scaleFactor
                    height: parent.height
                    title: root.labels.size
                    items: { var _ = root.refreshTick; return fontBridge.requestSizes() }
                    currentValue: { var _ = root.refreshTick; return fontBridge.currentSize() }
                    onSelected: function(v) { root.previewUrl = fontBridge.setSize(v).preview }
                }
            }

            // 下：样本类型 + 预览图。
            Column {
                width: parent.width
                height: parent.height - columns3.height - parent.spacing
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
                    // 预览高度 = 剩余高 - 样本下拉高(44×sf) - spacing(8×sf)。
                    // 预览内容超高时 PreviewPane 内部 Flickable 滚动，不撑破布局。
                    height: parent.height - 44 * Theme.scaleFactor - 8 * Theme.scaleFactor
                }
            }
        }

        // ---- 右列：选项卡面板（占满）+ 底部按钮组（跨上下区） ----
        Item {
            id: rightColumn
            anchors.left: leftColumn.right
            anchors.leftMargin: root.columnGap
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            // 选项卡面板（顶部，占据剩余高度）。直角无框容器（listBg 底），选项卡
            // 行与内容都在容器内；选中态为圆角胶囊（波浪包裹），色调与列表选中一致。
            Rectangle {
                id: tabPanel
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: bottomButtons.top
                anchors.bottomMargin: 12 * Theme.scaleFactor
                color: Theme.listBg
                radius: 8 * Theme.scaleFactor
                border.width: 1 * Theme.scaleFactor
                border.color: Theme.borderClr
                clip: true

                // 选项卡行（容器内顶部）。
                Row {
                    id: tabBar
                    anchors.top: parent.top
                    anchors.topMargin: 8 * Theme.scaleFactor
                    anchors.left: parent.left
                    anchors.leftMargin: 8 * Theme.scaleFactor
                    spacing: 8 * Theme.scaleFactor
                    Repeater {
                        model: [ { key: "filter", label: root.labels.filter }, { key: "advanced", label: root.labels.advanced } ]
                        delegate: Rectangle {
                            readonly property bool isActive: root.activeTab === modelData.key
                            width: tabText.width + 28 * Theme.scaleFactor
                            height: 30 * Theme.scaleFactor
                            radius: height / 2
                            color: isActive ? Theme.selectBg
                                            : (tabMa.containsMouse ? Theme.fieldBgHover : "transparent")
                            border.width: isActive ? 1 * Theme.scaleFactor : 0
                            border.color: Theme.selectBorder
                            Text {
                                id: tabText
                                anchors.centerIn: parent
                                text: modelData.label
                                color: isActive ? Theme.selectFg : Theme.fg
                                font.pixelSize: 13 * Theme.scaleFactor
                                font.bold: isActive
                            }
                            MouseArea {
                                id: tabMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.activeTab = modelData.key
                            }
                        }
                    }
                }

                // Filter 选项卡：9 项下拉，垂直单列排列，超高时纵向滚动。四周留内缩进。
                Flickable {
                    visible: root.activeTab === "filter"
                    anchors.top: tabBar.bottom
                    anchors.topMargin: 8 * Theme.scaleFactor
                    anchors.left: parent.left
                    anchors.leftMargin: 8 * Theme.scaleFactor
                    anchors.right: parent.right
                    anchors.rightMargin: 8 * Theme.scaleFactor
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 8 * Theme.scaleFactor
                    contentWidth: width
                    contentHeight: filterCol.height
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    Column {
                        id: filterCol
                        width: parent.width
                        spacing: 6 * Theme.scaleFactor

                        Repeater {
                            model: fontBridge.filterMeta()
                            delegate: EnumCombo {
                                width: filterCol.width
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
                }

                // Advanced 选项卡：定制下拉，垂直单列排列，超高时纵向滚动。四周留内缩进。
                Flickable {
                    visible: root.activeTab === "advanced"
                    anchors.top: tabBar.bottom
                    anchors.topMargin: 8 * Theme.scaleFactor
                    anchors.left: parent.left
                    anchors.leftMargin: 8 * Theme.scaleFactor
                    anchors.right: parent.right
                    anchors.rightMargin: 8 * Theme.scaleFactor
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 8 * Theme.scaleFactor
                    contentWidth: width
                    contentHeight: customizeCol.height
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    Column {
                        id: customizeCol
                        width: parent.width
                        spacing: 6 * Theme.scaleFactor

                        Repeater {
                            model: fontBridge.customizeMeta()
                            delegate: EnumCombo {
                                width: customizeCol.width
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

            // 底部按钮组（右列底部，四按钮并排铺满右列宽）：导入/重置/确定/取消。
            // 高级入口已并入顶部选项卡（activeTab="advanced"），此处不再单列按钮。
            // buttonWidth = (右列宽 376 - 3 间距×16) / 4 = 82，正好铺满右列不溢出。
            DialogButtons {
                id: bottomButtons
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                buttonLabels: [root.labels["import"], root.labels.reset, root.labels.ok, root.labels.cancel]
                primaryIndex: 2
                buttonWidth: 82 * Theme.scaleFactor
                onClicked: function(i) {
                    if (i === 0) fontBridge.importFont()
                    else if (i === 1) { fontBridge.reset(); root.refreshAll() }
                    else if (i === 2) fontBridge.submit()
                    else fontBridge.cancel()
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
        root.refreshTick++
        familyModel.value = fontBridge.requestFamilies()
        styleModel.value = fontBridge.requestStyles(fontBridge.currentFamily())
        previewUrl = fontBridge.requestPreview()
        // currentValue 重算后值可能未变（reset 回到打开时默认），changed 信号不发，
        // activeValue 不会更新；此处显式同步选中框。
        familyList.syncActiveValue()
        styleList.syncActiveValue()
        sizeList.syncActiveValue()
    }
}
