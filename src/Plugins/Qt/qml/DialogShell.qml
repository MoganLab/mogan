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
    id: root
    objectName: "DialogShell"
    anchors.fill: parent

    // 正文槽。挂载时 reparent 到 contentCol 并主动设 anchors.fill——调用方不能
    // 自己写 anchors.fill: parent：content 创建初的 parent 是 DialogShell 根，
    // anchors 绑定会钉死在旧 parent 上不随 reparent 更新，导致正文撑满全弹窗、
    // 无视 implicitMargins 边距。
    property Item content
    onContentChanged: {
        if (!content) return
        content.parent = contentCol
        content.anchors.fill = contentCol
    }

    implicitWidth: 400
    implicitHeight: 200
    property int implicitMargins: 24 * Theme.scaleFactor

    // ESC：浮层展开时先收起，否则取消整个弹窗。Enter/Return 默认无操作。
    property var onCancel: () => closeBridge.choose(-1)
    property var onActivate: null
    focus: true
    Keys.onEscapePressed: {
        if (root.activeCombo) root.activeCombo = null
        else if (root.onCancel) root.onCancel()
    }
    Keys.onReturnPressed: if (root.onActivate) root.onActivate()
    Keys.onEnterPressed: if (root.onActivate) root.onActivate()

    // 共享下拉浮层。EnumCombo 不自带浮层，点开后把自己设为 activeCombo（暴露
    // comboX/Y/W/options），本组件据此在 root 顶层渲染唯一一个 overlay。z 最高
    // 故不被正文遮挡；限高翻转算法与旧 FormDialog.qml 逐行一致（弹窗逻辑坐标系，
    // 不依赖 window/物理坐标），优先向下、不够向上、仍不够限高滚动。
    property var activeCombo: null

    Rectangle {
        anchors.fill: parent
        color: Theme.bg
        radius: 16 * Theme.scaleFactor
        border.width: 1 * Theme.scaleFactor
        border.color: Theme.borderClr

        DragHandler { target: null; onActiveChanged: if (active) closeBridge.startMove() }

        Item {
            id: contentCol
            anchors.fill: parent
            anchors.margins: root.implicitMargins
        }
    }

    // 展开时铺隐形遮罩拦截浮层外点击以收起（z 介于正文与浮层之间）。
    // 点在当前 combo 矩形内时不收起、只透传——让该 combo 的 onClicked 自己走
    // toggle 收起分支（它看到 activeCombo===self）；点在别处则先收起当前 combo
    // 再透传，让被点的另一个 combo 收到 onClicked 时看到 activeCombo===null 而
    // 展开。透传靠 propagateComposedEvents（仅作用于 click 等合成事件）+ onPressed
    // 里 accepted=false 放弃 grab，让 press 下沉到下层 MouseArea——否则 press 被
    // 遮罩吞掉，combo 收不到点击，切换需点两次（回归）。
    MouseArea {
        anchors.fill: parent
        visible: root.activeCombo !== null
        z: 500
        onPressed: function(mouse) {
            var c = root.activeCombo
            var inCombo = mouse.x >= c.comboX && mouse.x <= c.comboX + c.comboW
                       && mouse.y >= c.comboY && mouse.y <= c.comboY + c.comboH
            if (!inCombo) root.activeCombo = null
            mouse.accepted = false
        }
        propagateComposedEvents: true
    }

    Item {
        id: overlay
        visible: root.activeCombo !== null
        x: root.activeCombo ? root.activeCombo.comboX : 0
        width: root.activeCombo ? root.activeCombo.comboW : 0
        z: 1000

        readonly property real optH: root.activeCombo
                                     ? root.activeCombo.options.length * 36 * Theme.scaleFactor
                                     : 0
        readonly property real gap: 4 * Theme.scaleFactor
        readonly property real margin: 8 * Theme.scaleFactor
        readonly property real comboY: root.activeCombo ? root.activeCombo.comboY : 0
        readonly property real comboH: root.activeCombo ? root.activeCombo.comboH : 0
        readonly property real spaceBelow: root.height - comboY - comboH - margin
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
                model: root.activeCombo ? root.activeCombo.options : []
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
                            if (root.activeCombo) root.activeCombo.pick(modelData)
                            root.activeCombo = null
                        }
                    }
                }
            }
        }
    }
}
