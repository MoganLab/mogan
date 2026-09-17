// GoMenu.qml — 标题栏「Go」按钮弹出的 QML 菜单浮层。
// 显示光标历史操作（Back / Forward / Save position）、打开的文档列表与最近文档列表。
// 按钮点击经 goBridge 调用对应 Scheme 函数，并关闭菜单。

import QtQuick
import "atoms"

Item {
    id: root

    readonly property var meta: (typeof goBridge !== "undefined" && goBridge && goBridge.meta) ? goBridge.meta : ({})
    readonly property bool canBack: meta.can_back === true || meta.can_back === "true"
    readonly property bool canForward: meta.can_forward === true || meta.can_forward === "true"
    readonly property string labelBack: meta.label_back || qsTr("Back")
    readonly property string labelForward: meta.label_forward || qsTr("Forward")
    readonly property string labelSave: meta.label_save || qsTr("Save position")
    readonly property string labelBuffers: meta.label_buffers || qsTr("Open documents")
    readonly property string labelRecent: meta.label_recent || qsTr("Recent")
    readonly property var buffers: meta.buffers || []
    readonly property var recent: meta.recent || []

    readonly property real itemH: 32 * Theme.scaleFactor
    readonly property real sepH: 9 * Theme.scaleFactor
    readonly property real hdrH: 22 * Theme.scaleFactor
    readonly property real spacingH: 2 * Theme.scaleFactor

    // 3 个基础导航项（Back / Forward / Save position）
    readonly property real navHeight: 3 * itemH + 2 * spacingH
    // 打开文档列表高度
    readonly property real buffersHeight: buffers.length > 0
        ? (sepH + spacingH + hdrH + spacingH + buffers.length * (itemH + spacingH))
        : 0
    // 最近文档列表高度
    readonly property real recentHeight: recent.length > 0
        ? ((buffers.length > 0 ? (sepH + spacingH) : (sepH + spacingH)) + hdrH + spacingH + recent.length * (itemH + spacingH))
        : 0
    readonly property real calculatedHeight: navHeight
        + (buffers.length > 0 ? (sepH + spacingH + hdrH + spacingH + buffers.length * (itemH + spacingH)) : 0)
        + (recent.length > 0 ? ((buffers.length > 0 ? sepH + spacingH : 0) + hdrH + spacingH + recent.length * (itemH + spacingH)) : 0)

    readonly property real menuWidth: Math.max(280 * Theme.scaleFactor,
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
                spacing: 2 * Theme.scaleFactor

                // 1. 导航项：Back
                MenuItem {
                    width: parent.width
                    text: root.labelBack
                    iconText: "←"
                    enabled: root.canBack
                    onTriggered: if (root.canBack && typeof goBridge !== "undefined") goBridge.goBack()
                }

                // 2. 导航项：Forward
                MenuItem {
                    width: parent.width
                    text: root.labelForward
                    iconText: "→"
                    enabled: root.canForward
                    onTriggered: if (root.canForward && typeof goBridge !== "undefined") goBridge.goForward()
                }

                // 3. 导航项：Save position
                MenuItem {
                    width: parent.width
                    text: root.labelSave
                    iconText: "★"
                    enabled: true
                    onTriggered: if (typeof goBridge !== "undefined") goBridge.savePosition()
                }

                // 分隔线
                MenuSeparator {
                    width: parent.width
                    visible: root.buffers.length > 0 || root.recent.length > 0
                }

                // 4. 打开的文档列表
                MenuSectionHeader {
                    width: parent.width
                    text: root.labelBuffers
                    visible: root.buffers.length > 0
                }

                Repeater {
                    model: root.buffers
                    delegate: MenuItem {
                        width: contentColumn.width
                        text: modelData.title || ""
                        subText: modelData.url || ""
                        isChecked: modelData.current === true || modelData.current === "true"
                        enabled: true
                        onTriggered: if (typeof goBridge !== "undefined") goBridge.switchToBuffer(modelData.url)
                    }
                }

                // 分隔线
                MenuSeparator {
                    width: parent.width
                    visible: root.buffers.length > 0 && root.recent.length > 0
                }

                // 5. 最近文档列表
                MenuSectionHeader {
                    width: parent.width
                    text: root.labelRecent
                    visible: root.recent.length > 0
                }

                Repeater {
                    model: root.recent
                    delegate: MenuItem {
                        width: contentColumn.width
                        text: modelData.title || ""
                        subText: modelData.url || ""
                        enabled: true
                        onTriggered: if (typeof goBridge !== "undefined") goBridge.loadBuffer(modelData.url)
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
        property string iconText: ""
        property bool isChecked: false
        property bool enabled: true
        signal triggered()

        implicitHeight: 32 * Theme.scaleFactor
        height: implicitHeight
        implicitWidth: 280 * Theme.scaleFactor
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
            anchors.leftMargin: 8 * Theme.scaleFactor
            anchors.rightMargin: 8 * Theme.scaleFactor
            spacing: 8 * Theme.scaleFactor

            Item {
                width: 18 * Theme.scaleFactor
                height: parent.height

                Text {
                    anchors.centerIn: parent
                    text: itemRoot.isChecked ? "✓" : itemRoot.iconText
                    font.pixelSize: Theme.fontSmall
                    font.bold: itemRoot.isChecked
                    color: itemRoot.isChecked ? (Theme.dark ? "#4ec9b0" : "#0e7068")
                         : (itemRoot.enabled ? Theme.fg : Theme.muted)
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: itemRoot.text
                font.pixelSize: Theme.fontSmall
                font.bold: itemRoot.isChecked
                color: itemRoot.enabled ? Theme.fg : Theme.muted
                elide: Text.ElideMiddle
                width: parent.width - 26 * Theme.scaleFactor
            }
        }
    }

    // 分隔线组件
    component MenuSeparator: Item {
        implicitHeight: 9 * Theme.scaleFactor
        height: implicitHeight
        implicitWidth: 280 * Theme.scaleFactor
        Rectangle {
            anchors.centerIn: parent
            width: parent.width
            height: 1
            color: Theme.borderClr
            opacity: 0.7
        }
    }

    // 分组标题组件
    component MenuSectionHeader: Item {
        property string text: ""
        implicitHeight: 22 * Theme.scaleFactor
        height: implicitHeight
        implicitWidth: 280 * Theme.scaleFactor
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 8 * Theme.scaleFactor
            anchors.verticalCenter: parent.verticalCenter
            text: parent.text
            font.pixelSize: Theme.fontTiny
            font.bold: true
            color: Theme.muted
        }
    }

    focus: true
    Keys.onEscapePressed: {
        if (typeof goBridge !== "undefined") goBridge.closeMenu();
    }
}
