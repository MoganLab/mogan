// StartupHomePage.qml — 启动页 Home 页面。
// 对应 C++ QTMHomePage：Document Style 样式卡片 + 最近文档列表。
//
// 页面整体不滚动，仅 Recent Documents 内部滚动。

import QtQuick
import "atoms"

Item {
    id: page

    // 数据模型（来自 bridge，fallback 为空）
    property var styleCards: typeof startupBridge !== "undefined" && startupBridge.styleCards ? startupBridge.styleCards : []
    property var recentDocs: typeof startupBridge !== "undefined" && startupBridge.recentDocs ? startupBridge.recentDocs : []

    // 内容区水平内边距统一应用到一个覆盖层，竖直方向分区用锚点堆叠。
    property real padH: StartupTheme.contentPadH

    // ---- Document Style 区块（顶部固定） ----
    Item {
        id: topSection
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            leftMargin: page.padH
            rightMargin: page.padH
            topMargin: StartupTheme.contentPadTop
        }
        height: childrenRect.height

        Text {
            id: sectionTitle
            anchors {
                left: parent.left
                top: parent.top
            }
            text: StartupTheme.tr("Document Style")
            color: StartupTheme.sectionTitleFg
            font.pixelSize: StartupTheme.fontSectionTitle
            font.weight: Font.DemiBold
        }

        // 样式卡片行（Flow 布局，自动换行）
        Flow {
            id: cardsFlow
            anchors {
                left: parent.left
                right: parent.right
                top: sectionTitle.bottom
                topMargin: 16 * StartupTheme.scaleFactor
            }
            spacing: StartupTheme.gapCards

            // 新建文档（图标模式，固定）
            StyleCard {
                kind: "icon"
                iconSrc: "qrc:/startup-tab/new-file.svg"
                cardName: StartupTheme.tr("New document")
                onClicked: {
                    if (typeof startupBridge !== "undefined") startupBridge.newDocument()
                }
            }

            // 打开文档（图标模式，固定）
            StyleCard {
                kind: "icon"
                iconSrc: "qrc:/startup-tab/open-file.svg"
                cardName: StartupTheme.tr("Open document")
                onClicked: {
                    if (typeof startupBridge !== "undefined") startupBridge.openDocument()
                }
            }

            // 推荐模板卡片（缩略图模式，动态）
            Repeater {
                model: page.styleCards
                delegate: StyleCard {
                    kind: "thumbnail"
                    titleText: modelData.titleText || modelData.name
                    thumbSrc: modelData.thumbSrc || ""
                    onClicked: {
                        if (typeof startupBridge !== "undefined") startupBridge.openTemplate(modelData.id)
                    }
                }
            }
        }
    }

    // ---- 分隔线 ----
    Rectangle {
        id: separator
        anchors {
            left: parent.left
            right: parent.right
            top: topSection.bottom
            topMargin: StartupTheme.sectionGap
            leftMargin: page.padH
            rightMargin: page.padH
        }
        height: 1
        color: StartupTheme.separatorColor
    }

    // ---- Recent Documents 标题 ----
    Text {
        id: recentTitle
        anchors {
            left: parent.left
            top: separator.bottom
            topMargin: StartupTheme.sectionGap
            leftMargin: page.padH
        }
        text: StartupTheme.tr("Recent Documents")
        color: StartupTheme.sectionTitleFg
        font.pixelSize: StartupTheme.fontSectionTitle
        font.weight: Font.DemiBold
    }

    // ---- 最近文档列表（占满剩余高度，内部滚动） ----
    Rectangle {
        id: recentContainer
        anchors {
            left: parent.left
            right: parent.right
            top: recentTitle.bottom
            topMargin: 16 * StartupTheme.scaleFactor
            bottom: parent.bottom
            bottomMargin: StartupTheme.contentPadBottom
            leftMargin: page.padH
            rightMargin: page.padH
        }
        radius: StartupTheme.recentRadius
        color: StartupTheme.recentBg
        border.width: 1
        border.color: StartupTheme.recentBorder

        ListView {
            id: recentList
            anchors.fill: parent
            anchors.leftMargin: StartupTheme.recentItemMarginH
            anchors.rightMargin: StartupTheme.recentItemMarginH
            anchors.topMargin: StartupTheme.recentItemMarginV
            anchors.bottomMargin: StartupTheme.recentItemMarginV
            spacing: 0
            clip: true
            model: page.recentDocs
            interactive: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: recentList.width - 2 * StartupTheme.recentItemMarginH
                height: StartupTheme.recentItemH
                anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                radius: StartupTheme.recentItemRadius
                color: itemMouse.containsMouse ? StartupTheme.recentHoverBg : "transparent"

                Text {
                    id: fileNameText
                    anchors {
                        left: parent.left
                        leftMargin: StartupTheme.recentItemPadH
                        verticalCenter: parent.verticalCenter
                    }
                    width: parent.width - timeText.width - 2 * StartupTheme.recentItemPadH
                    text: modelData.fileName
                    color: StartupTheme.recentNameFg
                    font.pixelSize: StartupTheme.fontRecentName
                    font.weight: Font.Bold
                    elide: Text.ElideRight
                }
                Text {
                    id: timeText
                    anchors {
                        right: parent.right
                        rightMargin: StartupTheme.recentItemPadH
                        verticalCenter: parent.verticalCenter
                    }
                    text: StartupTheme.tr("Last opened") + ": " + (modelData.openedAt || "")
                    color: StartupTheme.recentTimeFg
                    font.pixelSize: StartupTheme.fontRecentTime
                }

                MouseArea {
                    id: itemMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: Qt.PointingHandCursor
                    onClicked: function(mouse) {
                        if (mouse.button === Qt.RightButton) {
                            // 右键：从列表移除该项（原 Controls 菜单已移除，避免引入 QtQuick.Controls）
                            if (typeof startupBridge !== "undefined")
                                startupBridge.removeRecentDoc(modelData.filePath)
                        } else {
                            if (typeof startupBridge !== "undefined") startupBridge.openRecentDoc(modelData.filePath)
                        }
                    }
                }
            }

            // 无文档占位提示
            Text {
                anchors.centerIn: parent
                text: StartupTheme.tr("No recent documents")
                color: StartupTheme.recentEmptyFg
                font.pixelSize: StartupTheme.fontRecentEmpty
                visible: page.recentDocs.length === 0
            }
        }
    }
}
