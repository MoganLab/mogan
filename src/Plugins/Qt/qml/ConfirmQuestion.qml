// ConfirmQuestion.qml — 「问题」确认弹窗（如 PDF 导出完成询问是否打开文件）。
// 替换原 QMessageBox 问题弹窗（question-no-cancel）。与 ConfirmClose 同构，
// 差异：主按钮（默认）下标由 C++ 经 dialogPrimary 注入——按钮显示顺序与语义
// 顺序相反（默认按钮居右），Enter 触发默认按钮。
//
// context property（C++ 注入）：dialogMessage、dialogButtons（显示顺序，均已
// 翻译）、dialogPrimary（默认按钮下标）、dpScale、isDark、closeBridge。
// 按钮下标从 1 起（0 = Esc = 取消），clicked(index) 映射 choose(index + 1)。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 400
    implicitHeight: 150
    implicitMargins: 28 * Theme.scaleFactor

    property string message: typeof dialogMessage !== "undefined" ? dialogMessage : ""
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : []
    property int primaryIndex: typeof dialogPrimary !== "undefined" ? dialogPrimary : 0

    // Enter/Return 触发默认按钮（primaryIndex 指向的按钮，如「是」）。
    onActivate: () => closeBridge.choose(root.primaryIndex + 1)

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
                // 单行。文案过长时 ElideMiddle 中间省略（保留首尾）。
                elide: Text.ElideMiddle
            }

            DialogButtons {
                anchors.horizontalCenter: parent.horizontalCenter
                buttonLabels: root.buttonLabels
                primaryIndex: root.primaryIndex
                buttonWidth: 108 * Theme.scaleFactor
                letterSpacing: 0.3
                onClicked: function (index) {
                    closeBridge.choose(index + 1);
                }
            }
        }
    }
}
