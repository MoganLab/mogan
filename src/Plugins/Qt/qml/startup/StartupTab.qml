// StartupTab.qml — 启动页主容器。
// 对应 C++ QTMStartupTabWidget：左侧导航栏 + 右侧内容区（QStackedWidget）。
//
// 布局：Row { Sidebar (固定宽) + Content (flex 1) }
// 右侧内容用 StackLayout 切换 Home / Template 两页。
//
// 数据：startupBridge context property 提供 categories、recentDocs 等。
// 动作：startupBridge.selectCategory(id) / quit() 等。

import QtQuick
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "atoms"

Rectangle {
    id: root
    color: StartupTheme.contentBg
    focus: true

    property var categories: typeof startupBridge !== "undefined" && startupBridge.categories ? startupBridge.categories : []
    property int activePage: 0  // 0=Home, 1=Template

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ================================================================
        // 左侧导航栏 (对应 QWidget#startup-tab-sidebar)
        // ================================================================
        Rectangle {
            Layout.preferredWidth: StartupTheme.sidebarWidth
            Layout.fillHeight: true
            color: StartupTheme.sidebarBg

            Column {
                anchors.fill: parent
                anchors.topMargin: 60 * StartupTheme.scaleFactor

                // Navigation 标题
                Text {
                    text: qsTr("Navigation")
                    color: StartupTheme.navTitleFg
                    font.pixelSize: StartupTheme.fontNavTitle
                    font.weight: Font.Bold
                    leftPadding: 20 * StartupTheme.scaleFactor
                    bottomPadding: 8 * StartupTheme.scaleFactor
                }

                // Home 按钮
                Rectangle {
                    id: homeBtn
                    width: parent.width - 16 * StartupTheme.scaleFactor
                    height: StartupTheme.fontNavBtn + 2 * StartupTheme.navBtnPadV
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: StartupTheme.navBtnRadius
                    color: root.activePage === 0 ? StartupTheme.navBtnActiveBg :
                           (homeMouse.containsMouse ? StartupTheme.navBtnHoverBg : "transparent")

                    Text {
                        anchors { left: parent.left; leftMargin: StartupTheme.navBtnPadH; verticalCenter: parent.verticalCenter }
                        text: qsTr("Home")
                        color: StartupTheme.navBtnFg
                        font.pixelSize: StartupTheme.fontNavBtn
                    }
                    MouseArea {
                        id: homeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.activePage = 0
                    }
                }

                // 分类按钮（动态）
                Repeater {
                    model: root.categories
                    delegate: Rectangle {
                        width: parent.width - 16 * StartupTheme.scaleFactor
                        height: StartupTheme.fontNavBtn + 2 * StartupTheme.navBtnPadV
                        anchors.horizontalCenter: parent.horizontalCenter
                        radius: StartupTheme.navBtnRadius
                        color: root.activePage === 1 && catMouse.containsMouse === false ?
                               (typeof startupBridge !== "undefined" && startupBridge.activeCategoryId === modelData.id ?
                                StartupTheme.navBtnActiveBg : "transparent") :
                               (catMouse.containsMouse ? StartupTheme.navBtnHoverBg :
                               (root.activePage === 1 && typeof startupBridge !== "undefined" && startupBridge.activeCategoryId === modelData.id ?
                                StartupTheme.navBtnActiveBg : "transparent"))

                        Text {
                            anchors { left: parent.left; leftMargin: StartupTheme.navBtnPadH; verticalCenter: parent.verticalCenter }
                            text: modelData.name
                            color: StartupTheme.navBtnFg
                            font.pixelSize: StartupTheme.fontNavBtn
                            elide: Text.ElideRight
                        }
                        MouseArea {
                            id: catMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.activePage = 1
                                if (typeof startupBridge !== "undefined") startupBridge.selectCategory(modelData.id)
                            }
                        }
                    }
                }

                // 弹性空间
                Item { Layout.fillHeight: true; height: 1 }

                // Quit 按钮
                Rectangle {
                    width: parent.width - 16 * StartupTheme.scaleFactor
                    height: StartupTheme.fontNavBtn + 2 * StartupTheme.navBtnPadV
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: StartupTheme.quitBtnRadius
                    color: quitMouse.containsMouse ? StartupTheme.quitBtnHoverBg : "transparent"
                    border.width: 1
                    border.color: StartupTheme.quitBtnBorder

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Quit")
                        color: StartupTheme.quitBtnFg
                        font.pixelSize: StartupTheme.fontNavBtn
                    }
                    MouseArea {
                        id: quitMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (typeof startupBridge !== "undefined") startupBridge.quit()
                        }
                    }
                }

                Item { height: 16 * StartupTheme.scaleFactor }
            }
        }

        // ================================================================
        // 右侧内容区
        // ================================================================
        StackLayout {
            id: contentStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.activePage

            StartupHomePage { }

            StartupTemplatePage { }
        }
    }
}
