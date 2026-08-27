// UpdaterProgress.qml — 更新下载中间态弹窗（切换通道时的下载反馈）。
// 走 run_modal_qml_dialog（setModal + show，非阻塞模态）：scheme 轮询链在状态
// 2（AVAILABLE，触发下载）时无条件打开本弹窗，不依赖 Velopack 是否报出
// DOWNLOADING(3) 状态——下载可能已进行而状态机无输出，此时弹窗照常出现。
// 只提示「正在下载」：无限转圈 + 已翻译文案，无进度条/百分比（不读
// updater-progress）。下载完成（READY）后 scheme 调 cpp-updater-dialog-close
// 关窗，再弹重启确认。
// 下载中不可 ESC/X 关闭（onCancel no-op）：切通道的下载是强制步骤，中途关窗
// 只丢反馈、下载本身照常，没有取消路径。
//
// context property（C++ 注入）：dialogMessage（已翻译）、dpScale、isDark、
// closeBridge。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 420
    implicitHeight: 180
    implicitMargins: 28 * Theme.scaleFactor

    property string message: typeof dialogMessage !== "undefined" ? dialogMessage : ""

    // 覆盖 DialogShell 默认的 ESC 取消（closeBridge.cancel 会关掉宿主）。
    onCancel: () => {}

    // content 填满正文区（DialogShell 强制 anchors.fill）；内层 Column 垂直居中。
    // 外层 Item 不可省：它承接 anchors.fill，让 Column 用 verticalCenter 居中。
    content: Item {
        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 18 * Theme.scaleFactor

            // 无限转圈指示器：fieldBg 轨道环 + accent 弧段绕中心无限旋转。
            // Canvas 只画 120° 弧，整体 RotationAnimation 转圈即成「加载中」。
            Item {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 36 * Theme.scaleFactor
                height: 36 * Theme.scaleFactor

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: "transparent"
                    border.width: 3 * Theme.scaleFactor
                    border.color: Theme.fieldBg
                }

                Canvas {
                    anchors.fill: parent
                    antialiasing: true
                    property real lineW: 3 * Theme.scaleFactor
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.lineWidth = lineW
                        ctx.lineCap = "round"
                        ctx.strokeStyle = Theme.accent
                        ctx.beginPath()
                        // 12 点方向起顺时针 120° 弧（-π/2 → π/6）。
                        ctx.arc(width / 2, height / 2,
                                width / 2 - lineW / 2,
                                -Math.PI / 2, Math.PI / 6, false)
                        ctx.stroke()
                    }
                }

                RotationAnimation on rotation {
                    from: 0
                    to: 360
                    duration: 900
                    loops: Animation.Infinite
                }
            }

            Text {
                width: parent.width
                text: root.message
                color: Theme.fg
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                font.pixelSize: 16 * Theme.scaleFactor
                font.weight: Font.Bold
            }
        }
    }
}
