// Toggle.qml — 布尔开关行原子（label + 可选 hint + 右侧 on/off 胶囊开关）。
// 供需要布尔偏好的成品弹窗（首选项 Preferences）复用。与 EnumCombo 同一套
// 行布局契约（labelRatio / labelWidth / 行宽取 parent），以便同一 Column 内
// EnumCombo 行与 Toggle 行的左标签宽度对齐、不错位。
//
// on/off 胶囊：圆角矩形（radius: height/2，高的一半），左 on / 右 off 两段；
// 当前态高亮（Theme.selectBg/selectFg），滑块圆点居当前态、切换时带滑动动画。
// 整行可点（点 label 也切换），hover 微变色。
//
// 内部 id 用 toggleRow（原子内部 id 不用 root，见 README 编码规矩）——
// 调用方 delegate 里的 root.xxx 指向调用方根，不被原子内部同名 id 遮蔽。
//
// API：
//   label        : string        —— 左侧标签文案。
//   hint         : string        —— 标签下副说明（如「仅 semantic editing 开时可见」），空则不占位。
//   value         : bool          —— 当前是否开。
//   labelRatio   : real          —— 标签占行宽比例，默认 0.42（与 EnumCombo 一致）。
//   rowHeight    : real          —— 行高，默认 44×scaleFactor（与 EnumCombo 一致）。
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
    signal toggled(bool value)

    property real labelWidth: (parent ? parent.width : 0) * labelRatio
    property real switchWidth: 64 * Theme.scaleFactor   // 胶囊宽（与 Theme 常量风格匹配，给两段 + 滑块留足空间）
    height: rowHeight

    // 左：label（+ 可选 hint）。Column 左对齐、垂直居中行。
    Column {
        width: toggleRow.labelWidth
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2 * Theme.scaleFactor
        Text {
            text: toggleRow.label
            color: Theme.fg
            font.pixelSize: Theme.fontBody
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

    // 右：on/off 胶囊开关。点整行 toggle。
    Item {
        id: pill
        width: toggleRow.switchWidth
        height: 28 * Theme.scaleFactor
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

        // 滑块圆点：居当前态（on -> 右，off -> 左）。
        Rectangle {
            id: knob
            width: (pill.height - Theme.borderW * 2) * 0.9
            height: width
            radius: height / 2
            color: Theme.selectFg
            // 切换动画：用 x binding + Behavior on x 平滑滑动（切换瞬间 value 变，x 重算）。
            x: toggleRow.value
                   ? track.width - width - Theme.padS * 0.5
                   : Theme.padS * 0.5
            anchors.verticalCenter: parent.verticalCenter
            Behavior on x {
                NumberAnimation { duration: 120; easing.type: Easing.OutQuad }
            }
        }

        // hover / 点击均作用在整 pill（含 track 的 transparent 区）。
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: toggleRow.toggled(!toggleRow.value)
        }
    }

    // 行内剩余空白可被调用方 reparent 为 info-row（如 last check 时间），不在此主动占。
    // 让 label 占 labelRatio、switch 占 switchWidth、剩余空白靠 Row 默认靠左对齐，
    // 与 EnumCombo 的 comboWidth 占满剩余不同——开关右端对齐靠调用方 anchor 即可（首选项首版开关紧跟标签右侧）。
}
