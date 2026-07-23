// MiniButton.qml — 通用按钮原子（对话框正文内的辅助按钮）。
// 与 DialogButtons 同风格（hover 变色 + 点击缩放 + 配色走 Theme），但无主按钮配色。
//
// size 两档：
//   "mini"   —— 紧凑小按钮（行间距预设等内联按钮组）：fontMini 字号、miniBtnH 高、
//              miniBtnR 圆角。宽度默认 miniBtnW，也可由调用方传 width 覆盖。
//   "normal" —— 与 EnumCombo 行等高的按钮（行内 action，如 Auto backup 打开备份目录）：
//              fontBody 字号、rowH 高、radius 圆角。宽度按文案自适应（文案 + 左右 padding）。
//
// 内部 id 用 btn（原子内部 id 不用 root，见 README 编码规矩）。
//
// API：
//   text     : string —— 按钮文案。
//   size     : string —— "mini"（默认）或 "normal"。
//   width    : real   —— 按钮宽度（mini 默认 miniBtnW，可覆盖；normal 按文案自适应）。
//   clicked()         —— 点击信号。

import QtQuick
import "."

Rectangle {
    id: btn
    property string text: ""
    property string size: "mini"
    readonly property bool isNormal: size === "normal"
    // normal：宽度按文案自适应（隐式 Text 宽 + 左右 padding）；mini：默认 miniBtnW，可覆盖。
    implicitWidth: isNormal ? (btnText.implicitWidth + 2 * Theme.comboPad) : Theme.miniBtnW
    width: implicitWidth
    height: isNormal ? Theme.rowH : Theme.miniBtnH
    radius: isNormal ? Theme.radius : Theme.miniBtnR
    color: ma.containsMouse ? Theme.fieldBgHover : Theme.fieldBg
    border.width: Theme.borderW
    border.color: Theme.borderClr

    signal clicked

    Text {
        id: btnText
        anchors.centerIn: parent
        text: btn.text
        color: Theme.fg
        font.pixelSize: btn.isNormal ? Theme.fontBody : Theme.fontMini
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
    Behavior on scale {
        NumberAnimation {
            duration: 80
            easing.type: Easing.OutQuad
        }
    }
    Behavior on color {
        ColorAnimation {
            duration: 150
        }
    }
}
