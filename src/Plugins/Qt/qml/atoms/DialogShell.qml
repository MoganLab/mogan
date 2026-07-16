// DialogShell.qml — 可复用弹窗外壳。
// 集中所有 QML 弹窗共有的骨架：圆角背景、主题（Theme 单例）、无边框拖动、ESC、
// content 正文槽、键盘回调、共享下拉浮层。各弹窗只提供正文 Item。
//
// API：
//   content          : Item   —— 正文（不要写 anchors.fill，挂载时由本组件设）。
//   implicitMargins  : int    —— 正文四周边距，默认 24×scaleFactor。
//   onActivate       : var    —— Enter/Return 回调（默认 no-op）。
//   onCancel         : var    —— ESC 且无下拉展开时的回调（默认 choose(-1)）。
//   activeCombo      : var    —— 当前展开的 EnumCombo（共享浮层用，通常不直接设）。
//
// 用法：
//   DialogShell {
//       implicitWidth: 420; implicitHeight: 200
//       content: Column { ... }
//       onActivate: () => closeBridge.choose(1)
//   }
//
// closeBridge/dpScale/isDark 由 C++ 经 context property 注入。正文内引用根属性用
// id（content 被 reparent 到内部容器，parent 链不可靠）。

import QtQuick

Item {
    id: shell
    objectName: "DialogShell"
    anchors.fill: parent

    // 正文槽。挂载时 reparent 到 contentCol 并主动设 anchors.fill——调用方不能
    // 自己写 anchors.fill: parent：content 创建初的 parent 是 DialogShell 根，
    // anchors 绑定会钉死在旧 parent 上不随 reparent 更新，导致正文撑满全弹窗、
    // 无视 implicitMargins 边距。
    property Item content
    onContentChanged: {
        if (!content)
            return;
        content.parent = contentCol;
        content.anchors.fill = contentCol;
    }

    implicitWidth: 400
    implicitHeight: 200
    property int implicitMargins: 24 * Theme.scaleFactor

    // ESC：浮层展开时先收起，否则取消整个弹窗。Enter/Return 默认无操作。
    property var onCancel: () => closeBridge.choose(-1)
    property var onActivate: null
    focus: true
    Keys.onEscapePressed: {
        if (shell.activeCombo)
            shell.activeCombo = null;
        else if (shell.onCancel)
            shell.onCancel();
    }
    Keys.onReturnPressed: if (shell.onActivate)
        shell.onActivate()
    Keys.onEnterPressed: if (shell.onActivate)
        shell.onActivate()

    // 共享下拉浮层。EnumCombo 不自带浮层，点开后把自己设为 activeCombo（暴露
    // comboX/Y/W/options），本组件据此在顶层渲染唯一一个 overlay。z 最高
    // 故不被正文遮挡；限高翻转算法与旧 FormDialog.qml 逐行一致（弹窗逻辑坐标系，
    // 不依赖 window/物理坐标），优先向下、不够向上、仍不够限高滚动。
    property var activeCombo: null
    // 当前处于编辑态的 EnumCombo（与 activeCombo 对称）。非 null 时铺点外遮罩提交编辑
    // （否则 editingInput activeFocus 不丢，点外退不出编辑，见 EnumCombo.startEdit）。
    property var editingCombo: null

    Rectangle {
        anchors.fill: parent
        color: Theme.bg
        radius: 16 * Theme.scaleFactor
        border.width: 1 * Theme.scaleFactor
        border.color: Theme.borderClr

        DragHandler {
            target: null
            onActiveChanged: if (active)
                closeBridge.startMove()
        }

        Item {
            id: contentCol
            anchors.fill: parent
            anchors.margins: shell.implicitMargins
        }
    }

    // 展开时铺隐形遮罩拦截浮层外点击以收起。点在当前 combo 矩形内时不收起、只
    // 透传，让该 combo 的 onClicked 走 toggle 收起分支；点在别处则先收起当前 combo
    // 再透传，让被点的 combo 看到 activeCombo===null 而展开。透传靠
    // propagateComposedEvents + onPressed 里 accepted=false 放弃 grab，让 press 下沉——
    // 否则 press 被遮罩吞掉，combo 收不到点击，切换需点两次（回归）。
    MouseArea {
        anchors.fill: parent
        visible: shell.activeCombo !== null
        z: 500
        onPressed: function (mouse) {
            var c = shell.activeCombo;
            var inCombo = mouse.x >= c.comboX && mouse.x <= c.comboX + c.comboW && mouse.y >= c.comboY && mouse.y <= c.comboY + c.comboH;
            if (!inCombo)
                shell.activeCombo = null;
            mouse.accepted = false;
        }
        propagateComposedEvents: true
    }

    // 编辑态点外遮罩：点空白处提交编辑，透传 press 让被点元素（如下一个 combo）正常响应，
    // 与下拉遮罩透传模式一致，避免「点两次才切换」回归。提交与焦点归还都在 commitEdit 内。
    MouseArea {
        anchors.fill: parent
        visible: shell.editingCombo !== null
        z: 500
        onPressed: function (mouse) {
            if (shell.editingCombo)
                shell.editingCombo.commitEdit();
            mouse.accepted = false;
        }
        propagateComposedEvents: true
    }

    Item {
        id: overlay
        visible: shell.activeCombo !== null
        x: shell.activeCombo ? shell.activeCombo.comboX : 0
        width: shell.activeCombo ? shell.activeCombo.comboW : 0
        z: 1000

        readonly property real optH: shell.activeCombo ? shell.activeCombo.options.length * 36 * Theme.scaleFactor : 0
        readonly property real gap: 4 * Theme.scaleFactor
        readonly property real margin: 8 * Theme.scaleFactor
        readonly property real comboY: shell.activeCombo ? shell.activeCombo.comboY : 0
        readonly property real comboH: shell.activeCombo ? shell.activeCombo.comboH : 0
        readonly property real spaceBelow: shell.height - comboY - comboH - margin
        readonly property real spaceAbove: comboY - margin
        readonly property bool fitBelow: optH <= spaceBelow - gap
        readonly property bool fitAbove: optH <= spaceAbove - gap
        readonly property bool openBelow: fitBelow || (!fitAbove && spaceBelow >= spaceAbove)
        height: Math.min(optH, (openBelow ? spaceBelow : spaceAbove) - gap)
        y: openBelow ? comboY + comboH + gap : comboY - height - gap

        Rectangle {
            anchors.fill: parent
            color: Theme.fieldBg
            radius: 8 * Theme.scaleFactor
            border.width: 1 * Theme.scaleFactor
            border.color: Theme.borderClr
            clip: true

            ListView {
                id: optList
                anchors.fill: parent
                clip: true
                interactive: true
                boundsBehavior: Flickable.StopAtBounds
                // 显示用 displayOptions（EnumCombo 按是否传 optionsTr 决定翻译显示），
                // 但 pick 回传 options[index]（英文 key），保证存储层收到原值。
                model: shell.activeCombo ? shell.activeCombo.displayOptions : []
                delegate: Rectangle {
                    width: optList.width
                    height: 36 * Theme.scaleFactor
                    color: optMa.containsMouse ? Theme.fieldBgHover : Theme.fieldBg
                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 14 * Theme.scaleFactor
                        verticalAlignment: Text.AlignVCenter
                        text: modelData
                        color: Theme.fg
                        font.pixelSize: 14 * Theme.scaleFactor
                        elide: Text.ElideRight
                    }
                    MouseArea {
                        id: optMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (shell.activeCombo)
                                shell.activeCombo.pick(shell.activeCombo.options[index]);
                            shell.activeCombo = null;
                        }
                    }
                }
            }
        }
    }
}
