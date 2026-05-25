;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-quiver.scm
;; DESCRIPTION : Initialize Quiver plugin
;; COPYRIGHT   : (C) 2026 (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-library (session quiver)
  (import (scheme base) (liii list))
  (export init-quiver)
  (begin
    (use-modules (binary pdflatex) (binary goldfish))

    (define (quiver-serialize lan t)
      (let* ((u (pre-serialize lan t))
             (s (texmacs->code (stree->tree u) "SourceCode")))
        (string-append s "\n<EOF>\n")
      )
    )

    (define (quiver-launcher)
      (string-append
        (string-quote (url->system (find-binary-goldfish)))
        " "
        "load"
        " "
        (string-quote
          (string-append (url->system (get-texmacs-path))
            "/plugins/quiver/goldfish/tm-quiver.scm"
          )
        )
        " "
        (string-quote (url->system (find-binary-pdflatex)))
      )
    )

    (define (init-quiver)
      (plugin-configure quiver
        (:require (and (has-binary-goldfish?)
                       (has-binary-pdflatex?)))
        (:launch ,(quiver-launcher))
        (:serializer ,quiver-serialize)
        (:session "Quiver")
      )
    )
  )
)

(import (session quiver))
(init-quiver)
