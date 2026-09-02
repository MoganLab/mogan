// InputField.qml — 单行自由输入原子（label + 可选行内按钮 + 右侧输入框）。
// 供需要「键入数值/路径/搜索词」的成品弹窗复用。
// 与 EnumCombo 同风格（fieldBg 底 + 圆角 + activeFocus 高亮边框），但直接键入，
// 不弹浮层。
//
// 内部 id 用 inputRow（原子内部 id 不用 root，见 README 编码规矩）——调用方
// delegate 里的 root.xxx 指向调用方根，不被原子内部同名 id 遮蔽。
//
// value 双向契约（避开 TextInput 的 text binding 在用户编辑时被打断的坑）：
//   - text 故意不绑定 value：TextInput 的 text 一旦被用户键入即失去绑定，若绑定
//     value，后续外部改 value（如 Browse）不会刷新显示。故初值经
//     Component.onCompleted 注入，用户键入 onTextChanged 回写 value，外部改 value
//     经 onValueChanged 再写回 text——三者都收敛到同一字符串，无循环。
//
// API：
//   label         : string  —— 左侧标签文案。
//   value         : string  —— 当前输入值（初始 + 双向：编辑/键入会回写，外部改也刷新）。
//   placeholder   : string  —— 空值占位文案。
//   numeric       : bool    —— 是否数字输入（限制仅数字键，非法字符不进入）。
//   isNarrow      : bool    —— 落在双栏半宽列时 label 占比调窄。
//   actionLabel   : string  —— 行内按钮文案（如「Browse」）；非空则在 label 与输入框
//                                之间渲染按钮，发 actionClicked。空则无按钮。
//   rowHeight     : real    —— 行高，默认 Theme.rowH。
//   changed(string)        —— 键入值时发出（当前文本）。
//   actionClicked          —— 行内按钮点击时发出（调用方决定行为，如浏览文件）。
//   accepted               —— 输入框回车（调用方可 submit）。
//
// 须在 DialogShell 内，宽度由父行给定。

import QtQuick
import "."

Row {
    id: inputRow
    spacing: Theme.gapM

    property string label: ""
    property string value: ""
    property string placeholder: ""
    property bool numeric: false
    property bool isNarrow: false
    property string actionLabel: ""
    property real rowHeight: Theme.rowH
    signal changed(string value)
    signal actionClicked
    signal accepted

    readonly property bool hasAction: actionLabel.length > 0
    readonly property real labelRatio: isNarrow ? Theme.comboLabelRatioNarrow : 0.35
    // 用自身 width（由调用方设成父列宽），不要读 parent.width：Column 宽度依赖子项时
    // parent.width 会是 0，标签宽变成 0，「文件名」和长路径叠在一起。
    property real labelWidth: inputRow.width * labelRatio
    readonly property real actionWidth: hasAction ? actionBtn.width : 0
    property real inputWidth: Math.max(0, inputRow.width - labelWidth - actionWidth
                              - (hasAction ? 2 * spacing : spacing))
    height: rowHeight
    clip: true

    onValueChanged: {
        if (inputTxt.text !== inputRow.value)
            inputTxt.text = inputRow.value;
    }

    Text {
        width: inputRow.labelWidth
        anchors.verticalCenter: parent.verticalCenter
        text: inputRow.label
        color: Theme.fg
        font.pixelSize: Theme.fontBody
        elide: Text.ElideRight
    }

    MiniButton {
        id: actionBtn
        size: "normal"
        visible: inputRow.hasAction
        width: inputRow.hasAction ? implicitWidth : 0
        text: inputRow.actionLabel
        onClicked: inputRow.actionClicked()
    }

    Rectangle {
        width: inputRow.inputWidth
        height: inputRow.rowHeight
        anchors.verticalCenter: parent.verticalCenter
        radius: Theme.radius
        clip: true
        color: inputTxt.activeFocus ? Theme.fieldBgHover : Theme.fieldBg
        border.width: Theme.borderW
        border.color: inputTxt.activeFocus ? Theme.accent : Theme.borderClr

        Text {
            anchors.fill: parent
            anchors.leftMargin: Theme.comboPad
            anchors.rightMargin: Theme.comboPad
            verticalAlignment: Text.AlignVCenter
            visible: inputTxt.text.length === 0 && inputRow.placeholder.length > 0
            text: inputRow.placeholder
            color: Theme.muted
            font.pixelSize: Theme.fontBody
        }

        TextInput {
            id: inputTxt
            anchors.fill: parent
            anchors.leftMargin: Theme.comboPad
            anchors.rightMargin: Theme.comboPad
            verticalAlignment: Text.AlignVCenter
            color: Theme.fg
            font.pixelSize: Theme.fontBody
            selectByMouse: true
            clip: true
            inputMethodHints: inputRow.numeric ? Qt.ImhDigitsOnly : Qt.ImhNone
            validator: inputRow.numeric ? numValidator : null
            Component.onCompleted: inputTxt.text = inputRow.value
            onTextChanged: {
                if (inputTxt.text !== inputRow.value) {
                    inputRow.value = inputTxt.text;
                    inputRow.changed(inputTxt.text);
                }
            }
            onAccepted: inputRow.accepted()
            onActiveFocusChanged: {
                if (inputTxt.activeFocus) inputTxt.selectAll();
            }

            RegularExpressionValidator {
                id: numValidator
                regularExpression: /^[0-9]*$/
            }
        }
    }
}
