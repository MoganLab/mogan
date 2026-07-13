// EnumComboList.qml — 可滚动的 EnumCombo 竖列（Filter/Advanced 选项卡内容复用）。
// Flickable + Column + Repeater<EnumCombo>，每项按 meta 渲染；超高时纵向滚动。
// 调用方提供 meta 列表（每项含 label/options/optionsTr/value），onChanged 外发整条
// item + 选中的英文 key，由调用方按各自字段（var/which）做副作用。
//
// API：
//   model                : list<meta>  —— 每项 {label, options, optionsTr, value}。
//   signal itemChanged(var item, string value) —— 某项选中新值时发出，value 为英文 key。
//
// 样式参数（边距/间距/滚动）与原 FontSelector 内联的 Flickable+Column 逐项一致。

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
                onChanged: function (v) { root.itemChanged (modelData, v) }
            }
        }
    }
}
