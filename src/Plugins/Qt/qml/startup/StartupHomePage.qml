// StartupHomePage.qml — 启动页 Home 页面。
// 对应 C++ QTMHomePage：Document Style 样式卡片 + 最近文档列表。
//
// 数据来源：startupBridge context property（C++ StartupBridge 注入）。
//   - startupBridge.recentDocs: [{fileName, filePath, openedAt}, ...]
//   - startupBridge.styleCards: [{kind, id, name, titleText, iconSrc, thumbSrc}, ...]
//
// 动作（调用 startupBridge 方法）：
//   - startupBridge.newDocument() / openDocument() / openRecentDoc(path)
//   - startupBridge.openTemplate(templateId)

import QtQuick
import QtQuick.Controls 2.15
import "atoms"

Flickable {
    id: page
    contentHeight: contentCol.height
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    // 数据模型（来自 bridge，fallback 为空）
    property var styleCards: typeof startupBridge !== "undefined" && startupBridge.styleCards ? startupBridge.styleCards : []
    property var recentDocs: typeof startupBridge !== "undefined" && startupBridge.recentDocs ? startupBridge.recentDocs : []

    Column {
        id: contentCol
        anchors {
            left: parent.left
            leftMargin: StartupTheme.contentPadH
            right: parent.right
            rightMargin: StartupTheme.contentPadH
        }
        spacing: 0

        // 顶部间距（对齐 HTML content padding-top）
        Item { width: 1; height: StartupTheme.contentPadTop }

        // ---- Document Style 分区 ----
        Text {
            text: qsTr("Document Style")
            color: StartupTheme.sectionTitleFg
            font.pixelSize: StartupTheme.fontSectionTitle
            font.weight: Font.DemiBold
        }

        Item { width: 1; height: 16 * StartupTheme.scaleFactor } // section 标题 margin-bottom 16px

        // 样式卡片行（Flow 布局，自动换行）
        Flow {
            id: cardsFlow
            width: parent.width
            spacing: StartupTheme.gapCards

            // 新建文档（图标模式，固定）
            StyleCard {
                kind: "icon"
                iconSrc: "qrc:/startup-tab/new-file.svg"
                cardName: qsTr("New document")
                onClicked: {
                    if (typeof startupBridge !== "undefined") startupBridge.newDocument()
                }
            }

            // 打开文档（图标模式，固定）
            StyleCard {
                kind: "icon"
                iconSrc: "qrc:/startup-tab/open-file.svg"
                cardName: qsTr("Open document")
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

        // 分区间距（HTML: .style-cards margin-bottom 20px）
        Item { width: 1; height: StartupTheme.sectionGap }

        // ---- 分隔线 (HTML: QFrame#startup-tab-separator) ----
        Rectangle {
            width: parent.width
            height: 1
            color: StartupTheme.separatorColor
        }

        Item { width: 1; height: StartupTheme.sectionGap }

        // ---- Recent Documents 分区 ----
        Text {
            text: qsTr("Recent Documents")
            color: StartupTheme.sectionTitleFg
            font.pixelSize: StartupTheme.fontSectionTitle
            font.weight: Font.DemiBold
        }

        Item { width: 1; height: 16 * StartupTheme.scaleFactor }

        // 最近文档列表
        Rectangle {
            width: parent.width
            height: recentListHeight
            radius: StartupTheme.recentRadius
            color: StartupTheme.recentBg
            border.width: 1
            border.color: StartupTheme.recentBorder

            readonly property real recentListHeight: {
                var count = page.recentDocs.length
                if (count === 0)
                    return StartupTheme.recentItemH + 12 * StartupTheme.scaleFactor
                return count * (StartupTheme.recentItemH + 2 * StartupTheme.recentItemMarginV)
                       + 4 * StartupTheme.scaleFactor
            }

            ListView {
                id: recentList
                anchors.fill: parent
                anchors.margins: StartupTheme.recentItemMarginH + StartupTheme.recentItemMarginV
                spacing: 0
                model: page.recentDocs
                interactive: false

                delegate: Rectangle {
                    width: recentList.width - 2 * StartupTheme.recentItemMarginH
                    height: StartupTheme.recentItemH
                    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                    radius: StartupTheme.recentItemRadius
                    color: itemMouse.containsMouse ? StartupTheme.recentHoverBg : "transparent"

                    // 文件名（左对齐，超出省略）
                    Text {
                        id: fileNameText
                        anchors {
                            left: parent.left
                            leftMargin: StartupTheme.recentItemPadH
                            verticalCenter: parent.verticalCenter
                        }
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
                        text: qsTr("Last opened") + ": " + (modelData.openedAt || "")
                        color: StartupTheme.recentTimeFg
                        font.pixelSize: StartupTheme.fontRecentTime
                    }

                    MouseArea {
                        id: itemMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (typeof startupBridge !== "undefined") startupBridge.openRecentDoc(modelData.filePath)
                        }
                    }
                }

                // 无文档占位提示
                Text {
                    anchors.centerIn: parent
                    text: qsTr("No recent documents")
                    color: StartupTheme.recentEmptyFg
                    font.pixelSize: StartupTheme.fontRecentEmpty
                    visible: page.recentDocs.length === 0
                }
            }
        }

        // 底部间距
        Item { width: 1; height: StartupTheme.contentPadBottom }
    }
}
