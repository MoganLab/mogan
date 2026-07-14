// ParagraphFormat.qml — 段落格式对话框正文。
// 非阻塞模态（run_modal_qml_dialog）：每次 setPara 经 selector 机制 live 写回文档
// （make-multi-line-with），主窗口实时重排段落；OK 落定，Cancel/重置走打开时快照
// 写回撤销。由 DialogShell + TabBar(基础/高级) + EnumCombo/EnumComboList(editable)
// + MiniButton(行间距预设) + DialogButtons 拼装，交互经 paraBridge 转发到 scheme
// facade（specsKey 句柄）。
//
// 布局以 ai-docs/qml/qml-dialog.html 的 #panel-paragraph 设计稿为准：基础/高级两 tab，
// 19 个段落参数；行间距预设 1.0x/1.25x/1.5x/2.0x 四按钮等宽，与 combo 列对齐。
//
// id 用 dialog（非 root）：EnumCombo/MiniButton/DialogButtons 等原子内部都用 id: root，
// 若本组件也用 root，delegate handler 里的 `root.xxx` 会被原子实例的同名 id 遮蔽，
// 导致刷新调用打到错误对象（点击不生效）。dialog 全局唯一，无遮蔽。
//
// paraBridge 契约（ParagraphFormatBridge，无状态透传 specsKey）：
//   uiLabels()            -> {basic, advanced, reset, ok, cancel, sepPresetLabel, sepPresets}
//   basicMeta()           -> [{label, options, var, value, editable}]（基础 tab）
//   advancedMeta()        -> [{label, options, var, value, editable}]（高级 tab）
//   setPara(var, val)     -> live 写回；返回新 value
//   submit()/cancel()/reset()

import QtQuick
import "." // DialogShell / EnumCombo / EnumComboList / MiniButton / DialogButtons / TabBar / Theme

DialogShell {
    id: dialog
    implicitWidth: 520
    implicitHeight: 600
    onCancel: () => paraBridge.cancel()

    property var labels: paraBridge.uiLabels()
    property string activeTab: "basic"

    // 预设按钮行与 EnumCombo 同布局：label 占 labelRatio，按钮组占 combo 区。
    readonly property real labelRatio: 0.42
    readonly property real rowSpacing: 16 * Theme.scaleFactor
    readonly property real presetGap: 8 * Theme.scaleFactor
    // 4 个预设按钮 + 3 个间距 = combo 区宽，按钮等宽。
    function presetBtnWidth(rowWidth) {
        var comboArea = rowWidth - rowWidth * dialog.labelRatio - dialog.rowSpacing
        return (comboArea - 3 * dialog.presetGap) / 4
    }

    // live 写回后重拉 meta（选项/值随文档变化）。basicModel/advancedModel 是本组件
    // 内 id，不被原子遮蔽。用 Qt.callLater 延一帧：setPara 的 make-multi-line-with
    // 触发 typeset，环境值（get-env）在 typeset 落地后才更新；同步立即重拉会读到
    // 旧值，显示滞后一次。延一帧让 typeset 完成再重拉。
    function refreshAll() {
        Qt.callLater(function () {
            basicModel.value = paraBridge.basicMeta()
            advancedModel.value = paraBridge.advancedMeta()
        })
    }

    content: Item {
        id: shell

        // 选项卡行（胶囊 TabBar），锚顶。
        TabBar {
            id: tabBar
            anchors.top: parent.top
            anchors.left: parent.left
            model: [ { key: "basic", label: dialog.labels.basic },
                     { key: "advanced", label: dialog.labels.advanced } ]
            activeKey: dialog.activeTab
            onSelected: function(key) { dialog.activeTab = key }
        }

        // 底部按钮：重置 / 确定 / 取消，锚底靠右。
        DialogButtons {
            id: bottomButtons
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            buttonLabels: [dialog.labels.reset, dialog.labels.ok, dialog.labels.cancel]
            primaryIndex: 1
            buttonWidth: 90 * Theme.scaleFactor
            onClicked: function(i) {
                if (i === 0) { paraBridge.reset(); dialog.refreshAll() }
                else if (i === 1) paraBridge.submit()
                else paraBridge.cancel()
            }
        }

        // 正文区：按 activeTab 切显隐，夹在 tabBar 与 bottomButtons 之间。
        Item {
            id: body
            anchors.top: tabBar.bottom
            anchors.topMargin: 14 * Theme.scaleFactor
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: bottomButtons.top
            anchors.bottomMargin: 14 * Theme.scaleFactor
            clip: true

            // 基础 tab：自定义 Column——需在 par-sep 后插入行间距预设按钮行，故不走
            // EnumComboList。
            Flickable {
                anchors.fill: parent
                visible: dialog.activeTab === "basic"
                contentWidth: width
                contentHeight: basicCol.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                Column {
                    id: basicCol
                    width: parent.width
                    spacing: 6 * Theme.scaleFactor
                    Repeater {
                        model: basicModel.value
                        delegate: Column {
                            width: basicCol.width
                            spacing: 6 * Theme.scaleFactor
                            EnumCombo {
                                width: parent.width
                                label: modelData.label
                                options: modelData.options
                                value: modelData.value
                                editable: modelData.editable !== undefined ? modelData.editable : false
                                onChanged: function(v) {
                                    paraBridge.setPara(modelData.var, v)
                                    dialog.refreshAll()
                                }
                            }
                            // 仅 par-sep 行下面插预设按钮行。行高与 EnumCombo 行一致
                            //（44px），MiniButton 居中、上下留间距，保证每行视觉高度统一。
                            Item {
                                id: presetRow
                                width: parent.width
                                height: modelData.var === "par-sep" ? 44 * Theme.scaleFactor : 0
                                visible: modelData.var === "par-sep"
                                Text {
                                    id: presetLabel
                                    width: presetRow.width * dialog.labelRatio
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: dialog.labels.sepPresetLabel !== undefined
                                          ? dialog.labels.sepPresetLabel : ""
                                    color: Theme.fg
                                    font.pixelSize: 14 * Theme.scaleFactor
                                    elide: Text.ElideRight
                                }
                                Row {
                                    anchors.left: presetLabel.right
                                    anchors.leftMargin: dialog.rowSpacing
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: dialog.presetGap
                                    Repeater {
                                        model: dialog.labels.sepPresets !== undefined ? dialog.labels.sepPresets : []
                                        delegate: MiniButton {
                                            width: dialog.presetBtnWidth(presetRow.width)
                                            text: modelData.label
                                            onClicked: {
                                                paraBridge.setPara("par-sep", modelData.val)
                                                dialog.refreshAll()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 高级 tab：纯 EnumCombo 竖列，直接复用 EnumComboList 原子（自带滚动）。
            EnumComboList {
                anchors.fill: parent
                visible: dialog.activeTab === "advanced"
                model: advancedModel.value
                onItemChanged: function(item, v) {
                    paraBridge.setPara(item.var, v)
                    dialog.refreshAll()
                }
            }
        }
    }

    // 可变 meta 列表：值变化后由 refreshAll() 显式 refetch，否则 EnumCombo.value 是
    // 旧快照不更新（参考 FontSelector 的 QtObject model 模式）。
    QtObject {
        id: basicModel
        property var value: paraBridge.basicMeta()
    }
    QtObject {
        id: advancedModel
        property var value: paraBridge.advancedMeta()
    }
}
