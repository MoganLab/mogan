// ConfirmClose.qml — 「是否保存」模态弹窗。
// DialogShell + DialogButtons 拼装，仅剩消息文案 + 按钮标签注入 + 按钮下标语义。
// 无关闭 X 按钮：与 Cancel 语义重复，三按钮已含取消。
//
// context property（C++ 注入）：dialogMessage、dialogButtons、dpScale、isDark、
// closeBridge。按钮下标从 1 起（0 = Esc = 取消），故 clicked(index) 映射为
// choose(index + 1)。

import QtQuick
import "."

DialogShell {
    id: root
    implicitWidth: 400
    implicitHeight: 150
    implicitMargins: 28 * Theme.scaleFactor

    property string message: typeof dialogMessage !== "undefined" ? dialogMessage : ""
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : []

    onActivate: () => closeBridge.choose(1)

    // content 填满正文区，消息+按钮用内层 Column 垂直居中。用 root.id 引用根属性，
    // 不走 parent 链（content 被 reparent 到 contentCol，中间隔两层）。
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
                // 单行；文件名过长由 scheme 侧 truncate-filename-middle 中间省略。
                elide: Text.ElideRight
            }

            DialogButtons {
                anchors.horizontalCenter: parent.horizontalCenter
                buttonLabels: root.buttonLabels
                buttonWidth: 108 * Theme.scaleFactor
                onClicked: function(index) { closeBridge.choose(index + 1) }
            }
        }
    }
}
