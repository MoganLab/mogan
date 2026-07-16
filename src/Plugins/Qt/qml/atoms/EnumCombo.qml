// EnumCombo.qml — 下拉 combo 行（标签 + 当前值 + 箭头），不含浮层。
// 点开后把自己注册为所在 DialogShell 的 activeCombo，浮层定位/限高翻转由
// DialogShell 的共享 overlay 负责。
//
// key/value 分离：options 为英文 key（存储/回传/过滤用），optionsTr 等长同序为
// 翻译显示。空 optionsTr 时回退显示 options 原文（不翻译场景）。changed 回传英文 key。
//
// 可输入模式（editable: true）：默认仍是点一下弹浮层选；**双击**进入可编辑输入态
// （TextInput 覆盖，Enter/失焦落定发 changed，Esc 撤销退出编辑）。用于数值类字段
// （fn/tab/fns 等），让用户能键入预设外的自定义值。editable: false（默认）只能下拉选。
//
// API：
//   label        : string        —— 左侧标签文案。
//   options      : list<string>  —— 英文 key 列表（存储/回传/过滤用）。
//   optionsTr    : list<string>  —— 翻译显示列表，与 options 等长同序；空则显示原文。
//   value        : string        —— 当前英文 key。
//   editable     : bool          —— 是否允许双击进入可编辑输入态，默认 false。
//   labelRatio   : real          —— 标签占行宽比例，默认 0.42。
//   rowHeight    : real          —— 行高，默认 44×scaleFactor。
//   changed(string)              —— 选中新值时发出（英文 key，经 DialogShell 浮层 pick 触发，
//                                   或可编辑态落定时发出键入值）。
//
// 须在 DialogShell 内，宽度由父行给定。

import QtQuick

Row {
    id: comboRow
    spacing: 16 * Theme.scaleFactor

    property string label: ""
    property var options: []
    property var optionsTr: []
    property string value: ""
    property bool editable: false
    property real labelRatio: 0.42
    property real rowHeight: Theme.rowH
    signal changed(string value)

    // 按 objectName 沿 parent 链找 DialogShell（QML property 不能用 hasOwnProperty）。
    property var dialogShell: {
        var p = parent;
        while (p) {
            if (p.objectName === "DialogShell")
                return p;
            p = p.parent;
        }
        return null;
    }
    readonly property bool open: dialogShell && dialogShell.activeCombo === comboRow

    // 可编辑输入态：双击(editable=true)进入，Enter/失焦落定，Esc 撤销。
    property bool editing: false

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
        if (!dialogShell || !combo)
            return;
        var p = combo.mapToItem(dialogShell, 0, 0);
        comboX = p.x;
        comboY = p.y;
        comboW = combo.width;
    }

    function pick(v) {
        comboRow.changed(v);
    }

    // 单击 toggle 浮层：已展开则收起，否则注册为 activeCombo 弹出（共享浮层）。
    function toggleOpen() {
        if (!comboRow.dialogShell)
            return;
        if (comboRow.dialogShell.activeCombo === comboRow) {
            comboRow.dialogShell.activeCombo = null;
        } else {
            comboRow.updateGeometry();
            comboRow.dialogShell.activeCombo = comboRow;
        }
    }

    // 进入/退出可编辑态。editingInput.text 在进入时同步当前 value，撤销时丢弃。
    function startEdit() {
        if (!comboRow.editable)
            return;
        if (comboRow.dialogShell) {
            comboRow.dialogShell.activeCombo = null;
            // 注册为 editingCombo：DialogShell 据此铺点外遮罩——否则点空白处不丢
            // activeFocus（正文 Flickable/Item 不抢焦点），点外退不出编辑。
            comboRow.dialogShell.editingCombo = comboRow;
        }
        editingInput.text = comboRow.value;
        comboRow.editing = true;
        editingInput.forceActiveFocus();
        editingInput.selectAll();
    }
    function commitEdit() {
        if (!comboRow.editing)
            return;
        var v = editingInput.text;
        comboRow.editing = false;
        if (comboRow.dialogShell && comboRow.dialogShell.editingCombo === comboRow)
            comboRow.dialogShell.editingCombo = null;
        if (v !== comboRow.value)
            comboRow.changed(v);
        // 焦点还根：根 Item 非 FocusScope，隐藏 editingInput 不会自动恢复 activeFocus，
        // 不显式归还则后续 Esc 收不到，编辑后 Esc 关不掉窗。
        if (comboRow.dialogShell)
            comboRow.dialogShell.forceActiveFocus();
    }
    function cancelEdit() {
        comboRow.editing = false;
        if (comboRow.dialogShell && comboRow.dialogShell.editingCombo === comboRow)
            comboRow.dialogShell.editingCombo = null;
        if (comboRow.dialogShell)
            comboRow.dialogShell.forceActiveFocus();
    }

    readonly property bool hasTr: optionsTr && optionsTr.length === options.length
    readonly property var displayOptions: hasTr ? optionsTr : options
    readonly property string displayValue: {
        var i = options.indexOf(value);
        return (i >= 0 && hasTr) ? optionsTr[i] : value;
    }

    Text {
        width: comboRow.labelWidth
        anchors.verticalCenter: parent.verticalCenter
        text: comboRow.label
        color: Theme.fg
        font.pixelSize: 14 * Theme.scaleFactor
        elide: Text.ElideRight
    }

    Rectangle {
        id: combo
        width: comboRow.comboWidth
        height: comboRow.rowHeight
        anchors.verticalCenter: parent.verticalCenter
        radius: 8 * Theme.scaleFactor
        color: comboRow.editing ? Theme.fieldBgHover : (comboMa.containsMouse ? Theme.fieldBgHover : Theme.fieldBg)
        border.width: 1 * Theme.scaleFactor
        border.color: comboRow.editing ? Theme.accent : Theme.borderClr

        Text {
            anchors.fill: parent
            anchors.leftMargin: 14 * Theme.scaleFactor
            anchors.rightMargin: 30 * Theme.scaleFactor
            verticalAlignment: Text.AlignVCenter
            text: comboRow.displayValue
            color: Theme.fg
            font.pixelSize: 14 * Theme.scaleFactor
            elide: Text.ElideRight
            visible: !comboRow.editing
        }
        // 可编辑态覆盖的输入框；非编辑态隐藏，点一下仍走下面的 MouseArea 弹浮层。
        TextInput {
            id: editingInput
            anchors.fill: parent
            anchors.leftMargin: 14 * Theme.scaleFactor
            anchors.rightMargin: 30 * Theme.scaleFactor
            verticalAlignment: Text.AlignVCenter
            color: Theme.fg
            font.pixelSize: 14 * Theme.scaleFactor
            selectByMouse: true
            visible: comboRow.editing
            onActiveFocusChanged: if (!activeFocus)
                comboRow.commitEdit()
            Keys.onReturnPressed: comboRow.commitEdit()
            Keys.onEnterPressed: comboRow.commitEdit()
            Keys.onEscapePressed: comboRow.cancelEdit()
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 12 * Theme.scaleFactor
            anchors.verticalCenter: parent.verticalCenter
            text: comboRow.open ? "▲" : "▼"
            color: Theme.fg
            font.pixelSize: 10 * Theme.scaleFactor
            visible: !comboRow.editing
        }

        MouseArea {
            id: comboMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            // 单击/双击分离：双击会先触发两次 clicked，若 editable 则延迟 clicked
            // 处理，期间双击到达即取消（避免浮层先弹后收的闪烁 + 状态错乱）。
            // 不可编辑时无歧义，立即 toggle。
            onClicked: {
                if (comboRow.editing)
                    return;
                if (!comboRow.dialogShell)
                    return;
                if (!comboRow.editable) {
                    comboRow.toggleOpen();
                    return;
                }
                if (clickTimer.running) {
                    clickTimer.stop();
                    return;
                }
                clickTimer.restart();
            }
            onDoubleClicked: {
                if (comboRow.editable) {
                    clickTimer.stop();
                    comboRow.startEdit();
                }
            }
        }

        // 单击延迟触发：若期间双击到达则被 stop，不 toggle 浮层。
        Timer {
            id: clickTimer
            interval: 220
            repeat: false
            onTriggered: comboRow.toggleOpen()
        }
    }
}
