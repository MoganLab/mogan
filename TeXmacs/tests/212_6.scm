;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 212_6.scm
;; DESCRIPTION : Regression test for PDF export with invisible characters
;; COPYRIGHT   : (C) 2026 Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (generic generic-edit))

(import (liii check))

;; 该用例验证含零宽空格等不可见字符的文档（tmu/212_6.tmu）能够正常导出为
;; PDF 而不触发渲染异常。导出过程不抛错即视为通过。
(tm-define (test_212_6)
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/212_6.tmu")
         (tmu-url (string->url tmu-path))
         (pdf-url (url-temp)))
    (load-buffer tmu-url)
    (switch-to-buffer tmu-url)
    (print-to-file pdf-url)
    (check (url-exists? pdf-url) => #t)))
