// ConfirmClose.qml — 「是否保存」模态弹窗。
// DialogShell + DialogButtons 拼装，仅剩消息文案 + 按钮标签注入 + 按钮下标语义。
// 无关闭 X 按钮：与 Cancel 语义重复，三按钮已含取消。
//
// context property（C++ 注入）：dialogMessage、dialogButtons、dpScale、isDark、
// closeBridge。按钮下标从 1 起（0 = Esc = 取消），故 clicked(index) 映射为
// choose(index + 1)。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 400
    implicitHeight: 150
    implicitMargins: 28 * Theme.scaleFactor

    property string message: typeof dialogMessage !== "undefined" ? dialogMessage : ""
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : []

    onActivate: () => closeBridge.choose(1)

    // content 填满正文区（DialogShell 强制设 anchors.fill），消息+按钮用内层
    // Column 垂直居中。外层 Item 不可省：它承接 anchors.fill，让 Column 自由用
    // verticalCenter 居中（同一 Item 不能既 fill 又 verticalCenter）。用 root.id
    // 引用根属性（content 被 reparent 到 contentCol，parent 链不可靠）。
    content: Item {
        Column {
            id: body
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 28 * Theme.scaleFactor

            Text {
                width: body.width
                text: root.message
                color: Theme.fg
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 17 * Theme.scaleFactor
                font.weight: Font.Bold
                font.letterSpacing: 0.2 * Theme.scaleFactor
                // 单行。文件名过长时 ElideMiddle 中间省略（保留「?」与文件名首尾）。
                elide: Text.ElideMiddle
            }

            DialogButtons {
                anchors.horizontalCenter: parent.horizontalCenter
                buttonLabels: root.buttonLabels
                buttonWidth: 108 * Theme.scaleFactor
                letterSpacing: 0.3
                onClicked: function (index) {
                    closeBridge.choose(index + 1);
                }
            }
        }
    }
}
