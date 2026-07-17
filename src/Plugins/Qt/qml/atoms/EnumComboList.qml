// EnumComboList.qml — 可滚动的 EnumCombo 竖列（Filter/Advanced 选项卡内容复用）。
// 调用方提供 meta 列表，每项含 {label, options, optionsTr, value, editable}；超高时纵向滚动。
// itemChanged 外发整条 item + 选中的英文 key，由调用方按各自字段（var/which）做副作用。
//
// value 真相源两种模式（按是否设 valueSource 切换）：
// - 默认（FontSelector）：value 取自 modelData.value（meta 自带，字体 selector 模式）。
// - valueSource 模式（FormDialog 风格）：value 取自外部 map valueSource[item[keyField]]，
//   改动只外发 itemChanged，调用方负责更新 valueSource。用于 live 写回且 get-env 重读
//   有延迟的场景（段落格式）——避免显示滞后。
//
// API：
//   model                      : list<meta>  —— 每项 {label, options, optionsTr, value, editable?, <keyField>}。
//   valueSource                : var         —— 可选，{key: value} 外部真相源 map。
//   keyField                   : string      —— item 里作 key 的字段名，默认 "var"。
//   itemChanged(var item, string value)      —— 某项选中新值时发出，value 为英文 key。
//
// 用法（默认模式）：
//   EnumComboList {
//       anchors.fill: parent; model: filterModel.value
//       onItemChanged: function(item, v) { fontBridge.setFilter(item.var, v) }
//   }
// 用法（valueSource 模式）：
//   EnumComboList {
//       model: fields; valueSource: values
//       onItemChanged: function(item, v) { setField(item.var, v) }
//   }

import QtQuick
import "."

Flickable {
    id: comboList
    contentWidth: width
    contentHeight: column.height
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    property var model: []
    property var valueSource: null
    property string keyField: "var"
    signal itemChanged(var item, string value)

    // 取某项当前显示值：优先 valueSource，回退 modelData.value。
    function valueOf(item) {
        if (comboList.valueSource) {
            var k = item[comboList.keyField];
            return comboList.valueSource[k] !== undefined ? comboList.valueSource[k] : "";
        }
        return item.value;
    }

    Column {
        id: column
        width: comboList.width
        spacing: Theme.gapS

        Repeater {
            model: comboList.model
            delegate: EnumCombo {
                width: column.width
                label: modelData.label
                options: modelData.options
                optionsTr: modelData.optionsTr !== undefined ? modelData.optionsTr : []
                value: comboList.valueOf(modelData)
                editable: modelData.editable !== undefined ? modelData.editable : false
                onChanged: function (v) {
                    comboList.itemChanged(modelData, v);
                }
            }
        }
    }
}
