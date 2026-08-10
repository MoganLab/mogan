;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MODULE      : bash-mode.scm
;; DESCRIPTION : mode predicate for bash scripts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(texmacs-module (bash bash-mode) (:use (kernel texmacs tm-modes)))

(texmacs-modes (in-bash% (== (get-env "prog-language") "bash"))
  (in-prog-bash% #t in-prog% in-bash%)
) ;texmacs-modes
