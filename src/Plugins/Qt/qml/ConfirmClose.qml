// ConfirmClose.qml
// 「是否保存」模态弹窗。全部视觉在 QML 自绘（C++ 侧无边框 QDialog + setMask
// 圆角），配色按 isDark 切 light / night，跟随 Mogan 当前主题。
// 无关闭 X 按钮：与 Cancel 语义重复，三按钮已含取消。
//
// context property（C++ 注入）：dialogMessage、dialogButtons、dpScale、isDark、
// closeBridge。root 基准尺寸（96 DPI），C++ 按 DpiUtils::scaled 放大 QDialog；
// QML 内部所有视觉值乘 dpScale 同步放大。按钮下标从 1 起（0 = Esc / mask 外 = 取消）。

import QtQuick

Item {
    // 填满 QQuickWidget 视口（C++ 用 SizeRootObjectToView + 固定 QDialog 尺寸）。
    anchors.fill: parent
    // 仅作 QML 设计器预览的首选尺寸。
    implicitWidth: 400
    implicitHeight: 200

    property string message: typeof dialogMessage !== "undefined" ? dialogMessage : ""
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : []
    property real scaleFactor: typeof dpScale !== "undefined" ? dpScale : 1.0
    property bool dark: typeof isDark !== "undefined" ? isDark : false

    // 键盘交互：Enter 触发肯定按钮（下标 1），Esc 触发取消。
    focus: true
    Keys.onEscapePressed: closeBridge.choose(-1)
    Keys.onReturnPressed: closeBridge.choose(1)
    Keys.onEnterPressed: closeBridge.choose(1)

    // 主题配色
    readonly property color bg: dark ? "#2b2b3a" : "#ffffff"
    readonly property color fg: dark ? "#eaeef2" : "#1e1e1e"
    readonly property color accent: dark ? "#4a9eff" : "#1e1e1e"
    readonly property color btnBg: dark ? "#3d3d52" : "#f1f3f4"
    readonly property color btnBgHover: dark ? "#4e4e66" : "#e5e7eb"
    readonly property color borderClr: dark ? "#3e3e52" : "#d0d4da"
    readonly property color shadowColor: dark ? Qt.rgba(0, 0, 0, 0.35) : Qt.rgba(0, 0, 0, 0.10)

    Rectangle {
        anchors.fill: parent
        color: bg
        radius: 16 * scaleFactor
        border.width: 1
        border.color: borderClr

        // 拖动无边框窗口：在背景区域按住拖动即移动整个弹窗。
        DragHandler {
            target: null
            onActiveChanged: if (active) closeBridge.startMove()
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 28 * scaleFactor
            spacing: 28 * scaleFactor

            Text {
                width: parent.width
                text: message
                color: fg
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 17 * scaleFactor
                font.weight: Font.Bold
                font.letterSpacing: 0.2 * scaleFactor
                lineHeight: 1.6
                lineHeightMode: Text.ProportionalHeight
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 16 * scaleFactor

                Repeater {
                    model: buttonLabels
                    delegate: Rectangle {
                        // 固定宽度让三按钮等宽（按最长文案「Don't save」留余量）。
                        width: 108 * scaleFactor
                        height: 40 * scaleFactor
                        radius: 20 * scaleFactor  // 胶囊圆角
                        color: {
                            if (ma.containsMouse) {
                                return primary ? (dark ? "#5faaff" : "#3a3a3a") : btnBgHover
                            }
                            return primary ? accent : btnBg
                        }
                        border.width: primary ? 1 : 0
                        border.color: primary ? accent : "transparent"
                        
                        property bool primary: index === 0
                        
                        Text {
                            id: txt
                            anchors.centerIn: parent
                            text: modelData
                            color: primary ? "#ffffff" : fg
                            font.pixelSize: 15 * scaleFactor
                            font.weight: primary ? Font.Bold : Font.DemiBold
                            font.letterSpacing: 0.3 * scaleFactor
                        }
                        
                        MouseArea {
                            id: ma
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: closeBridge.choose(index + 1)
                            
                            // 点击缩放效果
                            onPressed: parent.scale = 0.96
                            onReleased: parent.scale = 1.0
                            onCanceled: parent.scale = 1.0
                        }
                        
                        Behavior on scale {
                            NumberAnimation { duration: 80; easing.type: Easing.OutQuad }
                        }
                        
                        Behavior on color {
                            ColorAnimation { duration: 150 }
                        }
                    }
                }
            }
        }
    }
}