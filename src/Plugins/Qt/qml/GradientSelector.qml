// GradientSelector.qml — 渐变选择器 QML 对话框。
// 提供渐变图片选择、宽高尺寸设置、前景色与背景色选取，以及右侧实时渐变预览。
// 交互经 gradBridge 处理并支持原生 ColorPicker 颜色选取。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 720 * Theme.scaleFactor
    implicitHeight: 460 * Theme.scaleFactor
    onCancel: () => gradBridge.cancel()
    onActivate: () => gradBridge.submit()

    property var labels: gradBridge.labels

    content: Item {
        anchors.fill: parent

        // 标题行
        Row {
            id: titleRow
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: Theme.titleH

            Text {
                text: root.labels.title
                color: Theme.fg
                font.pixelSize: 18 * Theme.scaleFactor
                font.weight: Font.Bold
            }
        }

        // 中间主体：左侧属性控制列，右侧实时预览区
        Item {
            id: middleArea
            anchors.top: titleRow.bottom
            anchors.topMargin: Theme.gapM
            anchors.bottom: bottomRow.top
            anchors.bottomMargin: Theme.gapM
            anchors.left: parent.left
            anchors.right: parent.right

            Column {
                id: leftCol
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: parent.width * 0.53
                spacing: Theme.gapS

                // 渐变底图选择
                EnumCombo {
                    width: parent.width
                    labelWidth: 70 * Theme.scaleFactor
                    label: root.labels.pattern
                    options: gradBridge.patternOptions
                    optionsTr: gradBridge.patternOptionsTr
                    value: gradBridge.patternName
                    actionLabel: root.labels.browse
                    onActionClicked: gradBridge.browsePatternFile()
                    onChanged: function (v) { gradBridge.setPatternName(v); }
                }

                // 宽度
                EnumCombo {
                    width: parent.width
                    labelWidth: 70 * Theme.scaleFactor
                    label: root.labels.width
                    options: ["100%", "100@", "1cm", "2cm", "5cm"]
                    value: gradBridge.width
                    editable: true
                    onChanged: function (v) { gradBridge.setWidth(v); }
                }

                // 高度
                EnumCombo {
                    width: parent.width
                    labelWidth: 70 * Theme.scaleFactor
                    label: root.labels.height
                    options: ["100%", "100@", "1cm", "2cm", "5cm"]
                    value: gradBridge.height
                    editable: true
                    onChanged: function (v) { gradBridge.setHeight(v); }
                }

                // 前景色
                EnumCombo {
                    width: parent.width
                    labelWidth: 70 * Theme.scaleFactor
                    label: root.labels.foreground
                    options: gradBridge.colorOptions
                    optionsTr: gradBridge.colorOptionsTr
                    value: gradBridge.foregroundColor
                    actionLabel: root.labels.pickColor
                    onActionClicked: gradBridge.pickForegroundColor()
                    onChanged: function (v) { gradBridge.setForegroundColor(v); }
                }

                // 背景色
                EnumCombo {
                    width: parent.width
                    labelWidth: 70 * Theme.scaleFactor
                    label: root.labels.background
                    options: gradBridge.colorOptions
                    optionsTr: gradBridge.colorOptionsTr
                    value: gradBridge.backgroundColor
                    actionLabel: root.labels.pickColor
                    onActionClicked: gradBridge.pickBackgroundColor()
                    onChanged: function (v) { gradBridge.setBackgroundColor(v); }
                }
            }

            // 右侧实时预览面板
            Rectangle {
                id: previewBox
                anchors.left: leftCol.right
                anchors.leftMargin: Theme.gapM
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                radius: Theme.radius
                color: Theme.fieldBg
                clip: true

                Image {
                    id: previewImg
                    anchors.fill: parent
                    source: gradBridge.previewUrl
                    fillMode: (gradBridge.width === "100%" && gradBridge.height === "100%") 
                              ? Image.Stretch : Image.PreserveAspectFit
                    smooth: true
                    cache: false
                }

                // 预览提示角标
                Rectangle {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 8 * Theme.scaleFactor
                    width: previewTag.implicitWidth + 12 * Theme.scaleFactor
                    height: 20 * Theme.scaleFactor
                    radius: 4 * Theme.scaleFactor
                    color: Theme.dark ? "#90000000" : "#90ffffff"

                    Text {
                        id: previewTag
                        anchors.centerIn: parent
                        text: root.labels.preview
                        font.pixelSize: Theme.fontMini
                        color: Theme.fg
                    }
                }
            }
        }

        // 底部按钮栏
        Item {
            id: bottomRow
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: Theme.btnH

            DialogButtons {
                anchors.right: parent.right
                buttonLabels: [root.labels.ok, root.labels.cancel]
                onClicked: function (idx) {
                    if (idx === 0) gradBridge.submit();
                    else gradBridge.cancel();
                }
            }
        }
    }
}
