;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6111.scm
;; DESCRIPTION : Test Paste Special choices and format conversion
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/progs/generic/paste-widget.scm")
(import (liii check))

(define (test_6111)
  (check-set-mode! 'report-failed)

  ;; 1. Check format string and symbol conversion
  (check (convert-format-string-to-symbol "Markdown") => "md")
  (check (convert-format-string-to-symbol "LaTeX") => "latex")
  (check (convert-format-string-to-symbol "HTML") => "html")
  (check (convert-symbol-to-format-string "md") => "Markdown")
  (check (convert-symbol-to-format-string "latex") => "LaTeX")
  (check (convert-symbol-to-format-string "html") => "HTML")

  ;; 2. Check init-choices moves detected clipboard format to the top
  (let ((plain-list (list "Markdown" "LaTeX" "HTML" (translate "Plain text"))))
    (check (init-choices plain-list "latex")
           => (list "LaTeX" "Markdown" "HTML" (translate "Plain text")))
    (check (init-choices plain-list "html")
           => (list "HTML" "Markdown" "LaTeX" (translate "Plain text")))
    (check (car (init-choices plain-list "md"))
           => "Markdown")
  ) ;let

  (check-report)
) ;define
