;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1308.scm
;; DESCRIPTION : 集成测试：参考文献预览（插入 -> 自动 -> 参考文献）渲染 .bib
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;   回归点：bibwid-output 里加载样式模块的 (eval '(use-modules ...)) 整体被
;;   quote，(latex bibtex-<style>) 从未加载，bib-format-entry 未绑定，选择
;;   合法 .bib 后预览报 unbound variable。本测试钉死：预览正常产出 bib-list
;;   且两个条目都被格式化。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 1308
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(check-set-mode! 'report-failed)
(load "./TeXmacs/plugins/latex/progs/latex/bibtex-bib-widgets.scm")

(define (test-bibwid-output-renders-bib-list)
  ;; 预览路径不排版文档，须自行保证样式模块已加载（即本次修复点）
  (set! bibwid-use-relative? #f)
  (set! bibwid-style "tm-plain")
  (set! bibwid-url
    (string->url (string-append (getenv "TEXMACS_PATH") "/tests/bib/1308.bib")))
  (with out
    (tree->stree (bibwid-output))
    (check-true (tm-func? out 'with))
    ;; 渲染成功时 bib-process 产出 bib-list；文件非法时则是提示文案、无 bib-list
    (with bibs
      (select out '(:* bib-list))
      (check-true (nnull? bibs))
      ;; 1308.bib 两个条目均经 bib-format-entry 格式化
      (check (cadr (car bibs)) => "2")
      (check (length (cdr (caddr (car bibs)))) => 2)
    ) ;with
  ) ;with
) ;define

(tm-define (test_1308)
  (test-bibwid-output-renders-bib-list)
  (check-report)
) ;tm-define
