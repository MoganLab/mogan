// Version.qml -- 「帮助 -> 版本」模态弹窗。
//
// Scheme 侧负责翻译和组织 versionMessage；本组件仅渲染文本并回传确认结果。
// 正文按 \n 分行；单行超过弹窗宽度时自动换行，弹窗高度随内容自适应（见
// implicitHeight 绑定，C++ 定宽后读取该值锁定弹窗高度）。
//
// 文本宽度绑定到 contentW（定宽弹窗的正文内容宽），而非实际父宽：弹窗 show
// 之前 C++ 就要读 implicitHeight 定高，此刻布局宽度尚未就位，绑实际宽度会
// 量出错误的换行结果。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 560
    implicitHeight: Math.max(220 * Theme.scaleFactor,
                             body.implicitHeight + 2 * Theme.margin)
    implicitMargins: Theme.margin

    // 定宽弹窗的正文内容宽（QML 像素）：窗口物理宽 = 逻辑宽 × scaleFactor
    readonly property real contentW: implicitWidth * Theme.scaleFactor - 2 * implicitMargins

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
                width: root.contentW
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
                width: root.contentW
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
                        // Wrap：优先按词边界折行，超长词（URL/版本号）断开；
                        // 中文无空格也可在字符间断开
                        wrapMode: Text.Wrap
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
