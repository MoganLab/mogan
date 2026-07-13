// EnumCombo.qml — 下拉 combo 行（标签 + 当前值 + 箭头），不含浮层。
// 点开后把自己注册为所在 DialogShell 的 activeCombo，浮层定位/限高翻转由
// DialogShell 的共享 overlay 负责。
//
// key/value 分离：options 为英文 key（存储/回传/过滤用），optionsTr 等长同序为
// 翻译显示。空 optionsTr 时回退显示 options 原文（不翻译场景）。changed 回传英文 key。
//
// API：
//   label        : string        —— 左侧标签文案。
//   options      : list<string>  —— 英文 key 列表（存储/回传/过滤用）。
//   optionsTr    : list<string>  —— 翻译显示列表，与 options 等长同序；空则显示原文。
//   value        : string        —— 当前英文 key。
//   labelRatio   : real          —— 标签占行宽比例，默认 0.42。
//   rowHeight    : real          —— 行高，默认 44×scaleFactor。
//   changed(string)              —— 选中新值时发出（英文 key，经 DialogShell 浮层 pick 触发）。
//
// 须在 DialogShell 内，宽度由父行给定。

import QtQuick

Row {
    id: root
    spacing: 16 * Theme.scaleFactor

    property string label: ""
    property var options: []
    property var optionsTr: []
    property string value: ""
    property real labelRatio: 0.42
    property real rowHeight: 44 * Theme.scaleFactor
    signal changed(string value)

    // 按 objectName 沿 parent 链找 DialogShell（QML property 不能用 hasOwnProperty）。
    property var dialogShell: {
        var p = parent
        while (p) {
            if (p.objectName === "DialogShell") return p
            p = p.parent
        }
        return null
    }
    readonly property bool open: dialogShell && dialogShell.activeCombo === root

    property real labelWidth: (parent ? parent.width : 0) * labelRatio
    property real comboWidth: (parent ? parent.width : 0) - labelWidth - spacing
    height: rowHeight

    // 暴露给 DialogShell overlay 的几何（dialogShell 坐标系）。展开时由
    // updateGeometry() 算一次，不用 binding 实时算——布局未完成时 mapToItem 给出
    // stale 值会导致浮层定位偏。
    property real comboX: 0
    property real comboY: 0
    property real comboW: 0
    property real comboH: rowHeight
    function updateGeometry() {
        if (!dialogShell || !combo) return
        var p = combo.mapToItem(dialogShell, 0, 0)
        comboX = p.x
        comboY = p.y
        comboW = combo.width
    }

    function pick(v) { root.changed(v) }

    readonly property bool hasTr: optionsTr && optionsTr.length === options.length
    readonly property var displayOptions: hasTr ? optionsTr : options
    readonly property string displayValue: {
        var i = options.indexOf(value)
        return (i >= 0 && hasTr) ? optionsTr[i] : value
    }

    Text {
        width: root.labelWidth
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        color: Theme.fg
        font.pixelSize: 14 * Theme.scaleFactor
        elide: Text.ElideRight
    }

    Rectangle {
        id: combo
        width: root.comboWidth
        height: root.rowHeight
        anchors.verticalCenter: parent.verticalCenter
        radius: 8 * Theme.scaleFactor
        color: comboMa.containsMouse ? Theme.fieldBgHover : Theme.fieldBg
        border.width: 1 * Theme.scaleFactor
        border.color: Theme.borderClr

        Text {
            anchors.fill: parent
            anchors.leftMargin: 14 * Theme.scaleFactor
            anchors.rightMargin: 30 * Theme.scaleFactor
            verticalAlignment: Text.AlignVCenter
            text: root.displayValue
            color: Theme.fg
            font.pixelSize: 14 * Theme.scaleFactor
            elide: Text.ElideRight
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 12 * Theme.scaleFactor
            anchors.verticalCenter: parent.verticalCenter
            text: root.open ? "▲" : "▼"
            color: Theme.fg
            font.pixelSize: 10 * Theme.scaleFactor
        }

        MouseArea {
            id: comboMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (!root.dialogShell) return
                if (root.dialogShell.activeCombo === root) {
                    root.dialogShell.activeCombo = null
                } else {
                    root.updateGeometry()
                    root.dialogShell.activeCombo = root
                }
            }
        }
    }
}
