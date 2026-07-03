// FormDialog.qml
// 通用 form 弹窗。按注入的 formFields（QVariantList，每项含 type/label/key/
// options/value）动态渲染控件。首版仅 enum（自绘下拉，不依赖 QtQuick.Controls，
// 与 ConfirmClose.qml 同保持纯 QtQuick）。
//
// context property（C++ 注入）：formFields、dpScale、isDark、closeBridge。
// OK：closeBridge.submit({key: value, ...})；Cancel：closeBridge.choose(-1)。
// root 自报 implicitWidth/Height，C++ 读取后 × DPI 锁定 QDialog。

import QtQuick

Item {
    id: root
    anchors.fill: parent
    property real scaleFactor: typeof dpScale !== "undefined" ? dpScale : 1.0
    property bool dark: typeof isDark !== "undefined" ? isDark : false
    property var fields: typeof formFields !== "undefined" ? formFields : []

    // 各字段运行时值：拷贝自注入的 fields，下拉改动写回这里，OK 时统一收集。
    // 用数组 + index 改写（modelData 只读）。
    property var values: {
        var v = {}
        for (var i = 0; i < fields.length; i++) v[fields[i].key] = fields[i].value
        return v
    }
    // 同一时刻最多一个下拉展开；记录其 index，-1 表示全收起。
    property int openIndex: -1

    property int rowH: 44 * scaleFactor
    property int pad: 24 * scaleFactor
    implicitWidth: 420
    implicitHeight: pad * 2 + fields.length * (rowH + 12 * scaleFactor) + 64 * scaleFactor

    focus: true
    Keys.onEscapePressed: closeBridge.choose(-1)
    // 点击空白处收起所有下拉。
    MouseArea { anchors.fill: parent; onPressed: function(mouse) {
        openIndex = -1
        // 不吞事件，让下层控件继续响应。
    } }

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
                            onClicked: root.openIndex = (root.openIndex === fieldIndex ? -1 : fieldIndex)
                        }

                        // 展开浮层。
                        Column {
                            visible: root.openIndex === fieldIndex
                            anchors.top: parent.bottom
                            anchors.topMargin: 4 * root.scaleFactor
                            width: combo.width
                            z: 100

                            Repeater {
                                model: root.openIndex === fieldIndex ? modelData.options : []
                                delegate: Rectangle {
                                    width: combo.width
                                    height: 36 * root.scaleFactor
                                    color: optMa.containsMouse ? root.fieldBgHover : root.fieldBg
                                    border.width: 1 * root.scaleFactor
                                    border.color: root.borderClr
                                    Text {
                                        anchors.fill: parent
                                        anchors.leftMargin: 14 * root.scaleFactor
                                        verticalAlignment: Text.AlignVCenter
                                        text: modelData
                                        color: root.fg
                                        font.pixelSize: 14 * root.scaleFactor
                                    }
                                    MouseArea {
                                        id: optMa
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        // option 的 modelData 是字符串；字段 key 在
                                        // 外层 delegate 的 fieldKey（option delegate
                                        // 的 modelData 遮蔽了字段级 modelData）。
                                        onClicked: {
                                            var v = root.values
                                            v[combo.parent.fieldKey] = modelData
                                            root.values = v
                                            root.openIndex = -1
                                        }
                                    }
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

                Rectangle {
                    width: 100 * root.scaleFactor
                    height: 40 * root.scaleFactor
                    radius: 20 * root.scaleFactor
                    color: okMa.containsMouse ? (root.dark ? "#8a8a8a" : "#3a3a3a") : root.accent
                    border.width: 1 * root.scaleFactor
                    border.color: root.accent
                    Text {
                        anchors.centerIn: parent
                        text: "OK"
                        color: "#ffffff"
                        font.pixelSize: 15 * root.scaleFactor
                        font.weight: Font.Bold
                    }
                    MouseArea {
                        id: okMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onPressed: parent.scale = 0.96
                        onReleased: parent.scale = 1.0
                        onClicked: closeBridge.submit(root.values)
                    }
                    Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }
                }

                Rectangle {
                    width: 100 * root.scaleFactor
                    height: 40 * root.scaleFactor
                    radius: 20 * root.scaleFactor
                    color: cancelMa.containsMouse ? root.fieldBgHover : root.fieldBg
                    Text {
                        anchors.centerIn: parent
                        text: "Cancel"
                        color: root.fg
                        font.pixelSize: 15 * root.scaleFactor
                    }
                    MouseArea {
                        id: cancelMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: closeBridge.choose(-1)
                    }
                }
            }
        }
    }
}
