// Version.qml -- 「帮助 -> 版本」模态弹窗。
//
// Scheme 侧负责翻译和组织 versionMessage；本组件仅渲染文本并回传确认结果。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 560
    implicitHeight: 220
    implicitMargins: Theme.margin

    property string title: versionBridge.title
    property var lines: versionBridge.lines
    property var buttonLabels: versionBridge.buttonLabels

    onActivate: () => versionBridge.confirm()
    Component.onCompleted: forceActiveFocus()

    content: Item {
        Column {
            id: body
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            width: parent.width
            spacing: Theme.pad

            Text {
                width: body.width
                text: root.title
                color: Theme.fg
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 18 * Theme.scaleFactor
                font.weight: Font.Bold
                elide: Text.ElideRight
            }

            Column {
                id: messageLines
                objectName: "versionMessageLines"
                width: parent.width
                spacing: Theme.gapS

                Repeater {
                    model: root.lines

                    Text {
                        required property string modelData
                        objectName: "versionMessageLine"
                        width: messageLines.width
                        text: modelData
                        color: Theme.fg
                        horizontalAlignment: Text.AlignLeft
                        wrapMode: Text.NoWrap
                        font.pixelSize: Theme.fontBody
                        lineHeight: 1.35
                    }
                }
            }

            DialogButtons {
                anchors.horizontalCenter: parent.horizontalCenter
                buttonLabels: root.buttonLabels
                onClicked: versionBridge.confirm()
            }
        }
    }
}
