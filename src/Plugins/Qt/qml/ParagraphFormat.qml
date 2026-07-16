// ParagraphFormat.qml — 段落格式对话框正文（「格式→段落」与「文档→段落」共用）。
// 非阻塞模态（run_modal_qml_dialog）：每次改动经 paraBridge.setPara live 写回
// （段落级写段落 with、文档级写文档 initial），主窗口实时重排；OK 落定，Cancel 走
// 打开时快照写回，重置按 scope（段落级快照写回 / 文档级恢复默认）。scope 分流在
// scheme facade，QML 不感知。
//
// 显示真相源是 QML 本地的 values（参考 FormDialog）：打开时从 meta 读一次初始化，
// 之后改动直接改 values 并 setPara 写文档，不重读 get-env——后者相对编辑命令有延迟，
// 重读会显示滞后。唯一例外是「重置」：scheme 侧按 scope 撤销后 values 必须重读 meta
// 重建（文档级 init-default 后真相源已变，缓存值会与文档背离）。根 id 用 root，
// 与其它成品一致——原子内部 id 不用 root（见 atoms/ 各文件），调用方 delegate
// 的 root.xxx 不再被遮蔽。
//
// paraBridge 契约（ParagraphFormatBridge，无状态透传 specsKey）：
//   uiLabels()            -> {basic, advanced, reset, ok, cancel, sepPresetLabel, sepPresets}
//   basicMeta()           -> [{label, options, var, value, editable}]（打开时读一次；
//                            文档级基础 tab 不含 par-left/par-right）
//   advancedMeta()        -> 同上（高级 tab）
//   setPara(var, val)     -> live 写回（段落 with 或文档 initial）
//   reset()               -> 段落级快照写回 / 文档级恢复默认（不关窗）
//   submit()/cancel()     -> OK 落定 / Cancel 快照写回，均关窗

import QtQuick
import "." // DialogShell / EnumCombo / EnumComboList / MiniButton / DialogButtons / TabBar / Theme

DialogShell {
    id: root
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
        var comboArea = rowWidth - rowWidth * root.labelRatio - root.rowSpacing;
        return (comboArea - 3 * root.presetGap) / 4;
    }

    // 从 meta 列表建 {var: value} 映射。
    function initValues(fields) {
        var v = {};
        for (var i = 0; i < fields.length; i++)
            v[fields[i].var] = fields[i].value;
        return v;
    }
    // 改某分组某字段：更新本地 values（触发显示刷新）+ live 写回文档。
    function setField(group, varName, val) {
        var cur = group === "basic" ? root.basicValues : root.advancedValues;
        cur[varName] = val;
        if (group === "basic")
            root.basicValues = cur;
        else
            root.advancedValues = cur;
        paraBridge.setPara(varName, val);
    }

    content: Item {
        id: shell

        TabBar {
            id: tabBar
            anchors.top: parent.top
            anchors.left: parent.left
            model: [
                {
                    key: "basic",
                    label: root.labels.basic
                },
                {
                    key: "advanced",
                    label: root.labels.advanced
                }
            ]
            activeKey: root.activeTab
            onSelected: function (key) {
                root.activeTab = key;
            }
        }

        DialogButtons {
            id: bottomButtons
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            buttonLabels: [root.labels.reset, root.labels.ok, root.labels.cancel]
            primaryIndex: 1
            buttonWidth: 90 * Theme.scaleFactor
            onClicked: function (i) {
                if (i === 0) {
                    paraBridge.reset();
                    root.resetValues();
                } else if (i === 1)
                    paraBridge.submit();
                else
                    paraBridge.cancel();
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
                visible: root.activeTab === "basic"
                contentWidth: width
                contentHeight: basicCol.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                Column {
                    id: basicCol
                    width: parent.width
                    spacing: 6 * Theme.scaleFactor
                    Repeater {
                        model: root.basicFields
                        delegate: Column {
                            width: basicCol.width
                            spacing: 6 * Theme.scaleFactor
                            EnumCombo {
                                width: parent.width
                                label: modelData.label
                                options: modelData.options
                                value: root.basicValues[modelData.var] !== undefined ? root.basicValues[modelData.var] : ""
                                editable: modelData.editable !== undefined ? modelData.editable : false
                                onChanged: function (v) {
                                    root.setField("basic", modelData.var, v);
                                }
                            }
                            // 仅 par-sep 行下面插预设按钮行（行高 44，与 EnumCombo 行一致）。
                            Item {
                                id: presetRow
                                width: parent.width
                                height: modelData.var === "par-sep" ? Theme.rowH : 0
                                visible: modelData.var === "par-sep"
                                Text {
                                    id: presetLabel
                                    width: presetRow.width * root.labelRatio
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.labels.sepPresetLabel !== undefined ? root.labels.sepPresetLabel : ""
                                    color: Theme.fg
                                    font.pixelSize: 14 * Theme.scaleFactor
                                    elide: Text.ElideRight
                                }
                                Row {
                                    anchors.left: presetLabel.right
                                    anchors.leftMargin: root.rowSpacing
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: root.presetGap
                                    Repeater {
                                        model: root.labels.sepPresets !== undefined ? root.labels.sepPresets : []
                                        delegate: MiniButton {
                                            width: root.presetBtnWidth(presetRow.width)
                                            text: modelData.label
                                            onClicked: root.setField("basic", "par-sep", modelData.val)
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
                visible: root.activeTab === "advanced"
                model: root.advancedFields
                valueSource: root.advancedValues
                onItemChanged: function (item, v) {
                    root.setField("advanced", item.var, v);
                }
            }
        }
    }

    // 重置：scheme 侧已按 scope 撤销（段落级快照写回 / 文档级 init-default），
    // 本地 values 必须重读 meta 重建——不能用打开时缓存的 basicFields，否则文档级
    // reset 后显示仍是打开时值，而真相源（init）已是全局默认，造成显示与文档背离。
    // 重读 meta 此时拿到的是撤销后的值，两级语义都正确。
    function resetValues() {
        root.basicValues = root.initValues(paraBridge.basicMeta());
        root.advancedValues = root.initValues(paraBridge.advancedMeta());
    }
}
