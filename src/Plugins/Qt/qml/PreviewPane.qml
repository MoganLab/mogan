// PreviewPane.qml — 预览图区壳。显示 fontBridge 光栅化的样本 PNG（data URL）。
//
// 固定高度（外部给定），内部 Flickable 可滚动：样本类型切换（标准→中日韩）时
// 内容变高，不撑破布局，而是纵向滚动浏览。Image 按逻辑尺寸（物理像素 /
// Theme.scaleFactor，已含 retina factor）原样显示，不二次缩放 → 清晰。
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
        radius: 8 * Theme.scaleFactor
        border.width: 1 * Theme.scaleFactor
        border.color: Theme.borderClr
        clip: true

        Flickable {
            id: flick
            anchors.fill: parent
            anchors.margins: 1 * Theme.scaleFactor
            contentWidth: img.width
            contentHeight: img.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            // 仅纵向滚动（样本是宽接近容器、高度可变的长文本）。
            contentX: 0

            Image {
                id: img
                // 光栅化产物物理像素 = 逻辑 × retina_factor（qt_widget_rasterize 已乘）。
                // 这里按逻辑尺寸显示（物理像素 / scaleFactor），与屏幕设备像素 1:1，
                // 无有损缩放。宽度铺满容器，高度按图片原比例（可能超出容器 → 滚动）。
                width: root.width - 2 * Theme.scaleFactor
                sourceSize.width: width
                fillMode: Image.Pad
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
