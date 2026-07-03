
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : pref-keys.scm
;; DESCRIPTION : preference key 常量的单一可信源。
;;               各弹窗（QML form 引擎，见 record/qml/plan.md）构造字段表、
;;               preference 读写、live setter 等使用方统一引用此处函数，
;;               消除 key 字符串散落各处的问题。
;;               注意：key 字面值必须与 tm-print.scm 等 define-preferences 注册
;;               处完全一致（key 改名会断开 notify 回调链路）。
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel texmacs pref-keys))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Page setup（文件 → 页面设置）
;; 权威定义：tm-print.scm 的 define-preferences（含 notify 回调）。
;; 用 define-public 函数而非值变量：函数跨模块绑定解析更可靠（mogan 惯例），
;; 且在调用方 quasiquote 里 ,(pref-...) 意图清晰。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (pref-page-setup-preview-command) "preview command")
(define-public (pref-page-setup-printing-command) "printing command")
(define-public (pref-page-setup-paper-type) "paper type")
(define-public (pref-page-setup-printer-dpi) "printer dpi")
