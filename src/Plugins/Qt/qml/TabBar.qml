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
    id: root
    spacing: 8 * Theme.scaleFactor

    property var model: []
    property string activeKey: ""
    signal selected (string key)

    Repeater {
        model: root.model
        delegate: Rectangle {
            readonly property bool isActive: root.activeKey === modelData.key
            width: tabText.width + 28 * Theme.scaleFactor
            height: 30 * Theme.scaleFactor
            radius: height / 2
            color: isActive ? Theme.selectBg
                            : (tabMa.containsMouse ? Theme.fieldBgHover : "transparent")
            border.width: isActive ? 1 * Theme.scaleFactor : 0
            border.color: Theme.selectBorder
            Text {
                id: tabText
                anchors.centerIn: parent
                text: modelData.label
                color: isActive ? Theme.selectFg : Theme.fg
                font.pixelSize: 13 * Theme.scaleFactor
                font.bold: isActive
            }
            MouseArea {
                id: tabMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.selected (modelData.key)
            }
        }
    }
}
