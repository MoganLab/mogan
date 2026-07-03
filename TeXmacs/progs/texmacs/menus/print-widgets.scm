
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : print-widgets.scm
;; DESCRIPTION : the print widgets
;; COPYRIGHT   : (C) 2013  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs menus print-widgets)
  (:use (kernel texmacs pref-keys) (texmacs texmacs tm-print))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Page setup
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (printing-command-list)
  (with l
    (cons (get-default-printing-command) (list "lpr" "lp" "pdq" ""))
    (list-remove-duplicates l)
  ) ;with
) ;define

;; 构造页面设置的字段表 tree（喂给 QML form 引擎）。label 已翻译、value 取自
;; 当前 preference、key 引用 pref-keys 函数（函数形式跨模块更可靠）。
;; 详见 record/qml/plan.md §6.2。

(define (page-setup-form-tree)
  `(form (enum ,(translate "Preview command:")
           ,(pref-page-setup-preview-command)
           ("default" "ggv" "ghostview" "gv" "kghostview" "open" "")
           ,(get-pretty-preference (pref-page-setup-preview-command)))
     (enum ,(translate "Printing command:")
       ,(pref-page-setup-printing-command)
       ,(printing-command-list)
       ,(get-pretty-preference (pref-page-setup-printing-command)))
     (enum ,(translate "Paper type:")
       ,(pref-page-setup-paper-type)
       ("default" "A3" "A4" "A5" "B4" "B5" "B6" "Letter" "Legal" "Executive" "")
       ,(get-pretty-preference (pref-page-setup-paper-type)))
     (enum ,(translate "Printer dpi:")
       ,(pref-page-setup-printer-dpi)
       ("150" "200" "300" "400" "600" "800" "1200" "2400" "")
       ,(get-pretty-preference (pref-page-setup-printer-dpi))))
) ;define

;; 弹出 QML form 弹窗；用户点 OK 时 cpp-form-dialog 返回 tree（含若干 (key
;; value) 子节点）。page-setup-form-tree 用 quasiquote 生成的是 scheme 列表
;; （stree），glue 入参需 mogan tree，故 stree->tree 转换。返回 tree 经
;; tree->stree 转回 scheme 列表后用 cadr/caddr 解构（mogan tree 非 pair，
;; 不可直接 car/cadr）。Cancel / 关闭返回空 tree，cdr 得 ()，for-each no-op。
(tm-define (open-page-setup-window)
  (:interactive #t)
  (with result
    (cpp-form-dialog (stree->tree (page-setup-form-tree)))
    (for-each (lambda (kv) (set-pretty-preference (cadr kv) (caddr kv)))
      (cdr (tree->stree result))
    ) ;for-each
  ) ;with
) ;tm-define

;; 首案统一走 QML 弹窗（旧 side-tool 依赖的 tm-widget 已移除）。
(tm-define (open-page-setup) (:interactive #t) (open-page-setup-window))
