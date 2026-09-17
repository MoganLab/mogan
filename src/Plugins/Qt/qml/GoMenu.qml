// GoMenu.qml — 标题栏「Go」按钮弹出的 QML 菜单浮层。
// 仅展示最近使用的文档列表（Recent）。
// 按钮点击经 goBridge 调用 loadBuffer 快速打开文档，并关闭菜单。

import QtQuick
import "atoms"

Item {
    id: root

    readonly property var meta: (typeof goBridge !== "undefined" && goBridge && goBridge.meta) ? goBridge.meta : ({})
    readonly property string labelRecent: meta.label_recent || qsTr("Recent")
    readonly property var recent: meta.recent || []

    readonly property real itemH: 30 * Theme.scaleFactor
    readonly property real hdrH: 24 * Theme.scaleFactor
    readonly property real sepH: 6 * Theme.scaleFactor
    readonly property real spacingH: 2 * Theme.scaleFactor

    // 仅包含最近文档列表：标题 + 分隔 + N项（若为空则占位1项）
    readonly property real recentCount: recent.length > 0 ? recent.length : 1
    readonly property real calculatedHeight: hdrH + sepH + recentCount * (itemH + spacingH)

    readonly property real menuWidth: Math.max(260 * Theme.scaleFactor,
        Math.min(420 * Theme.scaleFactor, contentColumn.implicitWidth + 24 * Theme.scaleFactor))
    readonly property real maxListHeight: 460 * Theme.scaleFactor
    readonly property real actualContentHeight: Math.max(calculatedHeight, contentColumn.childrenRect.height)
    readonly property real listHeight: Math.min(maxListHeight, actualContentHeight)

    width: implicitWidth
    height: implicitHeight
    implicitWidth: menuWidth + 4 * Theme.scaleFactor
    implicitHeight: listHeight + 16 * Theme.scaleFactor

    // 渐进柔和阴影
    Rectangle {
        x: 0
        y: 3 * Theme.scaleFactor
        width: menuCard.width
        height: menuCard.height
        radius: Theme.radius + 1
        color: Theme.dark ? "#60000000" : "#18000000"
    }
    Rectangle {
        x: 0
        y: 1 * Theme.scaleFactor
        width: menuCard.width
        height: menuCard.height
        radius: Theme.radius
        color: Theme.dark ? "#40000000" : "#12000000"
    }

    // 菜单主容器
    Rectangle {
        id: menuCard
        width: root.menuWidth
        height: root.listHeight + 12 * Theme.scaleFactor
        radius: Theme.radius
        color: Theme.bg
        border.width: 1
        border.color: Theme.borderClr

        Flickable {
            id: flick
            anchors.fill: parent
            anchors.margins: 6 * Theme.scaleFactor
            contentWidth: width
            contentHeight: root.actualContentHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: contentColumn
                objectName: "contentColumn"
                width: flick.width
                spacing: root.spacingH

                // 最近使用 标题
                Item {
                    width: parent.width
                    implicitHeight: root.hdrH
                    height: implicitHeight

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 8 * Theme.scaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.labelRecent
                        font.pixelSize: Theme.fontTiny
                        font.bold: true
                        color: Theme.muted
                    }
                }

                // 分隔细线
                Item {
                    width: parent.width
                    implicitHeight: root.sepH
                    height: implicitHeight

                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width - 8 * Theme.scaleFactor
                        height: 1
                        color: Theme.borderClr
                        opacity: 0.6
                    }
                }

                // 最近文档项列表
                Repeater {
                    model: root.recent
                    delegate: MenuItem {
                        width: contentColumn.width
                        text: modelData.title || ""
                        subText: modelData.url || ""
                        onTriggered: if (typeof goBridge !== "undefined") goBridge.loadBuffer(modelData.url)
                    }
                }

                // 空状态占位
                Item {
                    width: parent.width
                    implicitHeight: root.itemH
                    height: implicitHeight
                    visible: root.recent.length === 0

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 10 * Theme.scaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("No recent documents")
                        font.pixelSize: Theme.fontSmall
                        color: Theme.muted
                    }
                }
            }
        }

        // 简易滚动指示器
        Rectangle {
            anchors.right: menuCard.right
            anchors.rightMargin: 2 * Theme.scaleFactor
            anchors.top: menuCard.top
            anchors.topMargin: 4 * Theme.scaleFactor
            anchors.bottom: menuCard.bottom
            anchors.bottomMargin: 4 * Theme.scaleFactor
            width: 3 * Theme.scaleFactor
            radius: 1.5 * Theme.scaleFactor
            color: "transparent"
            visible: flick.visibleArea.heightRatio < 1.0

            Rectangle {
                y: flick.visibleArea.yPosition * parent.height
                width: parent.width
                height: Math.max(16 * Theme.scaleFactor, flick.visibleArea.heightRatio * parent.height)
                radius: parent.radius
                color: Theme.muted
                opacity: 0.5
            }
        }
    }

    // 单个菜单项组件
    component MenuItem: Rectangle {
        id: itemRoot
        property string text: ""
        property string subText: ""
        property bool enabled: true
        signal triggered()

        implicitHeight: root.itemH
        height: implicitHeight
        implicitWidth: 260 * Theme.scaleFactor
        radius: 6 * Theme.scaleFactor
        color: mouseArea.containsMouse && itemRoot.enabled ? Theme.fieldBgHover : "transparent"

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: itemRoot.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: {
                if (itemRoot.enabled) {
                    itemRoot.triggered();
                }
            }
        }

        Row {
            anchors.fill: parent
            anchors.leftMargin: 10 * Theme.scaleFactor
            anchors.rightMargin: 10 * Theme.scaleFactor
            spacing: 6 * Theme.scaleFactor

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: itemRoot.text
                font.pixelSize: Theme.fontSmall
                color: itemRoot.enabled ? Theme.fg : Theme.muted
                elide: Text.ElideMiddle
                width: parent.width
            }
        }
    }

    focus: true
    Keys.onEscapePressed: {
        if (typeof goBridge !== "undefined") goBridge.closeMenu();
    }
}
