// SelectableList.qml — 始终可见的可滚动单选列表（三栏 family/style/size 复用）。
// 与 EnumCombo（下拉）相对：选项常驻、单击选中、高亮当前值。纯 QtQuick，不依赖
// Controls。标题渲染在容器内顶部第一行，下方为可滚动列表区；选中态是 delegate
// 内嵌的圆角高亮块。
//
// API：
//   items        : list<string>  —— 可选项。
//   currentValue : string        —— 当前选中值（高亮 + 滚到可见）。
//   title        : string        —— 容器内顶部标题（空串不占标题行）。
//   selected(string value)       —— 点击新项时发出。
//
// 用法（宽度/高度由父布局给定）：
//   SelectableList {
//       width: 300; height: 350; title: "字体"
//       items: fontBridge.requestFamilies(); currentValue: fontBridge.currentFamily()
//       onSelected: function(v) { /* 写回 + 联动 */ }
//   }
//
// 选中态用内部 activeValue 维护：currentValue 绑定到无参 bridge 函数，QML 不会因
// bridge 内部状态变化重算，故点击即时更新 activeValue 驱动高亮。reset 后调用方需
// 显式 syncActiveValue()（详见下）。

import QtQuick

Item {
    id: root

    property var items: []
    property string currentValue: ""
    property string title: ""
    signal selected(string value)

    // 点击即时更新 activeValue 驱动高亮；currentValue 变化时同步回内部态。
    property string activeValue: currentValue
    onCurrentValueChanged: activeValue = currentValue

    // reset 后 currentValue 可能「值未变」（回到打开时默认），changed 信号不发、
    // activeValue 不更新——调用方在 refreshAll 后显式调用。
    function syncActiveValue() { activeValue = currentValue }

    readonly property real headerH: root.title.length > 0 ? 24 * Theme.scaleFactor : 0

    readonly property int currentIndex: {
        for (var i = 0; i < items.length; i++)
            if (items[i] === activeValue) return i
        return -1
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.listBg
        radius: 8 * Theme.scaleFactor
        border.width: 1 * Theme.scaleFactor
        border.color: Theme.borderClr
        clip: true

        Text {
            id: header
            visible: root.title.length > 0
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: root.headerH
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8 * Theme.scaleFactor
            text: root.title
            color: Theme.fg
            font.bold: true
            font.pixelSize: 14 * Theme.scaleFactor
        }

        ListView {
            id: list
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            clip: true
            interactive: true
            boundsBehavior: Flickable.StopAtBounds
            model: root.items
            currentIndex: root.currentIndex

            onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)

            delegate: Item {
                width: list.width
                height: 36 * Theme.scaleFactor
                readonly property bool isCurrent: root.activeValue === modelData

                Rectangle {
                    id: hilite
                    anchors.fill: parent
                    anchors.leftMargin: 8 * Theme.scaleFactor
                    anchors.rightMargin: 8 * Theme.scaleFactor
                    anchors.topMargin: 4 * Theme.scaleFactor
                    anchors.bottomMargin: 4 * Theme.scaleFactor
                    radius: 8 * Theme.scaleFactor
                    color: isCurrent ? Theme.selectBg
                                     : (ma.containsMouse ? Theme.fieldBgHover : "transparent")
                    border.width: isCurrent ? 1 * Theme.scaleFactor : 0
                    border.color: Theme.selectBorder
                }

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 20 * Theme.scaleFactor
                    anchors.rightMargin: 18 * Theme.scaleFactor
                    verticalAlignment: Text.AlignVCenter
                    text: modelData
                    color: isCurrent ? Theme.selectFg : Theme.fg
                    font.pixelSize: 14 * Theme.scaleFactor
                    elide: Text.ElideRight
                }
                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!isCurrent) {
                            root.activeValue = modelData
                            root.selected(modelData)
                        }
                    }
                }
            }
        }
    }
}
