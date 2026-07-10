// SelectableList.qml — 始终可见的可滚动单选列表（三栏 family/style/size 复用）。
// 与 EnumCombo（下拉）相对：选项常驻、单击选中、高亮当前值。tm-widget 的
// choice+scrollable 即此形态。组件层纯 QtQuick，不依赖 Controls，与同目录其它
// 原子板块同构（Theme 单例取主题）。
//
// API：
//   items         : list<string>  —— 可选项。
//   currentValue  : string        —— 当前选中值（高亮 + 滚到可见）。
//   signal selected(string value) —— 点击新项时发出。
//
// 用法（宽度/高度由父布局给定）：
//   SelectableList {
//       width: 300; height: 350
//       items: fontBridge.requestFamilies(); currentValue: fontBridge.currentFamily()
//       onSelected: function(v) { /* 联动 */ }
//   }
//
// model 切换后 currentIndex 须据 currentValue 重算（scheme 是状态真相源，非 QML
// index）—— 由 currentValue 绑定 + onItemsChanged 自动重算，调用方无需手动同步。

import QtQuick

Item {
    id: root

    property var items: []
    property string currentValue: ""
    signal selected(string value)

    // currentValue 在 items 中的下标，不在则 -1。
    readonly property int currentIndex: {
        for (var i = 0; i < items.length; i++)
            if (items[i] === currentValue) return i
        return -1
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.fieldBg
        radius: 8 * Theme.scaleFactor
        border.width: 1 * Theme.scaleFactor
        border.color: Theme.borderClr
        clip: true

        ListView {
            id: list
            anchors.fill: parent
            clip: true
            interactive: true
            boundsBehavior: Flickable.StopAtBounds
            model: root.items
            currentIndex: root.currentIndex

            // currentValue 变化或 items 切换后，把当前项滚到可见。
            onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)

            delegate: Rectangle {
                width: list.width
                height: 36 * Theme.scaleFactor
                // 当前值高亮（accent 底 + 白字），hover 浅高亮。
                color: isCurrent ? Theme.accent
                                 : (ma.containsMouse ? Theme.fieldBgHover : Theme.fieldBg)
                readonly property bool isCurrent: root.currentValue === modelData

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 14 * Theme.scaleFactor
                    anchors.rightMargin: 10 * Theme.scaleFactor
                    verticalAlignment: Text.AlignVCenter
                    text: modelData
                    color: isCurrent ? "#ffffff" : Theme.fg
                    font.pixelSize: 14 * Theme.scaleFactor
                    elide: Text.ElideRight
                }
                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!isCurrent) root.selected(modelData)
                    }
                }
            }
        }
    }
}
