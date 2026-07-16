// SelectableList.qml — 始终可见的可滚动单选列表（三栏 family/style/size 复用）。
// 与 EnumCombo（下拉）相对：选项常驻、单击选中、高亮当前值。纯 QtQuick，不依赖
// Controls。标题渲染在容器内顶部第一行，下方为可滚动列表区；选中态是 delegate
// 内嵌的圆角高亮块。
//
// API：
//   items        : list<string>  —— 可选项。
//   currentValue : string        —— 当前选中值（高亮 + 滚到可见）。
//   title        : string        —— 容器内顶部标题（空串不占标题行）。
//   refreshTick  : int           —— 外部重算计数器（见下）。
//   selected(string value)       —— 点击新项时发出。
//
// 用法（宽度/高度由父布局给定）：
//   SelectableList {
//       id: familyList; width: 300; height: 350; title: "字体"
//       items: familyModel.value
//       // currentValue 绑定到无参 bridge 函数，读 refreshTick 注入重算依赖。
//       currentValue: { familyList.refreshTick; return fontBridge.currentFamily() }
//       onSelected: function(v) { /* 写回 + 联动 */ }
//   }
//
// currentValue 绑定到无参 bridge 函数时，bridge 内部状态变化 QML 感知不到、绑定不重算。
// refreshTick 是外部注入的重算依赖：调用方 reset/refresh 时 `listId.refreshTick++`，
// 绑定即重算。选中态（activeValue）随 currentValue 变化同步；值未变时改发信号，调用方
// 需显式 syncActiveValue()（见下）。

import QtQuick

Item {
    id: selList

    property var items: []
    property string currentValue: ""
    property string title: ""
    property int refreshTick: 0
    signal selected(string value)

    // 点击即时更新 activeValue 驱动高亮；currentValue 变化时同步回内部态。
    property string activeValue: currentValue
    onCurrentValueChanged: activeValue = currentValue

    // reset 后 currentValue 可能「值未变」（回到打开时默认），changed 信号不发、
    // activeValue 不更新——调用方在 refreshAll 后显式同步。
    function syncActiveValue() {
        activeValue = currentValue;
    }

    readonly property real headerH: selList.title.length > 0 ? Theme.headerH : 0

    readonly property int currentIndex: {
        for (var i = 0; i < items.length; i++)
            if (items[i] === activeValue)
                return i;
        return -1;
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.listBg
        radius: Theme.radius
        border.width: Theme.borderW
        border.color: Theme.borderClr
        clip: true

        Text {
            id: header
            visible: selList.title.length > 0
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: selList.headerH
            verticalAlignment: Text.AlignVCenter
            leftPadding: Theme.pad
            text: selList.title
            color: Theme.fg
            font.bold: true
            font.pixelSize: Theme.fontBody
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
            model: selList.items
            currentIndex: selList.currentIndex

            onCurrentIndexChanged: if (currentIndex >= 0)
                positionViewAtIndex(currentIndex, ListView.Contain)

            delegate: Item {
                width: list.width
                height: Theme.itemH
                readonly property bool isCurrent: selList.activeValue === modelData

                Rectangle {
                    id: hilite
                    anchors.fill: parent
                    anchors.leftMargin: Theme.pad
                    anchors.rightMargin: Theme.pad
                    anchors.topMargin: Theme.padS
                    anchors.bottomMargin: Theme.padS
                    radius: Theme.radius
                    color: isCurrent ? Theme.selectBg : (ma.containsMouse ? Theme.fieldBgHover : "transparent")
                    border.width: isCurrent ? Theme.borderW : 0
                    border.color: Theme.selectBorder
                }

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.listTextPadL
                    anchors.rightMargin: Theme.listTextPadR
                    verticalAlignment: Text.AlignVCenter
                    text: modelData
                    color: isCurrent ? Theme.selectFg : Theme.fg
                    font.pixelSize: Theme.fontBody
                    elide: Text.ElideRight
                }
                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!isCurrent) {
                            selList.activeValue = modelData;
                            selList.selected(modelData);
                        }
                    }
                }
            }
        }
    }
}
