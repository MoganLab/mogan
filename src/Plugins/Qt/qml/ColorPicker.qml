// ColorPicker.qml — 现代感 QML 调色板。
// 基于 DialogShell + Theme 原子组装，替换原生 QColorDialog，保持
// interactive-color / interactive-background 调用链不变。
//
// context property（C++ 注入）：pickerTitleProp、initialColorProp、
// proposalsProp、pickPatternProp、customColorsProp、labelsProp、
// dialogButtonsProp、colorBridge、closeBridge、dpScale、isDark。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 560 * Theme.scaleFactor
    implicitHeight: 720 * Theme.scaleFactor
    implicitMargins: 20 * Theme.scaleFactor

    // 由 C++ 注入的初始颜色（#rrggbb 字符串）与预设列表。
    property string initialColor: typeof initialColorProp !== "undefined" ? initialColorProp : "#ffffff"
    property var proposals: typeof proposalsProp !== "undefined" ? proposalsProp : []
    property bool pickPattern: typeof pickPatternProp !== "undefined" ? pickPatternProp : false
    property var labels: typeof labelsProp !== "undefined" ? labelsProp : ({})
    property var bridge: typeof colorBridge !== "undefined" ? colorBridge : null

    // 当前选中颜色（QML color 类型）。
    property color currentColor: root.initialColor

    // HSV 工作态：hue / saturation / value 均归一化到 [0,1]。
    property real hue: 0
    property real saturation: 0
    property real value: 1

    // 解析 initialColor 到 HSV。
    Component.onCompleted: {
        var c = Qt.rgba(currentColor.r, currentColor.g, currentColor.b, 1.0);
        root.hue = c.hsvHue;
        root.saturation = c.hsvSaturation;
        root.value = c.hsvValue;
    }

    // HSV 变化时更新 currentColor。
    onHueChanged: root.currentColor = Qt.hsva(root.hue, root.saturation, root.value, 1.0)
    onSaturationChanged: root.currentColor = Qt.hsva(root.hue, root.saturation, root.value, 1.0)
    onValueChanged: root.currentColor = Qt.hsva(root.hue, root.saturation, root.value, 1.0)

    // HEX 表示（小写，不带 #）。
    property string hexValue: {
        var toHex = function (v) {
            var h = Math.round(v * 255).toString(16);
            return h.length === 1 ? "0" + h : h;
        };
        return toHex(root.currentColor.r) + toHex(root.currentColor.g) + toHex(root.currentColor.b);
    }

    property var rgbValue: {
        return {
            r: Math.round(root.currentColor.r * 255),
            g: Math.round(root.currentColor.g * 255),
            b: Math.round(root.currentColor.b * 255)
        };
    }

    property var hsvValue: {
        return {
            h: Math.round(root.hue * 359),
            s: Math.round(root.saturation * 100),
            v: Math.round(root.value * 100)
        };
    }

    // 默认预设色板（现代感常用色），仅在 proposals 为空时启用。
    property var defaultPresets: [
        "#000000", "#434343", "#666666", "#999999", "#b7b7b7",
        "#980000", "#ff0000", "#ff9900", "#ffff00", "#00ff00",
        "#00ffff", "#4a86e8", "#0000ff", "#9900ff", "#ff00ff",
        "#e6b8af", "#f4cccc", "#fce5cd", "#fff2cc", "#d9ead3"
    ]

    property var presetColors: root.proposals.length > 0 ? root.proposals : root.defaultPresets

    // 自定义颜色（16 格，OK 时由 C++ 持久化到 preference）。
    property var customColors: {
        var init = typeof customColorsProp !== "undefined" ? customColorsProp : [];
        var arr = [];
        for (var i = 0; i < 16; i++)
            arr.push(i < init.length ? init[i] : "");
        return arr;
    }
    // 当前选中的自定义色格下标（删除对象）；-1 表示未选中。
    property int selectedCustomIndex: -1
    // 色格已满仍点添加时显示提示行。
    property bool customFullHint: false

    function addToCustomColors() {
        var arr = root.customColors.slice();
        var idx = arr.indexOf("");
        if (idx < 0) {
            // 已满：拒绝添加，提示需先删除。
            root.customFullHint = true;
            return;
        }
        arr[idx] = "#" + root.hexValue;
        root.customColors = arr;
        root.customFullHint = false;
    }

    function deleteCustomColor() {
        if (root.selectedCustomIndex < 0)
            return;
        var arr = root.customColors.slice();
        arr[root.selectedCustomIndex] = "";
        root.customColors = arr;
        root.selectedCustomIndex = -1;
        root.customFullHint = false;
    }

    function setColorFromHex(hex) {
        root.currentColor = hex;
        var c = Qt.rgba(root.currentColor.r, root.currentColor.g, root.currentColor.b, 1.0);
        root.hue = c.hsvHue;
        root.saturation = c.hsvSaturation;
        root.value = c.hsvValue;
    }

    onCancel: closeBridge.cancel()

    // 屏幕取色结果异步回流（bridge 隐藏宿主弹窗后全屏取色，结束经信号回传）。
    Connections {
        target: root.bridge
        function onScreenColorPicked(hex) {
            if (hex !== "") root.setColorFromHex(hex);
        }
    }

    content: Column {
        spacing: 12 * Theme.scaleFactor
        anchors.fill: parent

        // 标题
        Text {
            text: typeof pickerTitleProp !== "undefined" ? pickerTitleProp : qsTr("Choose color")
            color: Theme.fg
            font.pixelSize: Theme.fontBody
            font.bold: true
        }

        // 主编辑区
        Row {
            width: parent.width
            height: 280 * Theme.scaleFactor
            spacing: 16 * Theme.scaleFactor

            // 左侧：饱和度/明度面板 + 色相条
            Column {
                width: parent.width - 148 * Theme.scaleFactor
                height: parent.height
                spacing: 10 * Theme.scaleFactor

                // 饱和度/明度面板
                Item {
                    id: svPanel
                    width: parent.width
                    height: parent.height - Theme.rowH - parent.spacing

                    Canvas {
                        id: svCanvas
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d");
                            var w = width;
                            var h = height;
                            for (var y = 0; y < h; y++) {
                                var v = 1 - y / h;
                                var grad = ctx.createLinearGradient(0, 0, w, 0);
                                grad.addColorStop(0, Qt.hsva(root.hue, 0, v, 1).toString());
                                grad.addColorStop(1, Qt.hsva(root.hue, 1, v, 1).toString());
                                ctx.fillStyle = grad;
                                ctx.fillRect(0, y, w, 1);
                            }
                        }
                    }

                    Rectangle {
                        id: svKnob
                        width: 12 * Theme.scaleFactor
                        height: width
                        radius: width / 2
                        color: "transparent"
                        border.width: 2 * Theme.scaleFactor
                        border.color: root.value > 0.5 ? "#000000" : "#ffffff"
                        x: root.saturation * svPanel.width - width / 2
                        y: (1 - root.value) * svPanel.height - height / 2
                    }

                    MouseArea {
                        anchors.fill: parent
                        function updateFromMouse(mouse) {
                            root.saturation = Math.max(0, Math.min(1, mouse.x / svPanel.width));
                            root.value = Math.max(0, Math.min(1, 1 - mouse.y / svPanel.height));
                        }
                        onPressed: updateFromMouse(mouse)
                        onPositionChanged: if (pressed) updateFromMouse(mouse)
                    }

                    // hue 变化后重绘 SV 面板
                    Connections {
                        target: root
                        function onHueChanged() { svCanvas.requestPaint(); }
                    }
                }

                // 色相条
                Item {
                    id: huePanel
                    width: parent.width
                    height: Theme.rowH

                    Canvas {
                        id: hueCanvas
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d");
                            var grad = ctx.createLinearGradient(0, 0, width, 0);
                            for (var i = 0; i <= 6; i++) {
                                grad.addColorStop(i / 6, Qt.hsva(i / 6, 1, 1, 1).toString());
                            }
                            ctx.fillStyle = grad;
                            ctx.fillRect(0, 0, width, height);
                        }
                    }

                    Rectangle {
                        id: hueKnob
                        width: 8 * Theme.scaleFactor
                        height: parent.height + 4 * Theme.scaleFactor
                        radius: 2 * Theme.scaleFactor
                        color: "#ffffff"
                        border.width: 1 * Theme.scaleFactor
                        border.color: "#000000"
                        x: root.hue * huePanel.width - width / 2
                        y: -2 * Theme.scaleFactor
                    }

                    MouseArea {
                        anchors.fill: parent
                        function updateFromMouse(mouse) {
                            root.hue = Math.max(0, Math.min(1, mouse.x / huePanel.width));
                        }
                        onPressed: updateFromMouse(mouse)
                        onPositionChanged: if (pressed) updateFromMouse(mouse)
                    }
                }
            }

            // 右侧：预览 + HEX + RGB + HSV
            Column {
                width: 132 * Theme.scaleFactor
                height: parent.height
                spacing: 8 * Theme.scaleFactor

                // 颜色预览
                Rectangle {
                    width: parent.width
                    height: 36 * Theme.scaleFactor
                    radius: Theme.radius
                    color: root.currentColor
                    border.width: Theme.borderW
                    border.color: Theme.borderClr
                }

                // HEX 输入
                Column {
                    width: parent.width
                    spacing: 3 * Theme.scaleFactor

                    Text {
                        text: "HEX"
                        color: Theme.muted
                        font.pixelSize: Theme.fontTiny
                    }

                    Rectangle {
                        width: parent.width
                        height: 30 * Theme.scaleFactor
                        radius: Theme.radius
                        color: Theme.fieldBg
                        border.width: Theme.borderW
                        border.color: Theme.borderClr

                        TextInput {
                            id: hexInput
                            anchors.fill: parent
                            anchors.margins: Theme.padS
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignHCenter
                            color: Theme.fg
                            font.pixelSize: Theme.fontBody
                            text: "#" + root.hexValue
                            selectByMouse: true
                            validator: RegularExpressionValidator { regularExpression: /^#[0-9A-Fa-f]{6}$/ }
                            onEditingFinished: root.setColorFromHex(text)
                        }
                    }
                }

                // RGB 输入
                Column {
                    width: parent.width
                    spacing: 4 * Theme.scaleFactor

                    Repeater {
                        model: ["R", "G", "B"]
                        delegate: Row {
                            width: parent.width
                            spacing: 5 * Theme.scaleFactor

                            Text {
                                width: 14 * Theme.scaleFactor
                                text: modelData
                                color: Theme.muted
                                font.pixelSize: Theme.fontTiny
                                verticalAlignment: Text.AlignVCenter
                                height: 26 * Theme.scaleFactor
                            }

                            Rectangle {
                                width: parent.width - 19 * Theme.scaleFactor
                                height: 26 * Theme.scaleFactor
                                radius: Theme.radius
                                color: Theme.fieldBg
                                border.width: Theme.borderW
                                border.color: Theme.borderClr

                                TextInput {
                                    anchors.fill: parent
                                    anchors.margins: Theme.padS
                                    verticalAlignment: TextInput.AlignVCenter
                                    horizontalAlignment: TextInput.AlignRight
                                    color: Theme.fg
                                    font.pixelSize: Theme.fontBody
                                    text: modelData === "R" ? root.rgbValue.r : (modelData === "G" ? root.rgbValue.g : root.rgbValue.b)
                                    selectByMouse: true
                                    validator: IntValidator { bottom: 0; top: 255 }
                                    onEditingFinished: {
                                        var v = parseInt(text, 10);
                                        var r = root.rgbValue.r;
                                        var g = root.rgbValue.g;
                                        var b = root.rgbValue.b;
                                        if (modelData === "R") r = v;
                                        else if (modelData === "G") g = v;
                                        else b = v;
                                        var c = Qt.rgba(r / 255, g / 255, b / 255, 1.0);
                                        root.hue = c.hsvHue;
                                        root.saturation = c.hsvSaturation;
                                        root.value = c.hsvValue;
                                    }
                                }
                            }
                        }
                    }
                }

                // HSV 输入
                Column {
                    width: parent.width
                    spacing: 4 * Theme.scaleFactor

                    Repeater {
                        model: ["H", "S", "V"]
                        delegate: Row {
                            width: parent.width
                            spacing: 5 * Theme.scaleFactor

                            Text {
                                width: 14 * Theme.scaleFactor
                                text: modelData
                                color: Theme.muted
                                font.pixelSize: Theme.fontTiny
                                verticalAlignment: Text.AlignVCenter
                                height: 26 * Theme.scaleFactor
                            }

                            Rectangle {
                                width: parent.width - 19 * Theme.scaleFactor
                                height: 26 * Theme.scaleFactor
                                radius: Theme.radius
                                color: Theme.fieldBg
                                border.width: Theme.borderW
                                border.color: Theme.borderClr

                                TextInput {
                                    anchors.fill: parent
                                    anchors.margins: Theme.padS
                                    verticalAlignment: TextInput.AlignVCenter
                                    horizontalAlignment: TextInput.AlignRight
                                    color: Theme.fg
                                    font.pixelSize: Theme.fontBody
                                    text: modelData === "H" ? root.hsvValue.h : (modelData === "S" ? root.hsvValue.s : root.hsvValue.v)
                                    selectByMouse: true
                                    validator: IntValidator { bottom: 0; top: modelData === "H" ? 359 : 100 }
                                    onEditingFinished: {
                                        var v = parseInt(text, 10);
                                        if (modelData === "H") root.hue = Math.max(0, Math.min(359, v)) / 359;
                                        else if (modelData === "S") root.saturation = Math.max(0, Math.min(100, v)) / 100;
                                        else root.value = Math.max(0, Math.min(100, v)) / 100;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 基础颜色
        Text {
            text: root.labels.basicColors !== undefined ? root.labels.basicColors : "Basic colors"
            color: Theme.muted
            font.pixelSize: Theme.fontTiny
        }

        Grid {
            width: parent.width
            columns: 10
            spacing: 5 * Theme.scaleFactor

            Repeater {
                model: root.presetColors
                delegate: Rectangle {
                    width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns
                    height: width
                    radius: 3 * Theme.scaleFactor
                    color: modelData
                    border.width: Theme.borderW
                    border.color: root.hexValue === String(modelData).replace("#", "") ? Theme.selectBorder : Theme.borderClr

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.setColorFromHex(modelData)
                    }
                }
            }
        }

        // 自定义颜色
        Row {
            width: parent.width
            spacing: Theme.gapM

            Text {
                text: root.labels.customColors !== undefined ? root.labels.customColors : "Custom colors"
                color: Theme.muted
                font.pixelSize: Theme.fontTiny
                verticalAlignment: Text.AlignVCenter
                height: parent.height
                width: 92 * Theme.scaleFactor
            }

            Column {
                width: parent.width - 92 * Theme.scaleFactor - customBtns.width - 2 * Theme.gapM
                spacing: 4 * Theme.scaleFactor

                Grid {
                    width: parent.width
                    columns: 8
                    spacing: 5 * Theme.scaleFactor

                    Repeater {
                        model: root.customColors
                        delegate: Rectangle {
                            width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns
                            height: width
                            radius: 3 * Theme.scaleFactor
                            color: modelData !== "" ? modelData : Theme.fieldBg
                            border.width: index === root.selectedCustomIndex ? 2 * Theme.scaleFactor : Theme.borderW
                            border.color: index === root.selectedCustomIndex ? Theme.selectBorder : (root.hexValue === String(modelData).replace("#", "") ? Theme.selectBorder : Theme.borderClr)

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: modelData !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: {
                                    if (modelData !== "") {
                                        root.setColorFromHex(modelData);
                                        root.selectedCustomIndex = index;
                                    }
                                }
                            }
                        }
                    }
                }

                // 满员拒绝提示
                Text {
                    visible: root.customFullHint
                    text: root.labels.customFull !== undefined ? root.labels.customFull : "Custom colors are full, delete one first"
                    color: Theme.muted
                    font.pixelSize: Theme.fontTiny
                }
            }

            Column {
                id: customBtns
                spacing: 8 * Theme.scaleFactor
                anchors.verticalCenter: parent.verticalCenter

                MiniButton {
                    id: addBtn
                    size: "normal"
                    width: Math.max(addBtn.implicitWidth, delBtn.implicitWidth)
                    text: root.labels.addToCustom !== undefined ? root.labels.addToCustom : "Add to custom colors"
                    onClicked: root.addToCustomColors()
                }

                MiniButton {
                    id: delBtn
                    size: "normal"
                    width: Math.max(addBtn.implicitWidth, delBtn.implicitWidth)
                    text: root.labels.deleteCustom !== undefined ? root.labels.deleteCustom : "Delete custom color"
                    opacity: root.selectedCustomIndex >= 0 ? 1.0 : 0.5
                    onClicked: root.deleteCustomColor()
                }
            }
        }

        // 屏幕取色（Wayland 不支持抓屏，bridge 的 canPickScreen 为 false，隐藏按钮）
        MiniButton {
            size: "normal"
            text: root.labels.pickScreenColor !== undefined ? root.labels.pickScreenColor : "Pick screen color"
            visible: root.bridge !== null && root.bridge.canPickScreen === true
            onClicked: root.bridge.pickScreenColor()
        }

        // 按钮
        DialogButtons {
            anchors.horizontalCenter: parent.horizontalCenter
            buttonLabels: typeof dialogButtonsProp !== "undefined" ? dialogButtonsProp : [qsTr("OK"), qsTr("Cancel")]
            onClicked: function (index) {
                if (index === 0) {
                    var customs = [];
                    for (var i = 0; i < root.customColors.length; i++)
                        if (root.customColors[i] !== "") customs.push(root.customColors[i]);
                    closeBridge.submit({ color: "#" + root.hexValue, customColors: customs });
                } else {
                    closeBridge.cancel();
                }
            }
        }
    }
}
