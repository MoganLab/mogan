// Toggle.qml — 布尔开关行原子（label + 可选 hint + 右侧 on/off 胶囊开关）。
// 供需要布尔偏好的成品弹窗（首选项 Preferences）复用。与 EnumCombo 同一套
// 行布局契约（labelRatio / labelWidth / 行宽取 parent），以便同一 Column 内
// EnumCombo 行与 Toggle 行的左标签宽度对齐、不错位。
//
// on/off 胶囊：圆角矩形（radius: height/2，高的一半）；当前态高亮
// （Theme.selectBg/selectFg），滑块圆点居当前态、切换时带滑动动画。
// 整行可点（点 label 或 pill 都切换），hover 微变色。
//
// 内部 id 用 toggleRow（原子内部 id 不用 root，见 README 编码规矩）——
// 调用方 delegate 里的 root.xxx 指向调用方根，不被原子内部同名 id 遮蔽。
//
// API：
//   label        : string        —— 左侧标签文案。
//   hint         : string        —— 标签下副说明（如「仅 semantic editing 开时可见」），空则不占位。
//   value        : bool          —— 当前是否开。
//   labelRatio   : real          —— 标签占行宽比例，默认 0.42（与 EnumCombo 一致）。
//   rowHeight    : real          —— 行高，默认 Theme.rowH（与 EnumCombo 一致）。
//   fontScale    : real          —— 字体缩放，默认 1.0；双栏窄列传 < 1.0 缩小避免 label 挤换行。
//   toggled(bool)                —— 切换时发出，参数为新值。
//
// 须在 DialogShell 内（宽度由父行给定）。

import QtQuick
import "."

Row {
    id: toggleRow
    spacing: Theme.gapM

    property string label: ""
    property string hint: ""
    property bool value: false
    property real labelRatio: 0.42
    property real rowHeight: Theme.rowH
    property real fontScale: 1.0
    signal toggled(bool value)

    property real labelWidth: (parent ? parent.width : 0) * labelRatio
    height: rowHeight

    // 左：label（+ 可选 hint）。
    Column {
        width: toggleRow.labelWidth
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.toggleTextGap
        Text {
            text: toggleRow.label
            color: Theme.fg
            font.pixelSize: Theme.fontBody * toggleRow.fontScale
            wrapMode: Text.WordWrap
            width: parent.width
        }
        Text {
            text: toggleRow.hint
            color: Theme.muted
            font.pixelSize: Theme.fontMini
            wrapMode: Text.WordWrap
            width: parent.width
            visible: toggleRow.hint.length > 0
        }
    }

    // 右：on/off 胶囊开关。锚行右端（space-between：label 左、开关右），与设计稿 toggle-row 一致。
    Item {
        id: pill
        width: Theme.toggleW
        height: Theme.toggleH
        anchors.right: toggleRow.right
        anchors.verticalCenter: parent.verticalCenter

        Rectangle {
            id: track
            anchors.fill: parent
            radius: height / 2
            color: toggleRow.value ? Theme.selectBg : Theme.fieldBg
            border.width: Theme.borderW
            border.color: toggleRow.value ? Theme.selectBorder : Theme.borderClr
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }

        // 滑块圆点：knob 圆心对齐 track 两端圆弧圆心（track.height/2 处），缩放后仍紧贴两端、不偏。
        // x 用 binding + Behavior on x：切换瞬间 value 变、x 重算，平滑滑动。
        Rectangle {
            id: knob
            width: (pill.height - Theme.borderW * 2) * Theme.toggleKnob
            height: width
            radius: height / 2
            color: Theme.selectFg
            x: toggleRow.value
               ? track.width - track.height / 2 - width / 2
               : track.height / 2 - width / 2
            anchors.verticalCenter: parent.verticalCenter
            Behavior on x {
                NumberAnimation { duration: 120; easing.type: Easing.OutQuad }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: toggleRow.toggled(!toggleRow.value)
        }
    }
}
