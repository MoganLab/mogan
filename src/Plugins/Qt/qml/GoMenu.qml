// GoMenu.qml — 标题栏「转到/Go」按钮弹出的精致 QML 菜单浮层。
// 仅展示最近使用的文档列表（Recent）。
// 固定优雅宽度（260），杜绝长文件名撑宽整个下拉浮层；
// 顶部标题固定不随列表滚动，文本中间省略（ElideMiddle），
// 配备精致文件类型图标与柔和微立体阴影。

import QtQuick
import "atoms"

Item {
    id: root

    readonly property var meta: (typeof goBridge !== "undefined" && goBridge && goBridge.meta) ? goBridge.meta : ({})
    readonly property string labelRecent: meta.label_recent || qsTr("Recent")
    readonly property var recent: meta.recent || []

    readonly property real itemH: 32 * Theme.scaleFactor
    readonly property real hdrH: 26 * Theme.scaleFactor
    readonly property real sepH: 1 * Theme.scaleFactor
    readonly property real spacingH: 2 * Theme.scaleFactor

    // 仅包含最近文档列表项高（若为空则占位48px）
    readonly property real recentCount: recent.length > 0 ? recent.length : 1
    readonly property real calculatedHeight: recent.length > 0 ? recentCount * (itemH + spacingH) : 48 * Theme.scaleFactor

    // 固定优雅的菜单宽度，杜绝长文件名拉宽浮层
    readonly property real menuWidth: 260 * Theme.scaleFactor
    readonly property real maxListHeight: 380 * Theme.scaleFactor
    readonly property real listHeight: Math.min(maxListHeight, calculatedHeight)

    readonly property real shadowPad: 4 * Theme.scaleFactor

    width: implicitWidth
    height: implicitHeight
    implicitWidth: menuWidth + shadowPad * 2
    implicitHeight: listHeight + hdrH + sepH + 16 * Theme.scaleFactor + shadowPad * 2 + 2 * Theme.scaleFactor

    // 渐进柔和立体阴影（多层微透明矩形弥散）
    Rectangle {
        x: menuCard.x
        y: menuCard.y + 4 * Theme.scaleFactor
        width: menuCard.width
        height: menuCard.height
        radius: menuCard.radius + 2 * Theme.scaleFactor
        color: Theme.dark ? "#50000000" : "#10000000"
    }
    Rectangle {
        x: menuCard.x
        y: menuCard.y + 2 * Theme.scaleFactor
        width: menuCard.width
        height: menuCard.height
        radius: menuCard.radius + 1 * Theme.scaleFactor
        color: Theme.dark ? "#3c000000" : "#0c000000"
    }
    Rectangle {
        x: menuCard.x
        y: menuCard.y + 1 * Theme.scaleFactor
        width: menuCard.width
        height: menuCard.height
        radius: menuCard.radius
        color: Theme.dark ? "#28000000" : "#08000000"
    }

    // 菜单主容器卡片
    Rectangle {
        id: menuCard
        x: root.shadowPad
        y: root.shadowPad
        width: root.menuWidth
        height: root.listHeight + root.hdrH + root.sepH + 16 * Theme.scaleFactor
        radius: 10 * Theme.scaleFactor
        color: Theme.bg
        border.width: 1
        border.color: Theme.dark ? "#3a3a3c" : "#e5e7eb"

        // 顶部固定标题行（不随列表滚动）
        Item {
            id: headerArea
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 4 * Theme.scaleFactor
            anchors.leftMargin: 6 * Theme.scaleFactor
            anchors.rightMargin: 6 * Theme.scaleFactor
            height: root.hdrH

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 8 * Theme.scaleFactor
                anchors.verticalCenter: parent.verticalCenter
                text: root.labelRecent
                font.pixelSize: 11 * Theme.scaleFactor
                font.weight: Font.DemiBold
                color: Theme.muted
            }
        }

        // 分隔细线
        Rectangle {
            id: headerSep
            anchors.top: headerArea.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 12 * Theme.scaleFactor
            anchors.rightMargin: 12 * Theme.scaleFactor
            height: root.sepH
            color: Theme.borderClr
            opacity: 0.4
        }

        // 可滚动的文档列表区
        Flickable {
            id: flick
            anchors.top: headerSep.bottom
            anchors.topMargin: 4 * Theme.scaleFactor
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 6 * Theme.scaleFactor
            anchors.rightMargin: 6 * Theme.scaleFactor
            anchors.bottomMargin: 6 * Theme.scaleFactor
            contentWidth: width
            contentHeight: root.calculatedHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: contentColumn
                objectName: "contentColumn"
                width: flick.width
                spacing: root.spacingH

                // 最近文档项列表
                Repeater {
                    model: root.recent
                    delegate: MenuItem {
                        width: contentColumn.width
                        text: modelData.title || ""
                        onTriggered: if (typeof goBridge !== "undefined") goBridge.loadBuffer(modelData.url)
                    }
                }

                // 空状态占位
                Item {
                    width: parent.width
                    implicitHeight: 48 * Theme.scaleFactor
                    height: implicitHeight
                    visible: root.recent.length === 0

                    Row {
                        anchors.centerIn: parent
                        spacing: 8 * Theme.scaleFactor

                        DocumentIcon {
                            anchors.verticalCenter: parent.verticalCenter
                            filename: ""
                            opacity: 0.4
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("No recent documents")
                            font.pixelSize: Theme.fontSmall
                            color: Theme.muted
                        }
                    }
                }
            }
        }

        // 简易优雅滚动指示器（置于列表区侧边）
        Rectangle {
            anchors.right: menuCard.right
            anchors.rightMargin: 3 * Theme.scaleFactor
            anchors.top: headerSep.bottom
            anchors.topMargin: 4 * Theme.scaleFactor
            anchors.bottom: menuCard.bottom
            anchors.bottomMargin: 8 * Theme.scaleFactor
            width: 2.5 * Theme.scaleFactor
            radius: 1.25 * Theme.scaleFactor
            color: "transparent"
            visible: flick.visibleArea.heightRatio < 1.0

            readonly property real thumbH: Math.max(18 * Theme.scaleFactor, flick.visibleArea.heightRatio * parent.height)

            Rectangle {
                y: Math.min(parent.height - parent.thumbH, flick.visibleArea.yPosition * parent.height)
                width: parent.width
                height: parent.thumbH
                radius: parent.radius
                color: Theme.muted
                opacity: 0.35
            }
        }
    }

    // 单个文档类型图标组件
    component DocumentIcon: Item {
        id: iconRoot
        property string filename: ""
        width: 14 * Theme.scaleFactor
        height: 16 * Theme.scaleFactor

        readonly property string lower: filename.toLowerCase()
        readonly property bool isPdf: lower.endsWith(".pdf")
        readonly property bool isStem: lower.endsWith(".stem") || lower.endsWith(".tmu")
        readonly property bool isMd: lower.endsWith(".md") || lower.endsWith(".markdown")
        readonly property color tagColor: isPdf ? (Theme.dark ? "#ef5350" : "#d32f2f")
                                               : (isStem ? (Theme.dark ? "#4db6ac" : "#197672")
                                                         : (isMd ? (Theme.dark ? "#64b5f6" : "#1976d2")
                                                                 : (Theme.dark ? "#90a4ae" : "#78909c")))

        Rectangle {
            anchors.fill: parent
            radius: 2 * Theme.scaleFactor
            color: Theme.dark ? "#333333" : "#ffffff"
            border.width: 1
            border.color: Theme.dark ? "#555555" : "#d0d4da"

            // 顶部文件类型色条
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 1
                height: 3.5 * Theme.scaleFactor
                radius: 1 * Theme.scaleFactor
                color: iconRoot.tagColor
            }

            // 内嵌文档排版纹理线条
            Column {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: 1.5 * Theme.scaleFactor
                spacing: 1.5 * Theme.scaleFactor

                Rectangle {
                    width: 7 * Theme.scaleFactor
                    height: 1 * Theme.scaleFactor
                    radius: 0.5 * Theme.scaleFactor
                    color: Theme.dark ? "#777777" : "#b0b8c0"
                }
                Rectangle {
                    width: 5 * Theme.scaleFactor
                    height: 1 * Theme.scaleFactor
                    radius: 0.5 * Theme.scaleFactor
                    color: Theme.dark ? "#777777" : "#b0b8c0"
                }
            }
        }
    }

    // 单个菜单项组件
    component MenuItem: Item {
        id: itemRoot
        objectName: "goMenuItem"
        property string text: ""
        readonly property bool isHovered: mouseArea.containsMouse
        readonly property string fileExt: {
            var name = itemRoot.text;
            var idx = name.lastIndexOf(".");
            return (idx > 0 && idx < name.length - 1) ? name.substring(idx + 1).toLowerCase() : "";
        }
        signal triggered()

        implicitHeight: root.itemH
        height: implicitHeight

        // 浅灰悬浮高亮背景（通过 opacity 渐变，杜绝 transparent 颜色插值穿透黑色引起的深灰色闪烁）
        Rectangle {
            id: bgRect
            objectName: "goMenuItemBg"
            anchors.fill: parent
            radius: 6 * Theme.scaleFactor
            color: Theme.fieldBgHover
            opacity: itemRoot.isHovered ? 1.0 : 0.0

            Behavior on opacity {
                NumberAnimation { duration: 100 }
            }
        }

        DocumentIcon {
            id: docIcon
            anchors.left: parent.left
            anchors.leftMargin: 8 * Theme.scaleFactor
            anchors.verticalCenter: parent.verticalCenter
            filename: itemRoot.text
        }

        // 右侧文件类型标签（仅标注最后一级后缀）
        Rectangle {
            id: typeTag
            objectName: "goMenuItemTypeTag"
            anchors.right: parent.right
            anchors.rightMargin: 8 * Theme.scaleFactor
            anchors.verticalCenter: parent.verticalCenter
            visible: itemRoot.fileExt.length > 0
            implicitWidth: tagText.implicitWidth + 8 * Theme.scaleFactor
            implicitHeight: 16 * Theme.scaleFactor
            width: implicitWidth
            height: implicitHeight
            radius: 3 * Theme.scaleFactor
            color: itemRoot.isHovered ? (Theme.dark ? "#4a4a4c" : "#dbe0e6")
                                      : (Theme.dark ? "#38383a" : "#eceff1")

            Text {
                id: tagText
                objectName: "goMenuItemTagText"
                anchors.centerIn: parent
                text: itemRoot.fileExt
                font.pixelSize: 10 * Theme.scaleFactor
                font.weight: Font.DemiBold
                color: itemRoot.isHovered ? (Theme.dark ? "#d4d4d8" : "#475569")
                                          : (Theme.dark ? "#a1a1aa" : "#64748b")
            }
        }

        Text {
            id: labelText
            anchors.left: docIcon.right
            anchors.leftMargin: 8 * Theme.scaleFactor
            anchors.right: typeTag.visible ? typeTag.left : parent.right
            anchors.rightMargin: typeTag.visible ? 6 * Theme.scaleFactor : 8 * Theme.scaleFactor
            anchors.verticalCenter: parent.verticalCenter
            text: itemRoot.text
            font.pixelSize: 12 * Theme.scaleFactor
            color: itemRoot.isHovered ? (Theme.dark ? "#ffffff" : "#0f172a")
                                      : (Theme.dark ? "#d1d5db" : "#334155")
            elide: Text.ElideMiddle
        }

        MouseArea {
            id: mouseArea
            objectName: "goMenuItemMouseArea"
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onPressed: itemRoot.scale = 0.98
            onReleased: itemRoot.scale = 1.0
            onCanceled: itemRoot.scale = 1.0
            onClicked: itemRoot.triggered()
        }
    }

    focus: true
    Keys.onEscapePressed: {
        if (typeof goBridge !== "undefined") goBridge.closeMenu();
    }
}
