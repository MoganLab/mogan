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
//   primaryEnabled: bool          —— 主按钮是否可用，默认 true。
//   primaryDisabledToolTip: string—— 主按钮禁用时的 hover 提示文本，默认空。
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
    property bool primaryEnabled: true
    property string primaryDisabledToolTip: ""
    signal clicked(int index)

    Repeater {
        model: row.buttonLabels
        delegate: Item {
            id: btnContainer
            width: row.buttonWidth
            height: Theme.btnH

            property bool primary: index === row.primaryIndex
            property bool isBtnEnabled: !primary || row.primaryEnabled
            readonly property string tipText: (primary && !row.primaryEnabled) ? row.primaryDisabledToolTip : ""

            Rectangle {
                id: btnBg
                anchors.fill: parent
                radius: height / 2

                opacity: btnContainer.isBtnEnabled ? 1.0 : 0.4
                color: (ma.containsMouse && btnContainer.isBtnEnabled) ? (btnContainer.primary ? (Theme.dark ? "#8a8a8a" : "#3a3a3a") : Theme.fieldBgHover) : (btnContainer.primary ? Theme.accent : Theme.fieldBg)
                border.width: (btnContainer.primary && btnContainer.isBtnEnabled) ? Theme.borderW : 0
                border.color: (btnContainer.primary && btnContainer.isBtnEnabled) ? Theme.accent : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: modelData
                    color: btnContainer.primary ? "#ffffff" : Theme.fg
                    font.pixelSize: Theme.fontBtn
                    font.weight: btnContainer.primary ? Font.Bold : Font.DemiBold
                    font.letterSpacing: row.letterSpacing * Theme.scaleFactor
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
                Behavior on opacity {
                    NumberAnimation {
                        duration: 150
                    }
                }
            }

            MouseArea {
                id: ma
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: btnContainer.isBtnEnabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                onPressed: if (btnContainer.isBtnEnabled) btnBg.scale = 0.96
                onReleased: btnBg.scale = 1.0
                onCanceled: btnBg.scale = 1.0
                onClicked: if (btnContainer.isBtnEnabled) row.clicked(index)
            }

            // 悬停提示框
            Rectangle {
                id: tipBubble
                visible: btnContainer.tipText !== "" && ma.containsMouse
                z: 1000
                anchors.bottom: parent.top
                anchors.bottomMargin: 8 * Theme.scaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                width: tipTextItem.implicitWidth + 16 * Theme.scaleFactor
                height: tipTextItem.implicitHeight + 10 * Theme.scaleFactor
                radius: 6 * Theme.scaleFactor
                color: Theme.dark ? "#1f1f1f" : "#2d3748"
                border.width: Theme.borderW
                border.color: Theme.dark ? "#444444" : "#4a5568"

                Text {
                    id: tipTextItem
                    anchors.centerIn: parent
                    text: btnContainer.tipText
                    font.pixelSize: 12 * Theme.scaleFactor
                    color: "#ffffff"
                }
            }
        }
    }
}
