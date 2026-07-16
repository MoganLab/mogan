// TabPanel.qml — 带选项卡的容器面板（FontSelector 右栏复用）。
// 顶部 TabBar，下方 content 槽（占满剩余高度）。TabPanel 管 activeKey 状态、
// TabBar 渲染、content 区锚定。
//
// API：
//   tabs     : list<{key,label}> —— 选项卡项（透传给 TabBar）。
//   content  : Item              —— 调用方按 activeKey 自行切换内容；挂载时由本组件
//                                   reparent 到内容区并锚定，勿自行 anchors.fill。
//   activeKey: string            —— 当前选中 tab 的 key（TabPanel 持有，可双向绑定）。
//
// 用法：
//   TabPanel {
//       anchors.fill: parent
//       tabs: [{key:"filter",label:"Filter"}, {key:"adv",label:"Advanced"}]
//       activeKey: root.tab; onActiveKeyChanged: root.tab = activeKey
//       content: Item { /* 按 activeKey 切显隐的子项 */ }
//   }

import QtQuick
import "."

Rectangle {
    id: panel
    color: Theme.listBg
    radius: 8 * Theme.scaleFactor
    border.width: 1 * Theme.scaleFactor
    border.color: Theme.borderClr
    clip: true

    property var tabs: []
    property Item content
    property string activeKey: tabs.length > 0 ? tabs[0].key : ""

    onContentChanged: {
        if (!content)
            return;
        content.parent = contentArea;
        content.anchors.fill = contentArea;
    }

    TabBar {
        id: tabBar
        anchors.top: parent.top
        anchors.topMargin: 8 * Theme.scaleFactor
        anchors.left: parent.left
        anchors.leftMargin: 8 * Theme.scaleFactor
        model: panel.tabs
        activeKey: panel.activeKey
        onSelected: function (key) {
            panel.activeKey = key;
        }
    }

    // 调用方的 content 被挂到此处。
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
