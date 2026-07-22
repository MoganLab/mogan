// Statistics.qml — 文档统计信息弹窗。
// DialogShell + 统计项列表 + Close 按钮，纯展示、一次性提交，无实时编辑。
//
// context property（C++ 注入）：statsTitle、statsModel、dialogButtons、dpScale、
// isDark、closeBridge。dialogButtons 经 qt_translate 翻译。Close 映射 choose(0)。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 380
    implicitMargins: Theme.margin
    // 动态高度：上下边距 + 标题 + 统计行 + 按钮区 + Column 内间距。
    implicitHeight: Theme.margin * 2 + Theme.titleH
                    + statsModel.length * Theme.textRowH + Theme.btnH + Theme.padS * 2

    property real labelW: 0.62

    property string title: typeof statsTitle !== "undefined" ? statsTitle : ""
    property var statsModel: typeof statsItems !== "undefined" ? statsItems : []
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["Close"]

    onActivate: () => closeBridge.choose(0)

    content: Item {
        Column {
            id: body
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: Theme.pad

            Text {
                width: body.width
                text: root.title
                color: Theme.fg
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 18 * Theme.scaleFactor
                font.weight: Font.Bold
                font.letterSpacing: 0.2 * Theme.scaleFactor
                elide: Text.ElideRight
            }

            // 统计行列表
            Column {
                id: statsCol
                width: parent.width
                spacing: Theme.gapS

                Repeater {
                    model: root.statsModel
                    delegate: Row {
                        width: statsCol.width
                        spacing: Theme.inlineGap

                        Text {
                            width: parent.width * root.labelW
                            text: modelData.label
                            color: Theme.fg
                            font.pixelSize: Theme.fontBody
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignLeft
                        }

                        Text {
                            width: parent.width * (1 - root.labelW) - Theme.inlineGap
                            text: modelData.value
                            color: Theme.fg
                            font.pixelSize: Theme.fontBody
                            font.family: "monospace"
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }
            }

            DialogButtons {
                anchors.horizontalCenter: parent.horizontalCenter
                buttonLabels: root.buttonLabels
                onClicked: function (index) {
                    closeBridge.choose(0);
                }
            }
        }
    }
}
