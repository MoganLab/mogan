// SelectableList.qml — 始终可见的可滚动单选列表（三栏 family/style/size 复用）。
// 与 EnumCombo（下拉）相对：选项常驻、单击选中、高亮当前值。tm-widget 的
// choice+scrollable 即此形态。组件层纯 QtQuick，不依赖 Controls，与同目录其它
// 原子板块同构（Theme 单例取主题）。
//
// 直角无框：容器无边框无圆角，选中态是 delegate 内嵌的圆角高亮块；标题（title）
// 渲染在容器内顶部第一行，下方为可滚动列表区。
//
// API：
//   items         : list<string>  —— 可选项。
//   currentValue  : string        —— 当前选中值（高亮 + 滚到可见）。
//   title         : string        —— 容器内顶部标题（空串则不占标题行）。
//   signal selected(string value) —— 点击新项时发出。
//
// 用法（宽度/高度由父布局给定）：
//   SelectableList {
//       width: 300; height: 350
//       title: "字体"
//       items: fontBridge.requestFamilies(); currentValue: fontBridge.currentFamily()
//       onSelected: function(v) { /* 联动 */ }
//   }
//
// 选中态用内部 activeValue 维护：currentValue 绑定到无参 bridge 函数，QML 不会因
// bridge 内部状态变化重算，故点击即时更新 activeValue 驱动高亮；currentValue 变化
// 时（reset 后重拉）同步回内部态。调用方无需手动同步。

import QtQuick

Item {
    id: root

    property var items: []
    property string currentValue: ""
    property string title: ""
    signal selected(string value)

    // 选中态（详见文件头）：点击即时更新，currentValue 变化时同步（reset 等）。
    property string activeValue: currentValue
    onCurrentValueChanged: activeValue = currentValue

    // 标题行高（空 title 时为 0，不占空间）。
    readonly property real headerH: root.title.length > 0 ? 24 * Theme.scaleFactor : 0

    // activeValue 在 items 中的下标，不在则 -1。
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

        // 标题行（容器内顶部）。
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

            // currentValue 变化或 items 切换后，把当前项滚到可见。
            onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)

            delegate: Item {
                width: list.width
                height: 36 * Theme.scaleFactor
                readonly property bool isCurrent: root.activeValue === modelData

                // 选中/hover 的圆角内嵌高亮块（对齐 HTML .list-box li.active）：
                // 选中 = 浅青底 + 边框；hover（非选中）= 浅底；否则透明。
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
