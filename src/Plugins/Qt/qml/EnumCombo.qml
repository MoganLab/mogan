// EnumCombo.qml — 下拉的 combo 行样式。
// 只渲染一行（标签 + 当前值 + 箭头），不含浮层。点开后把自己注册为所在
// DialogShell 的 activeCombo，浮层定位/限高翻转由 DialogShell 的共享 overlay 负责
//（那套算法已验证可靠，所有弹窗复用）。原子化为复用样式，不强求黑盒——定位交
// 调用方（DialogShell）。
//
// API：
//   label        : string        —— 左侧标签文案。
//   options      : list<string>  —— 可选项。
//   value        : string        —— 当前值。
//   labelRatio   : real          —— 标签占行宽比例，默认 0.42。
//   rowHeight    : real          —— 行高，默认 44×scaleFactor。
//   changed(string)              —— 选中新值时发出（经 DialogShell 浮层 pick 触发）。
//
// 用法（须在 DialogShell 内，宽度由父行给定）：
//   EnumCombo {
//       width: parent.width
//       label: "Font:"; options: ["rm", "tt"]; value: "rm"
//       onChanged: function(v) { /* 写回 */ }
//   }

import QtQuick

Row {
    id: root
    spacing: 16 * Theme.scaleFactor

    property string label: ""
    property var options: []
    property string value: ""
    property real labelRatio: 0.42
    property real rowHeight: 44 * Theme.scaleFactor
    signal changed(string value)

    // 沿 parent 链按 objectName 找 DialogShell（不用 hasOwnProperty，对 QML property
    // 不可靠）。
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

    // 暴露给 DialogShell overlay 的几何（dialogShell 坐标系）。点击展开时由
    // updateGeometry() 算一次——不用 binding 实时算，避免布局未完成时 mapToItem
    // 给出 stale 值导致浮层定位偏。
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
            text: root.value
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
