// AiActionsBar.qml — 选区下方的一行 AI 操作栏（翻译 / 润色 / 对话）。
// 由 QTMAiTranslatePopup（QQuickWidget 宿主）以 qrc URL 直接加载：非模态、
// 不用 DialogShell。按钮文案由 C++ 经 context property 注入（已过 qt_translate）；
// 点击经根信号 triggered(action) 回传 C++。
import QtQuick
import "atoms"

Item {
    id: bar

    signal triggered(string action)

    // C++ autoSize 按屏幕 DPI 缩放注入；12 仅为加载测试/预览回退
    property int fontPixelSize: 12

    readonly property int iconPx: Math.max(11, Math.round(fontPixelSize * 1.3))
    readonly property int padH: Math.max(7, Math.round(fontPixelSize * 0.9))
    readonly property int padV: Math.max(4, Math.round(fontPixelSize * 0.45))
    readonly property int gap: Math.max(3, Math.round(fontPixelSize * 0.35))

    // 右侧 1px、下方 2px 留给假阴影
    width: implicitWidth
    height: implicitHeight
    implicitWidth: body.width + 1
    implicitHeight: body.height + 2

    Rectangle {
        x: 1
        y: 2
        width: body.width
        height: body.height
        radius: body.radius
        color: "#33000000"
    }

    Rectangle {
        id: body
        width: row.implicitWidth + bar.padH * 2
        height: row.implicitHeight + bar.padV * 2
        radius: height / 2
        color: Theme.bg
        border.width: 1
        border.color: Theme.borderClr

        Row {
            id: row
            x: bar.padH
            y: bar.padV
            spacing: bar.gap * 2

            Image {
                // OpenClaw 龙虾标识（emoji 在部分平台渲染异常，改用 SVG 线稿）
                source: "qrc:/ai-actions/lobster.svg"
                sourceSize: Qt.size(bar.iconPx, bar.iconPx)
                anchors.verticalCenter: parent.verticalCenter
            }

            Repeater {
                model: [
                    { icon: "qrc:/ai-actions/translate.svg", label: labelTranslate, action: "translate" },
                    { icon: "qrc:/ai-actions/polish.svg", label: labelPolish, action: "polish" },
                    { icon: "qrc:/ai-actions/chat.svg", label: labelChat, action: "chat" }
                ]

                delegate: Rectangle {
                    id: capsule
                    implicitWidth: content.implicitWidth + bar.padH
                    implicitHeight: content.implicitHeight + bar.padV
                    radius: height / 2
                    // hover 套用列表项选中态配色（selectBg/selectBorder/selectFg）；
                    // !pressed 兜底：按住拖动时事件被 grab，避免多个按钮同亮
                    property bool lit: hoverArea.containsMouse && !hoverArea.pressed
                    color: lit ? Theme.selectBg : "transparent"
                    border.width: lit ? 1 : 0
                    border.color: Theme.selectBorder

                    Row {
                        id: content
                        anchors.centerIn: parent
                        spacing: bar.gap

                        Image {
                            source: modelData.icon
                            sourceSize: Qt.size(bar.iconPx, bar.iconPx)
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: modelData.label
                            color: capsule.lit ? Theme.selectFg : Theme.fg
                            font.pixelSize: bar.fontPixelSize
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        id: hoverArea
                        objectName: "aiActionHoverArea"
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bar.triggered(modelData.action)
                    }
                }
            }
        }
    }
}
