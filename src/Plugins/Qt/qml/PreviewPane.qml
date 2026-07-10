// PreviewPane.qml — 预览图区壳。显示 fontBridge 光栅化的样本 PNG（data URL）。
//
// 固定高度（外部给定），内部 Flickable 可滚动：样本类型切换（标准→中日韩）时
// 内容变高，不撑破布局，而是纵向滚动浏览。Image 宽度铺满容器，高度按图片原
// 比例（sourceSize 算），可能超出容器 → Flickable 滚动。光栅化产物已含 retina
// factor（物理像素 2×），按逻辑尺寸显示清晰不糊。
//
// API：
//   imageSource : string —— "data:image/png;base64,..."。
//   容器宽高由父给定（高度固定，宽度铺满）。

import QtQuick

Item {
    id: root

    property string imageSource: ""

    Rectangle {
        anchors.fill: parent
        color: Theme.fieldBg
        clip: true

        Flickable {
            id: flick
            anchors.fill: parent
            anchors.margins: 1 * Theme.scaleFactor
            // content 跟随 Image 实际尺寸（宽度铺满，高度按比例可能超高）。
            contentWidth: img.width
            contentHeight: img.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentX: 0

            Image {
                id: img
                // 宽度铺满容器；高度 = 宽度 / 图片宽高比（据 sourceSize）。
                width: root.width - 2 * Theme.scaleFactor
                height: (img.sourceSize.width > 0)
                        ? width * img.sourceSize.height / img.sourceSize.width
                        : root.height
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignLeft
                verticalAlignment: Image.AlignTop
                source: root.imageSource
                asynchronous: false
                cache: false
                smooth: true
            }
        }
    }
}
