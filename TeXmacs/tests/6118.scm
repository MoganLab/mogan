;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6118.scm
;; DESCRIPTION : Test empty document node insertion defense and OCR text insertion
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/plugins/ocr/progs/ocr/ocr-insert.scm")
(import (liii check))

(define (test_6118)
  (check-set-mode! 'report-failed)

  ;; 1. OCR text insertion defense:
  ;; Empty, whitespace-only, or invalid input must return #f without calling
  ;; markdown insertion or kbd-return.
  (check (ocr-insert-text-by-cursor "") => #f)
  (check (ocr-insert-text-by-cursor "   ") => #f)
  (check (ocr-insert-text-by-cursor " \t\r\n ") => #f)
  (check (ocr-insert-text-by-cursor #f) => #f)
  (check (ocr-insert-text-by-cursor 123) => #f)

  ;; 2. C++ core defense:
  ;; Inserting empty document '(document) into empty buffer should not crash or corrupt tree.
  (insert '(document))
  (insert "hello 6118")
  (check (tree->stree (buffer-tree)) => '(document "hello 6118"))

  ;; Inserting empty document node again into non-empty buffer must be safely ignored.
  (insert '(document))
  (check (tree->stree (buffer-tree)) => '(document "hello 6118"))

  ;; Subsequent editing operations continue to work normally.
  (insert " world")
  (check (tree->stree (buffer-tree)) => '(document "hello 6118 world"))

  (check-report)
) ;define
