// PreviewPane.qml — 预览图区壳。显示 fontBridge 光栅化的样本 PNG（data URL）。
// 固定高度（外部给定），内部 Flickable 可滚动：样本类型切换（标准→中日韩）时
// 内容变高，不撑破布局而纵向滚动。光栅化产物已含 retina factor，按逻辑尺寸显示清晰。
//
// API：
//   imageSource : string —— "data:image/png;base64,..."。
//
// 用法（宽度/高度由父布局给定）：
//   PreviewPane { width: parent.width; height: 200; imageSource: root.previewUrl }

import QtQuick

Item {
    id: pane

    property string imageSource: ""

    Rectangle {
        anchors.fill: parent
        color: Theme.fieldBg
        clip: true

        Flickable {
            id: flick
            anchors.fill: parent
            anchors.margins: Theme.borderW
            contentWidth: img.width
            contentHeight: img.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentX: 0

            Image {
                id: img
                width: pane.width - 2 * Theme.borderW
                height: (img.sourceSize.width > 0) ? width * img.sourceSize.height / img.sourceSize.width : pane.height
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignLeft
                verticalAlignment: Image.AlignTop
                source: pane.imageSource
                asynchronous: false
                cache: false
                smooth: true
            }
        }
    }
}
