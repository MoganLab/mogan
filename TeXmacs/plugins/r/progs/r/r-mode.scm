;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MODULE      : r-mode.scm
;; DESCRIPTION : mode predicate for R programs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(texmacs-module (code r-mode)
  (:use (kernel texmacs tm-modes)))

(texmacs-modes
  (in-r% (== (get-env "prog-language") "r"))
  (in-prog-r% #t in-prog% in-r%))