// StartupHomePage.qml — 启动页 Home 页面。
// 对应 C++ QTMHomePage：Document Style 样式卡片 + 最近文档列表。
//
// 页面整体不滚动，仅 Recent Documents 内部滚动。

import QtQuick
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "atoms"

Item {
    id: page

    // 数据模型（来自 bridge，fallback 为空）
    property var styleCards: typeof startupBridge !== "undefined" && startupBridge.styleCards ? startupBridge.styleCards : []
    property var recentDocs: typeof startupBridge !== "undefined" && startupBridge.recentDocs ? startupBridge.recentDocs : []

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: StartupTheme.contentPadH
        anchors.rightMargin: StartupTheme.contentPadH
        spacing: 0

        // 顶部间距（对齐 HTML content padding-top）
        Item { Layout.preferredWidth: 1; Layout.preferredHeight: StartupTheme.contentPadTop }

        // ---- Document Style 分区 ----
        Text {
            Layout.fillWidth: true
            text: StartupTheme.tr("Document Style")
            color: StartupTheme.sectionTitleFg
            font.pixelSize: StartupTheme.fontSectionTitle
            font.weight: Font.DemiBold
        }

        Item { Layout.preferredWidth: 1; Layout.preferredHeight: 16 * StartupTheme.scaleFactor }

        // 样式卡片行（Flow 布局，自动换行）
        Flow {
            id: cardsFlow
            Layout.fillWidth: true
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

        // 分区间距
        Item { Layout.preferredWidth: 1; Layout.preferredHeight: StartupTheme.sectionGap }

        // ---- 分隔线 ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: StartupTheme.separatorColor
        }

        Item { Layout.preferredWidth: 1; Layout.preferredHeight: StartupTheme.sectionGap }

        // ---- Recent Documents 分区 ----
        Text {
            Layout.fillWidth: true
            text: StartupTheme.tr("Recent Documents")
            color: StartupTheme.sectionTitleFg
            font.pixelSize: StartupTheme.fontSectionTitle
            font.weight: Font.DemiBold
        }

        Item { Layout.preferredWidth: 1; Layout.preferredHeight: 16 * StartupTheme.scaleFactor }

        // 最近文档列表 — 占满剩余高度，内部滚动
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
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
                                contextMenu.popup()
                            } else {
                                if (typeof startupBridge !== "undefined") startupBridge.openRecentDoc(modelData.filePath)
                            }
                        }
                    }

                    Menu {
                        id: contextMenu
                        MenuItem {
                            text: StartupTheme.tr("Remove from list")
                            onTriggered: {
                                if (typeof startupBridge !== "undefined") startupBridge.removeRecentDoc(modelData.filePath)
                            }
                        }
                        MenuSeparator { }
                        MenuItem {
                            text: StartupTheme.tr("Clear list")
                            onTriggered: {
                                if (typeof startupBridge !== "undefined") startupBridge.clearAllRecentDocs()
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

        // 底部间距
        Item { Layout.preferredWidth: 1; Layout.preferredHeight: StartupTheme.contentPadBottom }
    }
}
