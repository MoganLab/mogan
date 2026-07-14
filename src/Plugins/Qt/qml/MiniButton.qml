// MiniButton.qml — 紧凑小按钮原子（行间距预设等内联按钮组复用）。
// 与 DialogButtons 同风格（hover 变色 + 点击缩放 + 配色走 Theme），但尺寸小、
// 圆角小、无主按钮配色——用于对话框正文内的辅助按钮组，不替代底部主按钮行。
//
// 内部 id 用 btn（非 root），避免调用方 delegate 的 `root.xxx`（指 DialogShell）
// 被本组件实例的同名 id 遮蔽，导致 refreshTick++ 改错对象、点击不刷新。
//
// API：
//   text     : string  —— 按钮文案。
//   width    : real    —— 按钮宽度（调用方传含 scaleFactor 的绝对值），默认 48×scale。
//   height   : real    —— 按钮高度，默认 28×scale。
//   clicked()          —— 点击信号。
//
// 用法：
//   MiniButton { text: "1.5x"; width: 40*Theme.scaleFactor; onClicked: doSomething() }

import QtQuick
import "."

Rectangle {
    id: btn
    width: 48 * Theme.scaleFactor
    height: 28 * Theme.scaleFactor
    radius: 7 * Theme.scaleFactor
    color: ma.containsMouse ? Theme.fieldBgHover : Theme.fieldBg
    border.width: 1 * Theme.scaleFactor
    border.color: Theme.borderClr

    property string text: ""
    signal clicked()

    Text {
        anchors.centerIn: parent
        text: btn.text
        color: Theme.fg
        font.pixelSize: 11 * Theme.scaleFactor
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onPressed: parent.scale = 0.94
        onReleased: parent.scale = 1.0
        onCanceled: parent.scale = 1.0
        onClicked: btn.clicked()
    }
    Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }
    Behavior on color { ColorAnimation { duration: 150 } }
}
