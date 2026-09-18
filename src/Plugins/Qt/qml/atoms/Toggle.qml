// Toggle.qml — 布尔开关行原子（label + 可选 hint + 右侧 on/off 胶囊开关）。
// 供需要布尔偏好的成品弹窗（首选项 Preferences）复用。
//
// on/off 胶囊：圆角矩形（radius: height/2，高的一半）；当前态高亮
// （Theme.selectBg/selectFg），滑块圆点居当前态、切换时带滑动动画。
// 整行可点（点 label 或 pill 都切换），hover 微变色。
//
// 外层用 Item（非 Row）：label 锚左、胶囊锚右，space-between 布局，半宽列下右侧
// 不留大片空白。Row 的子项不能用 left/right/centerIn anchors（Qt 报错），故用 Item。
//
// 内部 id 用 toggleRow（原子内部 id 不用 root，见 README 编码规矩）——
// 调用方 delegate 里的 root.xxx 指向调用方根，不被原子内部同名 id 遮蔽。
//
// API：
//   label        : string        —— 左侧标签文案。
//   hint         : string        —— 标签下副说明（如「仅 semantic editing 开时可见」），空则不占位。
//   tooltip      : string        —— 悬浮提示文案（如展开折叠环境说明），非空时显示问号图标与悬浮气泡。
//   value        : bool          —— 当前是否开。
//   isNarrow     : bool          —— 是否落在双栏半宽列；true 时原子自动缩小字体 + label 占更大比例。
//   enabled      : bool          —— 是否可交互（内置）；false 时锁定：label 灰显、胶囊不可点
//                                    （用于 latex transparent/Store tracking：source-tracking 关时锁定）。
//   rowHeight    : real          —— 行高，默认 Theme.rowH。
//   toggled(bool)                —— 切换时发出，参数为新值。
//
// 须在 DialogShell 内（宽度由父行给定）。

import QtQuick
import "."

Item {
    id: toggleRow

    property string label: ""
    property string hint: ""
    property string tooltip: ""
    property bool value: false
    property bool isNarrow: false
    property real rowHeight: Theme.rowH
    signal toggled(bool value)

    // 双栏半宽列：label 占更大比例（长 label 如 "Show only semantic focus" 不挤换行），
    // 字体缩小（twoColFontScale）。比例/缩放由 isNarrow 内部决定，调用方不感知布局。
    property real labelRatio: isNarrow ? Theme.toggleLabelRatioNarrow : Theme.toggleLabelRatio
    property real fontScale: isNarrow ? Theme.twoColFontScale : 1.0
    property real labelWidth: toggleRow.width * labelRatio
    height: rowHeight
    z: (typeof helpMa !== "undefined" && helpMa.containsMouse) ? 100 : 1

    // 左：label（+ 可选 hint + 可选问号帮助图标）。
    Column {
        id: labelCol
        width: toggleRow.labelWidth
        anchors.left: toggleRow.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.toggleTextGap

        Row {
            id: labelRow
            spacing: 6 * Theme.scaleFactor
            width: parent.width

            Text {
                id: labelText
                text: toggleRow.label
                color: toggleRow.enabled ? Theme.fg : Theme.muted
                font.pixelSize: Theme.fontBody * toggleRow.fontScale
                wrapMode: Text.WordWrap
                width: toggleRow.tooltip.length > 0
                       ? Math.min(implicitWidth, parent.width - 24 * Theme.scaleFactor)
                       : parent.width
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                id: helpIcon
                visible: toggleRow.tooltip.length > 0
                width: 16 * Theme.scaleFactor
                height: 16 * Theme.scaleFactor
                radius: 8 * Theme.scaleFactor
                anchors.verticalCenter: parent.verticalCenter
                color: helpMa.containsMouse
                       ? (Theme.dark ? "#505050" : "#d0d4da")
                       : (Theme.dark ? "#3a3a3a" : "#e5e7eb")
                border.width: Theme.borderW
                border.color: helpMa.containsMouse ? Theme.accent : Theme.borderClr

                Text {
                    anchors.centerIn: parent
                    text: "?"
                    font.pixelSize: 11 * Theme.scaleFactor
                    font.bold: true
                    color: helpMa.containsMouse ? Theme.fg : Theme.muted
                }

                MouseArea {
                    id: helpMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                }
            }
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

    // 隐藏用于测量单行宽度的文本项
    Text {
        id: dummyTipText
        visible: false
        text: toggleRow.tooltip
        font.pixelSize: 12 * Theme.scaleFactor
    }

    // 悬停提示框
    Rectangle {
        id: tipBubble
        visible: toggleRow.tooltip.length > 0 && helpMa.containsMouse
        z: 1000
        width: Math.min(320 * Theme.scaleFactor, Math.max(120 * Theme.scaleFactor, dummyTipText.implicitWidth + 20 * Theme.scaleFactor))
        height: tipTextItem.implicitHeight + 14 * Theme.scaleFactor
        radius: 6 * Theme.scaleFactor
        color: Theme.dark ? "#1f1f1f" : "#2d3748"
        border.width: Theme.borderW
        border.color: Theme.dark ? "#444444" : "#4a5568"

        x: {
            if (!visible || !helpIcon.visible) return 0;
            var pos = helpIcon.mapToItem(toggleRow, 0, 0);
            var idealX = pos.x + helpIcon.width / 2 - width / 2;
            return Math.max(0, Math.min(idealX, toggleRow.width - width));
        }
        y: {
            if (!visible || !helpIcon.visible) return 0;
            var pos = helpIcon.mapToItem(toggleRow, 0, 0);
            var topY = pos.y - height - 6 * Theme.scaleFactor;
            return (topY < 0) ? (pos.y + helpIcon.height + 6 * Theme.scaleFactor) : topY;
        }

        Text {
            id: tipTextItem
            x: 8 * Theme.scaleFactor
            y: 7 * Theme.scaleFactor
            width: parent.width - 16 * Theme.scaleFactor
            text: toggleRow.tooltip
            font.pixelSize: 12 * Theme.scaleFactor
            color: "#ffffff"
            wrapMode: Text.WordWrap
            lineHeight: 1.2
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
                ColorAnimation {
                    duration: 150
                }
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
            x: toggleRow.value ? track.width - track.height / 2 - width / 2 : track.height / 2 - width / 2
            anchors.verticalCenter: parent.verticalCenter
            Behavior on x {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutQuad
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: toggleRow.enabled
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: toggleRow.toggled(!toggleRow.value)
        }
    }
}
