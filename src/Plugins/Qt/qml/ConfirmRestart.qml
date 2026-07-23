// ConfirmRestart.qml — 需重启字段的三按钮确认弹窗（重启 / 稍后 / 取消）。
// 用于首选项里改 look and feel / gui theme / keyboard shortcut style 后的确认，
// 替换旧 user-confirm（两按钮）。
// 与 ConfirmClose 同构，差异：多了标题行；标题居中、正文左对齐（英文换行时更整齐）。
//
// context property（C++ 注入）：dialogTitle / dialogMessage / dialogButtons
// （均已翻译）、dpScale、isDark、closeBridge。按钮下标从 1 起（0 = Esc = 取消），
// clicked(index) 映射 choose(index + 1)：1=重启 / 2=稍后 / 3=取消。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 420
    implicitHeight: 170
    implicitMargins: 24 * Theme.scaleFactor

    property string title: typeof dialogTitle !== "undefined" ? dialogTitle : ""
    property string message: typeof dialogMessage !== "undefined" ? dialogMessage : ""
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : []

    // Enter/Return 默认重启。
    onActivate: () => closeBridge.choose(1)

    // content 被 DialogShell reparent 并 anchors.fill，外层 Item 承接 fill、
    // 内层 Column 用 verticalCenter 居中（同 Item 不能既 fill 又 verticalCenter）。
    content: Item {
        Column {
            id: body
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 16 * Theme.scaleFactor

            Text {
                width: body.width
                text: root.title
                color: Theme.fg
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 17 * Theme.scaleFactor
                font.weight: Font.Bold
                font.letterSpacing: 0.2 * Theme.scaleFactor
                elide: Text.ElideRight
                visible: text.length > 0
            }

            Text {
                width: body.width
                text: root.message
                color: Theme.muted
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontBody
                lineHeight: 1.5
            }

            DialogButtons {
                anchors.horizontalCenter: parent.horizontalCenter
                buttonLabels: root.buttonLabels
                buttonWidth: 108 * Theme.scaleFactor
                letterSpacing: 0.3
                primaryIndex: 0
                onClicked: function (index) {
                    closeBridge.choose(index + 1);
                }
            }
        }
    }
}
