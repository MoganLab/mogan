// GroupHeader.qml — 分组小标题原子。
// 供字段列表里组首字段（带 group 文案）渲染分组标题，如首选项 Mathematics 的
// Keyboard / Contextual hints、Other 的 Miscellaneous / Experimental。
//
// 样式：加粗标题 + 上下对称间距（groupHeaderTopGap），不画分隔线（靠间距区分组）。
// 单栏 / 双栏半宽列间距一致。
//
// API：
//   text     : string —— 分组标题文案（已 translate，bridge cork_to_utf8 还原）。
//   isNarrow : bool   —— 保留供调用方传入（与 Toggle/EnumCombo 同接口）；当前不影响布局。
//
// 宽度由父行给定（满宽或双栏半宽列）。

import QtQuick
import "."

Item {
    id: header

    property var text: ""
    property bool isNarrow: false

    // 高度 = 上间距 + 标题行 + 底部间距（所有 group 统一加上间距）。
    // implicitHeight 供调用方按 visible 折叠（visible=false 时 height 归零不占位）。
    implicitHeight: Theme.groupHeaderTopGap
            + Theme.fontGroupHeader + Theme.groupHeaderTopGap
    height: implicitHeight
    width: parent ? parent.width : 0

    Text {
        anchors.top: parent.top
        anchors.topMargin: Theme.groupHeaderTopGap
        width: parent.width
        text: header.text || ""
        color: Theme.fg
        font.pixelSize: Theme.fontGroupHeader
        font.bold: true
        elide: Text.ElideRight
    }
}
