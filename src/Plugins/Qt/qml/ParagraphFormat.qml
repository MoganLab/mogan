// ParagraphFormat.qml — 段落格式对话框正文。
// 非阻塞模态（run_modal_qml_dialog）：每次改动经 paraBridge.setPara live 写回文档
// （make-multi-line-with），主窗口实时重排段落；OK 落定，Cancel/重置走打开时快照
// 写回撤销。
//
// 显示真相源是 QML 本地的 values（参考 FormDialog）：打开时从 meta 读一次初始化，
// 之后改动直接改 values 并 setPara 写文档，不重读 get-env——后者相对编辑命令有延迟，
// 重读会显示滞后。id 用 dialog（非 root），避免被 EnumCombo/MiniButton 等原子内部
// id: root 遮蔽。
//
// paraBridge 契约（ParagraphFormatBridge，无状态透传 specsKey）：
//   uiLabels()            -> {basic, advanced, reset, ok, cancel, sepPresetLabel, sepPresets}
//   basicMeta()           -> [{label, options, var, value, editable}]（打开时读一次）
//   advancedMeta()        -> 同上（高级 tab）
//   setPara(var, val)     -> live 写回文档
//   reset()               -> 快照撤销（重置按钮；Cancel 由 cancel() 另走关窗）
//   submit()/cancel()

import QtQuick
import "." // DialogShell / EnumCombo / EnumComboList / MiniButton / DialogButtons / TabBar / Theme

DialogShell {
    id: dialog
    implicitWidth: 520
    implicitHeight: 590
    onCancel: () => paraBridge.cancel()

    property var labels: paraBridge.uiLabels()
    property string activeTab: "basic"

    // 字段定义（打开时读一次，只读）+ 运行时值（显示真相源，改动直改它）。
    property var basicFields: paraBridge.basicMeta()
    property var advancedFields: paraBridge.advancedMeta()
    property var basicValues: initValues(basicFields)
    property var advancedValues: initValues(advancedFields)

    // 预设按钮行与 EnumCombo 同布局：label 占 labelRatio，按钮组占 combo 区。
    readonly property real labelRatio: 0.42
    readonly property real rowSpacing: 16 * Theme.scaleFactor
    readonly property real presetGap: 8 * Theme.scaleFactor
    function presetBtnWidth(rowWidth) {
        var comboArea = rowWidth - rowWidth * dialog.labelRatio - dialog.rowSpacing
        return (comboArea - 3 * dialog.presetGap) / 4
    }

    // 从 meta 列表建 {var: value} 映射。
    function initValues(fields) {
        var v = {}
        for (var i = 0; i < fields.length; i++) v[fields[i].var] = fields[i].value
        return v
    }
    // 改某分组某字段：更新本地 values（触发显示刷新）+ live 写回文档。
    function setField(group, varName, val) {
        var cur = group === "basic" ? dialog.basicValues : dialog.advancedValues
        cur[varName] = val
        if (group === "basic") dialog.basicValues = cur; else dialog.advancedValues = cur
        paraBridge.setPara(varName, val)
    }

    content: Item {
        id: shell

        TabBar {
            id: tabBar
            anchors.top: parent.top
            anchors.left: parent.left
            model: [ { key: "basic", label: dialog.labels.basic },
                     { key: "advanced", label: dialog.labels.advanced } ]
            activeKey: dialog.activeTab
            onSelected: function(key) { dialog.activeTab = key }
        }

        DialogButtons {
            id: bottomButtons
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            buttonLabels: [dialog.labels.reset, dialog.labels.ok, dialog.labels.cancel]
            primaryIndex: 1
            buttonWidth: 90 * Theme.scaleFactor
            onClicked: function(i) {
                if (i === 0) { paraBridge.reset(); dialog.resetValues() }
                else if (i === 1) paraBridge.submit()
                else paraBridge.cancel()
            }
        }

        Item {
            id: body
            anchors.top: tabBar.bottom
            anchors.topMargin: 14 * Theme.scaleFactor
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: bottomButtons.top
            anchors.bottomMargin: 14 * Theme.scaleFactor
            clip: true

            // 基础 tab：par-sep 行后插行间距预设按钮行，故自定义 Column。
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
                        model: dialog.basicFields
                        delegate: Column {
                            width: basicCol.width
                            spacing: 6 * Theme.scaleFactor
                            EnumCombo {
                                width: parent.width
                                label: modelData.label
                                options: modelData.options
                                value: dialog.basicValues[modelData.var] !== undefined
                                       ? dialog.basicValues[modelData.var] : ""
                                editable: modelData.editable !== undefined ? modelData.editable : false
                                onChanged: function(v) { dialog.setField("basic", modelData.var, v) }
                            }
                            // 仅 par-sep 行下面插预设按钮行（行高 44，与 EnumCombo 行一致）。
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
                                            onClicked: dialog.setField("basic", "par-sep", modelData.val)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 高级 tab：纯 EnumCombo 竖列，用 EnumComboList 原子（valueSource 模式）。
            EnumComboList {
                anchors.fill: parent
                visible: dialog.activeTab === "advanced"
                model: dialog.advancedFields
                valueSource: dialog.advancedValues
                onItemChanged: function(item, v) { dialog.setField("advanced", item.var, v) }
            }
        }
    }

    // 重置：scheme 快照撤销文档后，本地 values 回到打开时的 meta 值。
    function resetValues() {
        dialog.basicValues = dialog.initValues(dialog.basicFields)
        dialog.advancedValues = dialog.initValues(dialog.advancedFields)
    }
}
