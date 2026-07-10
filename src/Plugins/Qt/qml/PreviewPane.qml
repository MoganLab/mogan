// PreviewPane.qml — 预览图区壳。显示 fontBridge 光栅化的样本 PNG（data URL）。
//
// 宽高比跟随图片：容器宽度由父给定，高度 = 宽度 / 图片宽高比，保证展示区与图片
// 同比例、不 letterbox 不变形。图片加载后据 sourceSize 算宽高比；未加载时用
// defaultAspect（默认 4:1，样本是多行长文本）占位。
//
// API：
//   imageSource   : string —— "data:image/png;base64,..."。
//   defaultAspect : real   —— 未加载时的宽/高比，默认 4.0（宽:高）。
//   maxWidth      : real   —— 容器最大宽度（限图片很扁时不撑爆），默认绑父宽。
//
// 光栅化产物已含 retina factor，Image 按逻辑尺寸等比缩放。

import QtQuick

Item {
    id: root

    property string imageSource: ""
    property real defaultAspect: 4.0
    property real maxWidth: 0  // 0 = 不限
    // 当前宽高比（宽/高）。图片加载后据 sourceSize 更新。
    property real aspect: defaultAspect

    // 容器宽度 = 父宽（或受 maxWidth 限），高度据宽高比。
    implicitHeight: width / aspect

    Rectangle {
        id: bg
        anchors.fill: parent
        color: Theme.fieldBg
        radius: 8 * Theme.scaleFactor
        border.width: 1 * Theme.scaleFactor
        border.color: Theme.borderClr
        clip: true

        Image {
            id: img
            anchors.fill: parent
            anchors.margins: 1 * Theme.scaleFactor
            fillMode: Image.PreserveAspectFit
            source: root.imageSource
            asynchronous: false
            cache: false
            onStatusChanged: {
                if (status === Image.Ready && sourceSize.width > 0) {
                    root.aspect = sourceSize.width / sourceSize.height
                }
            }
        }
    }
}
