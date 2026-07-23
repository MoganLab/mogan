// TabBar.qml — 胶囊选项卡行（FontSelector 右栏 Filter/Advanced 切换复用）。
// 选中态为圆角胶囊（selectBg/selectFg/边框），色调与 SelectableList 选中一致。
// 纯 QtQuick。
//
// API：
//   model    : list<{key,label}> —— 选项卡项。
//   activeKey: string            —— 当前选中项的 key（高亮它）。
//   selected(string key)         —— 点击新项时发出。
//
// 用法：
//   TabBar {
//       model: [{key:"a",label:"A"}, {key:"b",label:"B"}]; activeKey: root.tab
//       onSelected: function(k) { root.tab = k }
//   }

import QtQuick
import "."

Row {
    id: bar
    spacing: Theme.pad

    property var model: []
    property string activeKey: ""
    signal selected(string key)

    Repeater {
        model: bar.model
        delegate: Rectangle {
            readonly property bool isActive: bar.activeKey === modelData.key
            width: tabText.width + Theme.tabPad
            height: Theme.tabH
            radius: height / 2
            color: isActive ? Theme.selectBg : (tabMa.containsMouse ? Theme.fieldBgHover : "transparent")
            border.width: isActive ? Theme.borderW : 0
            border.color: Theme.selectBorder
            Text {
                id: tabText
                anchors.centerIn: parent
                text: modelData.label
                color: isActive ? Theme.selectFg : Theme.fg
                font.pixelSize: Theme.fontTab
            }
            MouseArea {
                id: tabMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: bar.selected(modelData.key)
            }
        }
    }
}
