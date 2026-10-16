;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6121.scm
;; DESCRIPTION : Test BibTeX year field containing LaTeX macros sorting defense
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(check-set-mode! 'report-failed)
(load "./TeXmacs/plugins/latex/progs/latex/bibtex-bib-widgets.scm")

(define test-bib (url->string (url-append (url-temp-dir) "reproduce_6121.bib")))

(define (setup-test-file)
  (string-save (string-append "@inbook{knuth1973,\n"
                 "  author = {Donald E. Knuth},\n"
                 "  title = {Fundamental Algorithms},\n"
                 "  publisher = {Addison-Wesley},\n"
                 "  year = \"{\\noopsort{1973b}}1973\"\n" "}\n"
               ) ;string-append
    (string->url test-bib)
  ) ;string-save
) ;define

(define (cleanup-test-file)
  (if (url-exists? (string->url test-bib)) (system-remove (string->url test-bib)))
) ;define

(tm-define (test_6121)
  (setup-test-file)
  (let ((res (bib-to-tree test-bib "tm-plain")))
    (check (car res) => "valid")
    (check (tree? (caddr res)) => #t)
  ) ;let
  (let ((res-alpha (bib-to-tree test-bib "tm-alpha")))
    (check (car res-alpha) => "valid")
    (check (tree? (caddr res-alpha)) => #t)
  ) ;let
  (cleanup-test-file)
  (check-report)
) ;tm-define
