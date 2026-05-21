
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-tikz.scm
;; DESCRIPTION : Initialize TikZ plugin
;; COPYRIGHT   : (C) 2021 Darcy Shen
;;                   2026  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-library (session tikz)
  (import (scheme base) (liii list))
  (export init-tikz)
  (begin
    (use-modules (binary pdflatex) (binary goldfish))

    (define (tikz-serialize lan t)
      (let* ((u (pre-serialize lan t))
             (s (texmacs->code (stree->tree u) "SourceCode")))
        (string-append s "\n<EOF>\n")
      )
    )

    (define (tikz-launcher)
      (string-append
        (string-quote (url->system (find-binary-goldfish)))
        " "
        "load"
        " "
        (string-quote
          (string-append (url->system (get-texmacs-path))
            "/plugins/tikz/goldfish/tm-tikz.scm"
          )
        )
        " "
        (string-quote (url->system (find-binary-pdflatex)))
      )
    )

    (define (init-tikz)
      (plugin-configure tikz
        (:require (and (has-binary-goldfish?)
                       (has-binary-pdflatex?)))
        (:launch ,(tikz-launcher))
        (:serializer ,tikz-serialize)
        (:session "TikZ")
      )
    )
  )
)

(import (session tikz))
(init-tikz)
(lazy-format (data tikz) tikz)
(use-modules (code tikz-mode) (code tikz-edit))
