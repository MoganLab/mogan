// StartupTab.qml — 启动页主容器。
// 左侧导航栏 + 右侧内容区。

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

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 0
                spacing: 0

                // Navigation 标题
                Text {
                    Layout.topMargin: 16 * StartupTheme.scaleFactor
                    Layout.leftMargin: 20 * StartupTheme.scaleFactor
                    Layout.bottomMargin: 8 * StartupTheme.scaleFactor
                    text: qsTr("Navigation")
                    color: StartupTheme.navTitleFg
                    font.pixelSize: StartupTheme.fontNavTitle
                    font.weight: Font.Bold
                }

                // Home 按钮
                Rectangle {
                    Layout.preferredWidth: parent.width - 16 * StartupTheme.scaleFactor
                    Layout.preferredHeight: StartupTheme.fontNavBtn + 2 * StartupTheme.navBtnPadV
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 2 * StartupTheme.scaleFactor
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
                        Layout.preferredWidth: parent.width - 16 * StartupTheme.scaleFactor
                        Layout.preferredHeight: StartupTheme.fontNavBtn + 2 * StartupTheme.navBtnPadV
                        Layout.alignment: Qt.AlignHCenter
                        Layout.bottomMargin: 2 * StartupTheme.scaleFactor
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
                Item { Layout.fillHeight: true }

                // Quit 按钮
                Rectangle {
                    Layout.preferredWidth: parent.width - 16 * StartupTheme.scaleFactor
                    Layout.preferredHeight: StartupTheme.fontNavBtn + 2 * StartupTheme.navBtnPadV
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 16 * StartupTheme.scaleFactor
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
