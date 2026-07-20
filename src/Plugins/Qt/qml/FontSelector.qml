// FontSelector.qml — 字体选择器正文。
// 非阻塞模态对话框：左侧 selector-set 实时写回文档（live），Cancel 经打开时
// 快照写回撤销，Reset 按 global? 分流（文档级恢复系统默认、段落级回快照），OK 补齐差异落定。
// 由 DialogShell + SelectableList + EnumCombo +
// PreviewPane + DialogButtons 拼装，交互经 fontBridge 转发到 scheme facade
//（specsKey 句柄）。fontBridge setter 返回 {preview, styles/families} 等联动
// 结果，QML 在同一 handler 更新 model + 预览，省二次往返。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 980
    implicitHeight: 600
    onCancel: () => fontBridge.cancel()

    // 预览 data URL。各 setter 返回新 preview 时更新。
    property string previewUrl: fontBridge.requestPreview()
    property var labels: fontBridge.uiLabels()
    property string activeTab: "filter"

    readonly property real leftColumnWidth: 540 * Theme.scaleFactor
    readonly property real columnGap: 16 * Theme.scaleFactor

    content: Item {
        Column {
            id: leftColumn
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.leftColumnWidth
            spacing: root.columnGap

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
                    // currentValue 绑定读本 list 的 refreshTick（建立重算依赖）。
                    currentValue: {
                        familyList.refreshTick;
                        return fontBridge.currentFamily();
                    }
                    onSelected: function (v) {
                        var d = fontBridge.setFamily(v);
                        styleModel.value = d.styles;
                        root.previewUrl = d.preview;
                    }
                }
                SelectableList {
                    id: styleList
                    width: 172 * Theme.scaleFactor
                    height: parent.height
                    title: root.labels.style
                    items: styleModel.value
                    currentValue: {
                        styleList.refreshTick;
                        return fontBridge.currentStyle();
                    }
                    onSelected: function (v) {
                        root.previewUrl = fontBridge.setStyle(v).preview;
                    }
                }
                SelectableList {
                    id: sizeList
                    width: 115 * Theme.scaleFactor
                    height: parent.height
                    title: root.labels.size
                    items: {
                        sizeList.refreshTick;
                        return fontBridge.requestSizes();
                    }
                    currentValue: {
                        sizeList.refreshTick;
                        return fontBridge.currentSize();
                    }
                    onSelected: function (v) {
                        root.previewUrl = fontBridge.setSize(v).preview;
                    }
                }
            }

            Column {
                width: parent.width
                height: parent.height - columns3.height - parent.spacing
                spacing: 8 * Theme.scaleFactor

                EnumCombo {
                    width: 240 * Theme.scaleFactor
                    label: root.labels.sample
                    options: fontBridge.sampleKinds()
                    value: fontBridge.currentSampleKind()
                    onChanged: function (v) {
                        root.previewUrl = fontBridge.setSampleKind(v).preview;
                    }
                }
                PreviewPane {
                    width: parent.width
                    imageSource: root.previewUrl
                    height: parent.height - Theme.rowH - 8 * Theme.scaleFactor
                }
            }
        }

        Item {
            id: rightColumn
            anchors.left: leftColumn.right
            anchors.leftMargin: root.columnGap
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            TabPanel {
                id: tabPanel
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: bottomButtons.top
                anchors.bottomMargin: 12 * Theme.scaleFactor
                tabs: [
                    {
                        key: "filter",
                        label: root.labels.filter
                    },
                    {
                        key: "advanced",
                        label: root.labels.advanced
                    }
                ]
                activeKey: root.activeTab
                onActiveKeyChanged: root.activeTab = activeKey
                content: Item {
                    EnumComboList {
                        anchors.fill: parent
                        visible: root.activeTab === "filter"
                        model: filterModel.value
                        onItemChanged: function (item, v) {
                            var d = fontBridge.setFilter(item.var, v);
                            familyModel.value = d.families;
                            filterModel.value = fontBridge.filterMeta();
                            root.previewUrl = d.preview;
                        }
                    }
                    EnumComboList {
                        anchors.fill: parent
                        visible: root.activeTab === "advanced"
                        model: customizeModel.value
                        onItemChanged: function (item, v) {
                            root.previewUrl = fontBridge.setCustomize(item.which, v).preview;
                            customizeModel.value = fontBridge.customizeMeta();
                        }
                    }
                }
            }

            // 四按钮并排铺满右列宽：Import/Reset/OK/Cancel（Advanced 已并入顶部选项卡）。
            DialogButtons {
                id: bottomButtons
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                buttonLabels: [root.labels["import"], root.labels.reset, root.labels.ok, root.labels.cancel]
                primaryIndex: 2
                buttonWidth: 82 * Theme.scaleFactor
                onClicked: function (i) {
                    // font-import 的 refresh-now 只对老 tm-widget 标签生效，Import 后
                    // 须手动重拉 family 列表让新字体出现。
                    if (i === 0) {
                        fontBridge.importFont();
                        familyModel.value = fontBridge.requestFamilies();
                    } else if (i === 1) {
                        fontBridge.reset();
                        root.refreshAll();
                    } else if (i === 2)
                        fontBridge.submit();
                    else
                        fontBridge.cancel();
                }
            }
        }
    }

    // family/style/filter/customize 用 QtObject 持可变 list：选项变化后重拉触发刷新
    //（否则 SelectableList.items / EnumCombo.value 是旧快照不更新）。
    QtObject {
        id: familyModel
        property var value: fontBridge.requestFamilies()
    }
    QtObject {
        id: styleModel
        property var value: fontBridge.requestStyles(fontBridge.currentFamily())
    }
    QtObject {
        id: filterModel
        property var value: fontBridge.filterMeta()
    }
    QtObject {
        id: customizeModel
        property var value: fontBridge.customizeMeta()
    }

    function refreshAll() {
        // 递增各 list 自身的 refreshTick，驱动其 currentValue/items 绑定重算
        //（取代对话框级单一 refreshTick）。重算后值可能未变（reset 后回系统默认或打开时快照、可能==改前值），
        // onCurrentValueChanged 不发、activeValue 不更新——显式 syncActiveValue 同步选中框。
        familyList.refreshTick++;
        styleList.refreshTick++;
        sizeList.refreshTick++;
        familyModel.value = fontBridge.requestFamilies();
        styleModel.value = fontBridge.requestStyles(fontBridge.currentFamily());
        filterModel.value = fontBridge.filterMeta();
        customizeModel.value = fontBridge.customizeMeta();
        previewUrl = fontBridge.requestPreview();
        familyList.syncActiveValue();
        styleList.syncActiveValue();
        sizeList.syncActiveValue();
    }
}
