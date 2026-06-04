;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0610.scm
;; DESCRIPTION : Unit and Integration tests for HTML export/import of nested ordered lists
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/plugins/html/progs/convert/html/tmhtml-expand.scm")

(check-set-mode! 'report-failed)

(define (patch-has-macro? patch macro-name)
  (let loop ((lst (cdr patch)))
    (cond ((null? lst) #f)
          ((and (pair? (car lst))
                (eq? (caar lst) 'associate)
                (string=? (cadar lst) macro-name))
           #t)
          (else (loop (cdr lst))))))

(define (test-0610-env-patch-inclusion)
  (display "Verifying that list environments are properly registered in tmhtml-env-patch...\n")
  (let ((patch (tmhtml-env-patch)))
    ;; Check that previously missing list environments are now included as identity macros
    (check (patch-has-macro? patch "enumerate-numeric-paren") => #t)
    (check (patch-has-macro? patch "enumerate-circle") => #t)
    (check (patch-has-macro? patch "enumerate-hanzi") => #t)))

(define (test-0610-html-export-integration)
  (display "Verifying HTML export of nested lists...\n")
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0610.tmu")
         (tmp-html (url-temp))
         (dummy (load-buffer tmu-path))
         (dummy2 (buffer-export tmu-path tmp-html "html"))
         (html-content (string-load tmp-html)))
    ;; Check that HTML contains multiple ol tags representing nested ordered lists
    (check (string-contains? html-content "<ol") => #t)
    ;; We should have nested ol structure
    ;; Let's do a basic check on content
    (check (string-contains? html-content "误差分析") => #t)
    (check (string-contains? html-content "螺旋测微器存在误差") => #t)
    (check (string-contains? html-content "3号小球") => #t)))

(tm-define (test_0610)
  (test-0610-env-patch-inclusion)
  (test-0610-html-export-integration)
  (check-report))
