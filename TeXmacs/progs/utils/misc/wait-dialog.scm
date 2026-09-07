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
;; 打开等待弹窗并注册取消回调
;;
;; 语法
;; ----
;; (wait-dialog-open msg on-cancel)
;;
;; 参数
;; ----
;; msg - 待翻译的英文文案 key（translate 在本 GPL 层完成，调用方/goldfish
;;       编排层只传纯数据字符串）
;; on-cancel - thunk，用户取消（ESC/Cancel 按钮）时被调用

(tm-define (wait-dialog-open msg on-cancel)
  (set! current-cancel-thunk on-cancel)
  (cpp-wait-dialog-open (translate msg))
) ;tm-define

;; 关闭等待弹窗（程序性关闭：不触发取消回流；幂等，未打开时 no-op）。

(tm-define (wait-dialog-close)
  (cpp-wait-dialog-close)
) ;tm-define

;; C++ 取消回流入口：用户 ESC/点取消时，Qt 侧 WaitDialogBridge 经
;; eval_scheme 按名调用本函数（模式同 paragraph-format-commit），转发到
;; 当前注册的 on-cancel；回调属主是编排层（如 goldfish (liii ocr-impl) 的
;; per-task 闭包），置位后由各异步回调入口守卫拦截后续动作。

(tm-define (wait-dialog-cancelled)
  (current-cancel-thunk)
) ;tm-define
