;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6206.scm
;; DESCRIPTION : Test page-type style change dispatch
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/progs/generic/document-edit.scm")
(import (liii check))

(check-set-mode! 'report-failed)

(tm-define (test_6206)
  (new-document)
  (init-page-type "a5")
  (check (get-init "page-type") => "a5")
  (init-page-type "a4")
  (check (get-init "page-type") => "a4")
  (check-report)
) ;tm-define
