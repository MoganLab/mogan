// DialogButtons.qml — 可复用按钮行（自绘胶囊 + hover + 点击缩放）。
// 只发 clicked(index)，不假定按钮语义——两弹窗语义相反（FormDialog 是
// index 0=submit / 其余=cancel；ConfirmClose 是 choose(index+1)，0/Esc=取消），
// 故语义由调用方在 onClicked 映射。
//
// API：
//   buttonLabels : list<string>   —— C++ 已翻译注入的按钮文案。
//   primaryIndex : int            —— 主按钮下标（决定深色配色），默认 0。
//   buttonWidth  : real           —— 单按钮宽度（逻辑像素 × scaleFactor），默认 100。
//   letterSpacing: real           —— 按钮文字字距，默认 0。
//   signal clicked(int index)     —— 点击信号，调用方在 onClicked 决定 submit/cancel。
//
// 用法：
//   DialogButtons {
//       buttonLabels: ["OK", "Cancel"]
//       onClicked: function(i) { if (i === 0) submit(); else cancel() }
//   }

import QtQuick

Row {
    id: row
    spacing: Theme.gapM

    property var buttonLabels: []
    property int primaryIndex: 0
    property real buttonWidth: Theme.btnW
    property real letterSpacing: 0
    signal clicked(int index)

    Repeater {
        model: row.buttonLabels
        delegate: Rectangle {
            width: row.buttonWidth
            height: Theme.btnH
            radius: height / 2
            color: ma.containsMouse ? (primary ? (Theme.dark ? "#8a8a8a" : "#3a3a3a") : Theme.fieldBgHover) : (primary ? Theme.accent : Theme.fieldBg)
            border.width: primary ? Theme.borderW : 0
            border.color: primary ? Theme.accent : "transparent"

            property bool primary: index === row.primaryIndex

            Text {
                anchors.centerIn: parent
                text: modelData
                color: primary ? "#ffffff" : Theme.fg
                font.pixelSize: Theme.fontBtn
                font.weight: primary ? Font.Bold : Font.DemiBold
                font.letterSpacing: row.letterSpacing * Theme.scaleFactor
            }
            MouseArea {
                id: ma
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onPressed: parent.scale = 0.96
                onReleased: parent.scale = 1.0
                onCanceled: parent.scale = 1.0
                onClicked: row.clicked(index)
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
    }
}
