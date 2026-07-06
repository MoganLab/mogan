// FormDialog.qml
// 通用 form 弹窗。按注入的 formFields（QVariantList，每项含 type/label/key/
// options/value）动态渲染控件。首版仅 enum（自绘下拉，不依赖 QtQuick.Controls，
// 与 ConfirmClose.qml 同保持纯 QtQuick）。
//
// context property（C++ 注入）：formFields、dialogButtons、dpScale、isDark、
// closeBridge。dialogButtons 经 qt_translate，跟随当前界面语言。
// OK：closeBridge.submit({key: value, ...})；Cancel：closeBridge.choose(-1)。
// root 自报 implicitWidth/Height，C++ 读取后 × DPI 锁定 QDialog。

import QtQuick

Item {
    id: root
    anchors.fill: parent
    property real scaleFactor: typeof dpScale !== "undefined" ? dpScale : 1.0
    property bool dark: typeof isDark !== "undefined" ? isDark : false
    property var fields: typeof formFields !== "undefined" ? formFields : []
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["OK", "Cancel"]

    // 各字段运行时值：modelData 只读，故拷贝到这里改写，OK 时统一提交。
    property var values: {
        var v = {}
        for (var i = 0; i < fields.length; i++) v[fields[i].key] = fields[i].value
        return v
    }
    property int openIndex: -1  // 当前展开的下拉 index，-1 全收起
    // 浮层不作为 combo 子项而挂在 root 顶层：combo 在 Repeater delegate 内，
    // 同 Column 后续 delegate（下方字段行）会盖住 combo 内部弹出的浮层，故
    // 把定位（comboX/Y/W）和选项（openOptions）由展开中的 combo 写回 root，
    // overlay 据此定位、叠在所有字段行之上。
    property real comboX: 0
    property real comboY: 0
    property real comboW: 0
    property var openOptions: []

    property int rowH: 44 * scaleFactor
    property int pad: 24 * scaleFactor
    implicitWidth: 420
    implicitHeight: pad * 2 + fields.length * (rowH + 12 * scaleFactor) + 64 * scaleFactor

    focus: true
    Keys.onEscapePressed: closeBridge.choose(-1)
    // 点击空白收起下拉（不吞事件，下层控件继续响应）。
    MouseArea { anchors.fill: parent; onPressed: function(mouse) { openIndex = -1 } }

    // 主题配色（与 ConfirmClose.qml 同源，首版复制，待第三个 QML 弹窗抽公共骨架）。
    readonly property color bg: dark ? "#2b2b2b" : "#ffffff"
    readonly property color fg: dark ? "#eaeaea" : "#1e1e1e"
    readonly property color accent: dark ? "#7a7a7a" : "#1e1e1e"
    readonly property color fieldBg: dark ? "#3d3d3d" : "#f1f3f4"
    readonly property color fieldBgHover: dark ? "#4e4e4e" : "#e5e7eb"
    readonly property color borderClr: dark ? "#3e3e3e" : "#d0d4da"

    Rectangle {
        anchors.fill: parent
        color: bg
        radius: 16 * scaleFactor
        border.width: 1 * scaleFactor
        border.color: borderClr

        DragHandler { target: null; onActiveChanged: if (active) closeBridge.startMove() }

        Column {
            anchors.fill: parent
            anchors.margins: root.pad
            spacing: 12 * scaleFactor

            Repeater {
                model: root.fields
                delegate: Row {
                    width: parent.width
                    height: root.rowH
                    spacing: 16 * scaleFactor

                    property int fieldIndex: index
                    property string fieldKey: modelData.key

                    Text {
                        id: lbl
                        width: parent.width * 0.42
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.label
                        color: root.fg
                        font.pixelSize: 14 * root.scaleFactor
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        id: combo
                        width: parent.width - parent.spacing - lbl.width
                        height: root.rowH
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 8 * root.scaleFactor
                        color: comboMa.containsMouse ? root.fieldBgHover : root.fieldBg
                        border.width: 1 * root.scaleFactor
                        border.color: root.borderClr

                        // 展开中的 combo 把自身坐标/尺寸写回 root 供 overlay 定位。
                        onYChanged: updatePos()
                        onXChanged: updatePos()
                        onWidthChanged: updatePos()
                        Component.onCompleted: updatePos()
                        function updatePos() {
                            if (root.openIndex !== fieldIndex) return
                            var p = mapToItem(root, 0, 0)
                            root.comboX = p.x
                            root.comboY = p.y
                            root.comboW = combo.width
                        }
                        Connections {
                            target: root
                            function onOpenIndexChanged() { combo.updatePos() }
                        }

                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: 14 * root.scaleFactor
                            anchors.rightMargin: 30 * root.scaleFactor
                            verticalAlignment: Text.AlignVCenter
                            text: root.values[modelData.key]
                            color: root.fg
                            font.pixelSize: 14 * root.scaleFactor
                            elide: Text.ElideRight
                        }
                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 12 * root.scaleFactor
                            anchors.verticalCenter: parent.verticalCenter
                            text: root.openIndex === fieldIndex ? "▲" : "▼"
                            color: root.fg
                            font.pixelSize: 10 * root.scaleFactor
                        }

                        MouseArea {
                            id: comboMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.openIndex === fieldIndex) {
                                    root.openIndex = -1
                                } else {
                                    root.openOptions = modelData.options
                                    root.openIndex = fieldIndex
                                    combo.updatePos()
                                }
                            }
                        }
                    }
                }
            }

            Item { width: 1; height: 8 * root.scaleFactor }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 16 * root.scaleFactor

                // 与 ConfirmClose.qml 同构：buttonLabels 由 C++ 注入（已翻译），
                // index 0 = 肯定（submit 表单值），其余 = 取消（choose(-1)）。
                Repeater {
                    model: root.buttonLabels
                    delegate: Rectangle {
                        width: 100 * root.scaleFactor
                        height: 40 * root.scaleFactor
                        radius: 20 * root.scaleFactor
                        color: ma.containsMouse
                               ? (primary ? (root.dark ? "#8a8a8a" : "#3a3a3a") : root.fieldBgHover)
                               : (primary ? root.accent : root.fieldBg)
                        border.width: primary ? 1 * root.scaleFactor : 0
                        border.color: primary ? root.accent : "transparent"

                        property bool primary: index === 0

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: primary ? "#ffffff" : root.fg
                            font.pixelSize: 15 * root.scaleFactor
                            font.weight: primary ? Font.Bold : Font.DemiBold
                        }
                        MouseArea {
                            id: ma
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onPressed: parent.scale = 0.96
                            onReleased: parent.scale = 1.0
                            onCanceled: parent.scale = 1.0
                            onClicked: primary ? closeBridge.submit(root.values)
                                               : closeBridge.choose(-1)
                        }
                        Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }
            }
        }
    }

    // 下拉浮层：root 直接子项（盖住所有字段行，不被下方行遮挡）。高度按可用
    // 空间限高——优先向下展开，下方不够则向上，仍不够则限高 + ListView 滚动，
    // 避免顶出 dialog 边界被裁。
    Item {
        id: overlay
        visible: root.openIndex >= 0
        x: root.comboX
        width: root.comboW
        z: 1000

        // 选项总高与上下两侧可用空间（留 8px 边距），决定展开方向与限高。
        readonly property real optH: root.openOptions.length * 36 * root.scaleFactor
        readonly property real gap: 4 * root.scaleFactor
        readonly property real margin: 8 * root.scaleFactor
        readonly property real spaceBelow: root.height - root.comboY - root.rowH - margin
        readonly property real spaceAbove: root.comboY - margin
        readonly property bool fitBelow: optH <= spaceBelow - gap
        readonly property bool fitAbove: optH <= spaceAbove - gap
        readonly property bool openBelow: fitBelow || (!fitAbove && spaceBelow >= spaceAbove)
        height: Math.min(optH, (openBelow ? spaceBelow : spaceAbove) - gap)
        y: openBelow ? root.comboY + root.rowH + gap
                     : root.comboY - height - gap

        Rectangle {
            anchors.fill: parent
            color: root.fieldBg
            radius: 8 * root.scaleFactor
            border.width: 1 * root.scaleFactor
            border.color: root.borderClr
            clip: true

            // 用 ListView：自带 contentY 管理与滚轮交互，比 Flickable+Column
            // 更适合"model + delegate + 限高滚动"场景。
            ListView {
                id: optList
                anchors.fill: parent
                clip: true
                interactive: true
                boundsBehavior: Flickable.StopAtBounds
                model: root.openOptions
                delegate: Rectangle {
                    width: overlay.width
                    height: 36 * root.scaleFactor
                    color: optMa.containsMouse ? root.fieldBgHover : root.fieldBg
                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 14 * root.scaleFactor
                        verticalAlignment: Text.AlignVCenter
                        text: modelData
                        color: root.fg
                        font.pixelSize: 14 * root.scaleFactor
                        elide: Text.ElideRight
                    }
                    MouseArea {
                        id: optMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var v = root.values
                            v[root.fields[root.openIndex].key] = modelData
                            root.values = v
                            root.openIndex = -1
                        }
                    }
                }
            }
        }
    }
}
