// TabPanel.qml — 带选项卡的容器面板（FontSelector 右栏复用）。
// 直角无框容器（listBg 底 + 边框），顶部 TabBar，下方 content 槽（占满剩余高度）。
// 调用方提供 tabs（选项卡定义）与 content（每个 tab 的内容 Item）；TabPanel 管
// activeKey 状态、TabBar 渲染、content 区锚定。
//
// API：
//   tabs       : list<{key,label}>  —— 选项卡项（透传给 TabBar）。
//   content    : Item               —— 内容（调用方按 activeKey 自行切换；挂载时由本
//                                       组件 reparent 到内容区并锚定，勿自行 anchors.fill）。
//   activeKey  : string             —— 当前选中 tab 的 key（TabPanel 持有，可双向绑定）。
//
// 样式（容器 radius 8、border、tabBar topMargin/leftMargin 8、内容四周 margin 8）与原
// FontSelector 内联 tabPanel 逐项一致。

import QtQuick
import "."

Rectangle {
    id: root
    color: Theme.listBg
    radius: 8 * Theme.scaleFactor
    border.width: 1 * Theme.scaleFactor
    border.color: Theme.borderClr
    clip: true

    property var tabs: []
    property Item content
    property string activeKey: tabs.length > 0 ? tabs[0].key : ""

    onContentChanged: {
        if (!content) return
        content.parent = contentArea
        content.anchors.fill = contentArea
    }

    TabBar {
        id: tabBar
        anchors.top: parent.top
        anchors.topMargin: 8 * Theme.scaleFactor
        anchors.left: parent.left
        anchors.leftMargin: 8 * Theme.scaleFactor
        model: root.tabs
        activeKey: root.activeKey
        onSelected: function (key) { root.activeKey = key }
    }

    // 内容区：tabBar 下方到底，四周留 8 内缩进。调用方的 content 被挂到此处。
    Item {
        id: contentArea
        anchors.top: tabBar.bottom
        anchors.topMargin: 8 * Theme.scaleFactor
        anchors.left: parent.left
        anchors.leftMargin: 8 * Theme.scaleFactor
        anchors.right: parent.right
        anchors.rightMargin: 8 * Theme.scaleFactor
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8 * Theme.scaleFactor
    }
}
