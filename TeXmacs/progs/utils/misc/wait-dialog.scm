;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : wait-dialog.scm
;; DESCRIPTION : generic wait dialog (spinner + cancel) for async tasks
;; COPYRIGHT   : (C) 2026  Mogan STEM authors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils misc wait-dialog))

;; 通用等待弹窗的 scheme 门面：包装 Qt 侧 glue（cpp-wait-dialog-open/close，
;; QML WaitProgressDialog：无限转圈 + 已翻译文案 + Cancel，可 ESC 取消）。
;; 供 OCR 识别、其他魔法粘贴等异步任务共用；任务链须由 delayed 轮询驱动
;; （非阻塞模态弹窗，主线程回到事件循环动画才转得动，见 devel/0521.md）。

;; 当前注册的取消回调（弹窗全局单例 -> 同时仅一个在飞任务，单 thunk 足够；
;; 下次打开覆盖注册，任务正常完成后残留无害——弹窗已关，不会再有取消事
;; 件）。默认 no-op，未注册时回流调用不报错。

(define current-cancel-thunk (lambda () #f))

;; wait-dialog-open
;; 打开等待弹窗，可选用 opt-on-cancel 注册取消回调（缺省为 no-op）
;;
;; 语法
;; ----
;; (wait-dialog-open msg [on-cancel])
;;
;; 参数
;; ----
;; msg - 待翻译的英文文案 key（translate 在本 GPL 层完成，调用方/goldfish
;;       编排层只传纯数据字符串）
;; on-cancel - 可选 thunk，用户取消（ESC/Cancel 按钮）时被调用；传入则
;;             弹窗展示「取消」按钮，不传则无取消按钮亦不注册取消回调

(tm-define (wait-dialog-open msg . opt-on-cancel)
  (if (pair? opt-on-cancel)
    (begin
      (set! current-cancel-thunk (car opt-on-cancel))
      (cpp-wait-dialog-open (translate msg) #t)
    ) ;begin
    (begin
      (set! current-cancel-thunk (lambda () #f))
      (cpp-wait-dialog-open (translate msg) #f)
    ) ;begin
  ) ;if
) ;tm-define

;; 关闭等待弹窗（程序性关闭：不触发取消回流；幂等，未打开时 no-op）。

(tm-define (wait-dialog-close) (cpp-wait-dialog-close))

;; C++ 取消回流入口：用户 ESC/点取消时，Qt 侧 WaitDialogBridge 经
;; eval_scheme 按名调用本函数（模式同 paragraph-format-commit），转发到
;; 当前注册的 on-cancel；回调属主是编排层（如 goldfish (liii ocr-impl) 的
;; per-task 闭包），置位后由各异步回调入口守卫拦截后续动作。

(tm-define (wait-dialog-cancelled) (current-cancel-thunk))

;; wait-dialog-run
;; 在等待弹窗下调度执行任务：延迟 pause 毫秒（让 Qt 事件循环完成 QML
;; 转圈首帧渲染，不能 open 后同调用栈立即执行）后运行 thunk，正常完成
;; 即关闭弹窗并调用可选 on-done。不注册取消回调（无取消按钮）。
;; headless 模式下 delayed 回调不派发，退化为同步直跑。
;;
;; 语法
;; ----
;; (wait-dialog-run msg pause thunk [on-done])
;;
;; 参数
;; ----
;; msg - 待翻译的英文文案 key（translate 在本 GPL 层完成，调用方只传
;;       纯数据字符串）
;; pause - 弹窗首帧渲染的等待毫秒数
;; thunk - 实际任务（弹窗存活期间执行）
;; on-done - 可选 thunk，任务完成后调用

(tm-define (wait-dialog-run msg pause thunk . opt-on-done)
  (let ((on-done (if (null? opt-on-done) (lambda () #f) (car opt-on-done))))
    (if (headless?)
      (begin
        (thunk)
        (on-done)
      ) ;begin
      (begin
        (wait-dialog-open msg)
        (delayed (:pause pause) (thunk) (wait-dialog-close) (on-done))
      ) ;begin
    ) ;if
  ) ;let
) ;tm-define
