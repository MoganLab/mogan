// WaitProgressDialog.qml — 通用等待中间态弹窗（异步任务期间的用户反馈）。
// 与 UpdaterProgress.qml 同构：无限转圈 + 已翻译文案，无进度条/百分比。
// 走 run_modal_qml_dialog（setModal + show，非阻塞模态）：任务链由 scheme
// 轮询驱动（delayed → g_http-poll），主线程必须回到事件循环，否则转圈动画
// 冻死、轮询回调也不执行。
// 通用组件：文案由调用方经 cpp-wait-dialog-open(message) 传入（OCR 识别、
// 其他魔法粘贴等异步任务共用）。
// 生命周期：scheme 侧在发起异步任务前调 cpp-wait-dialog-open 打开，任务完成
// （成功插入/失败通知）前调 cpp-wait-dialog-close 关闭；幂等由 C++ 侧
// g_wait_dialog_host 保证。
// 可取消：ESC 与 Cancel 按钮均调 waitCancelBridge.cancel()——宿主 close 同步
// 析构（不走 closeBridge.cancel 的 done，后者对 show 型只 hide 不析构会泄漏），
// 并经 eval_scheme 回流 (wait-dialog-cancelled) 置 scheme 侧取消标志，拦截
// 后续识别/插入动作（不中止已在飞的网络请求，结果到达后被忽略；成功结果
// 仍写缓存）。
//
// context property（C++ 注入）：dialogMessage（已翻译）、dialogButtons（已
// 翻译的按钮文案）、dpScale、isDark、closeBridge、waitCancelBridge。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 420
    implicitHeight: 240
    implicitMargins: 28 * Theme.scaleFactor

    property string message: typeof dialogMessage !== "undefined" ? dialogMessage : ""
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["Cancel"]

    // 取消动作统一入口：waitCancelBridge 由 C++ 注入；typeof 保护与
    // dialogMessage 同风格——未注入（如旧二进制/测试环境）时降级 no-op 而非
    // JS 报错，症状是「取消无响应」而非弹窗炸掉，更易诊断。
    function cancelWait() {
        if (typeof waitCancelBridge !== "undefined") waitCancelBridge.cancel()
    }

    // ESC → 用户取消（覆盖 DialogShell 默认的 closeBridge.cancel：done(Rejected)
    // 对 show 型弹窗只 hide 不析构，且不走取消回流）。
    onCancel: () => cancelWait()

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

            // 取消按钮：经 waitCancelBridge.cancel() 回流 scheme 侧取消。
            DialogButtons {
                anchors.horizontalCenter: parent.horizontalCenter
                buttonLabels: root.buttonLabels
                onClicked: function(i) { root.cancelWait() }
            }
        }
    }
}
