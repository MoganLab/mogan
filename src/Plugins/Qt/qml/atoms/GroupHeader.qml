// GroupHeader.qml — 分组小标题原子。
// 供字段列表里组首字段（带 group 文案）渲染分组标题，如首选项 Mathematics 的
// Keyboard / Contextual hints、Other 的 Miscellaneous / Experimental。
//
// 样式对齐设计稿 .group-title：小号加粗标题 + 上方分隔线（非首组才显示线）+ 上下间距。
// 复用此原子统一所有分组标题的视觉。
//
// isNarrow 与 Toggle/EnumCombo 同语义：落在双栏半宽列时为 true（如 Math 左右两列各自
// 的组标题），此时分隔线不显示（列本身就窄、靠列间距分隔组），间距更紧凑；满宽横跨
// 标题（如 IR 的 Remote controllers，横跨左右双列）isNarrow=false，显示分隔线。
//
// API：
//   text     : string —— 分组标题文案（已 translate，bridge cork_to_utf8 还原）。
//   isFirst  : bool   —— 是否该 tab/区段/列的首个分组；true 时顶部不留间距、不显示上方分隔线。
//   isNarrow : bool   —— 是否落在双栏半宽列；true 时为列内紧凑标题（无分隔线、小间距）。
//
// 宽度由父行给定（满宽或双栏半宽列）。

import QtQuick
import "."

Item {
    id: header

    property var text: ""
    property bool isFirst: false
    property bool isNarrow: false

    // 分隔线仅满宽非首组显示：半宽列内 header 紧凑、靠列间距分隔，不画线。
    readonly property bool showSeparator: !isFirst && !isNarrow
    // 高度 = 顶部间距（满宽非首组）+ 标题行 + 底部小间距 + 分隔线。
    height: (showSeparator ? Theme.groupHeaderTopGap : (isFirst ? 0 : Theme.padS))
            + Theme.fontGroupHeader + Theme.groupHeaderBottomPad
            + (showSeparator ? Theme.borderW : 0)
    width: parent ? parent.width : 0

    Rectangle {
        id: separator
        anchors.top: parent.top
        width: parent.width
        height: Theme.borderW
        color: Theme.borderClr
        visible: header.showSeparator
    }

    Text {
        id: title
        // 满宽非首组：在分隔线下方；其它：贴顶。
        anchors.top: parent.top
        anchors.topMargin: header.showSeparator ? Theme.groupHeaderTopGap : 0
        width: parent.width
        text: header.text || ""
        color: Theme.fg
        font.pixelSize: Theme.fontGroupHeader
        font.bold: true
        elide: Text.ElideRight
    }
}

