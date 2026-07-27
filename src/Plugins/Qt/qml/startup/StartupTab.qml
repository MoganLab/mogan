// StartupTab.qml — 启动页主容器。
// 左侧导航栏 + 右侧内容区。

import QtQuick
import "atoms"

Rectangle {
    id: root
    color: StartupTheme.contentBg
    focus: true

    property var categories: typeof startupBridge !== "undefined" && startupBridge.categories ? startupBridge.categories : []
    property int activePage: 0  // 0=Home, 1=Template

    // ================================================================
    // 左侧导航栏 (对应 QWidget#startup-tab-sidebar)
    // ================================================================
    Rectangle {
        id: sidebar
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: StartupTheme.sidebarWidth
        color: StartupTheme.sidebarBg

        // 导航按钮统一尺寸
        property real btnWidth: sidebar.width - 16 * StartupTheme.scaleFactor
        property real btnHeight: StartupTheme.fontNavBtn + 2 * StartupTheme.navBtnPadV

        // Navigation 标题
        Text {
            id: navTitle
            anchors {
                top: parent.top
                left: parent.left
                topMargin: 16 * StartupTheme.scaleFactor
                leftMargin: 20 * StartupTheme.scaleFactor
            }
            text: StartupTheme.tr("Navigation")
            color: StartupTheme.navTitleFg
            font.pixelSize: StartupTheme.fontNavTitle
            font.weight: Font.Bold
        }

        // Home 按钮
        Rectangle {
            id: homeBtn
            anchors {
                top: navTitle.bottom
                topMargin: 8 * StartupTheme.scaleFactor + 2 * StartupTheme.scaleFactor
                horizontalCenter: parent.horizontalCenter
            }
            width: sidebar.btnWidth
            height: sidebar.btnHeight
            radius: StartupTheme.navBtnRadius
            color: root.activePage === 0 ? StartupTheme.navBtnActiveBg :
                   (homeMouse.containsMouse ? StartupTheme.navBtnHoverBg : "transparent")

            Text {
                anchors.centerIn: parent
                text: StartupTheme.tr("Home")
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

        // 分类按钮（动态）—— 用 Column 垂直堆叠，挂在 Home 按钮下方
        Column {
            id: categoryColumn
            anchors {
                top: homeBtn.bottom
                left: parent.left
                right: parent.right
                topMargin: 2 * StartupTheme.scaleFactor
            }
            spacing: 2 * StartupTheme.scaleFactor

            Repeater {
                model: root.categories
                delegate: Rectangle {
                    width: sidebar.btnWidth
                    height: sidebar.btnHeight
                    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                    radius: StartupTheme.navBtnRadius
                    color: {
                        var isActive = root.activePage === 1 &&
                            typeof startupBridge !== "undefined" &&
                            startupBridge.activeCategoryId === modelData.id
                        if (isActive) return StartupTheme.navBtnActiveBg
                        if (catMouse.containsMouse) return StartupTheme.navBtnHoverBg
                        return "transparent"
                    }

                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 2 * StartupTheme.navBtnPadH
                        text: modelData.name
                        color: StartupTheme.navBtnFg
                        font.pixelSize: StartupTheme.fontNavBtn
                        elide: Text.ElideMiddle
                        horizontalAlignment: Text.AlignHCenter
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
        }

        // Quit 按钮（钉在侧边栏底部）
        Rectangle {
            id: quitBtn
            anchors {
                bottom: parent.bottom
                bottomMargin: 16 * StartupTheme.scaleFactor
                horizontalCenter: parent.horizontalCenter
            }
            width: sidebar.btnWidth
            height: sidebar.btnHeight
            radius: StartupTheme.quitBtnRadius
            color: quitMouse.containsMouse ? StartupTheme.quitBtnHoverBg : "transparent"
            border.width: 1
            border.color: StartupTheme.quitBtnBorder

            Text {
                anchors.centerIn: parent
                text: StartupTheme.tr("Quit")
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
    }

    // ================================================================
    // 右侧内容区 —— 用 visible 切换，替代 StackLayout
    // ================================================================
    Item {
        id: contentArea
        anchors {
            left: sidebar.right
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }

        StartupHomePage {
            anchors.fill: parent
            visible: root.activePage === 0
        }
        StartupTemplatePage {
            anchors.fill: parent
            visible: root.activePage === 1
        }
    }
}
