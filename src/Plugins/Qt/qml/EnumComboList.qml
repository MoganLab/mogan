// EnumComboList.qml — 可滚动的 EnumCombo 竖列（Filter/Advanced 选项卡内容复用）。
// 调用方提供 meta 列表，每项含 {label, options, optionsTr, value, editable}；超高时纵向滚动。
// itemChanged 外发整条 item + 选中的英文 key，由调用方按各自字段（var/which）做副作用。
//
// API：
//   model                      : list<meta>  —— 每项 {label, options, optionsTr, value, editable?}。
//   itemChanged(var item, string value)      —— 某项选中新值时发出，value 为英文 key。
//
// 用法（宽度/高度由父布局给定）：
//   EnumComboList {
//       anchors.fill: parent; model: filterModel.value
//       onItemChanged: function(item, v) { fontBridge.setFilter(item.var, v) }
//   }

import QtQuick
import "."

Flickable {
    id: root
    contentWidth: width
    contentHeight: column.height
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    property var model: []
    signal itemChanged (var item, string value)

    Column {
        id: column
        width: root.width
        spacing: 6 * Theme.scaleFactor

        Repeater {
            model: root.model
            delegate: EnumCombo {
                width: column.width
                label: modelData.label
                options: modelData.options
                optionsTr: modelData.optionsTr !== undefined ? modelData.optionsTr : []
                value: modelData.value
                editable: modelData.editable !== undefined ? modelData.editable : false
                onChanged: function (v) { root.itemChanged (modelData, v) }
            }
        }
    }
}
