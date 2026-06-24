;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0606.scm
;; DESCRIPTION : Tests for \to command LaTeX export
;; COPYRIGHT   : (C) 2026  Jack Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")

;; 未安装 latex（pdflatex 不在 PATH）时跳过：导出测试依赖本地 TeX 环境，
;; CI 等无 latex 的环境无法运行。
(define (latex-present?)
  (!= (url-resolve-in-path "pdflatex") (url-none)))

(define (export-as-latex-and-load path)
  (with path (string-append "$TEXMACS_PATH/tests/tmu/" path)
    (with tmpfile (url-temp)
      (load-buffer path)
      (buffer-export path tmpfile "latex")
      (string-load tmpfile))))
  
(define (load-latex path)
  (with path (string-append "$TEXMACS_PATH/tests/tex/" path)
    (string-replace (string-load path)  "\r\n" "\n")))


(define (test_0606)
  (when (latex-present?)
    (check (export-as-latex-and-load "0606.tmu") => (load-latex "0606_to_export.tex")))
  (check-report))
