
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-gnuplot.scm
;; DESCRIPTION : Initialize Goldfish plugin
;; COPYRIGHT   : (C) 2024   Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-library (session gnuplot)
  (import (scheme base) (liii list))
  (export init-gnuplot)
  (begin
    (use-modules (binary gnuplot) (binary goldfish) (binary gs))

    (lazy-format (data gnuplot) gnuplot)

    (define (gnuplot-serialize lan t)
      (let* ((u (pre-serialize lan t)) (s (texmacs->code (stree->tree u) "utf-8")))
        (string-append s "\n<EOF>\n")
      ) ;let*
    ) ;define

    (define (gen-launcher image-format)
      (string-append (string-quote (url->system (find-binary-goldfish)))
        " "
        "load"
        " "
        (string-quote (string-append (url->system (get-texmacs-path))
                        "/plugins/gnuplot/goldfish/tm-gnuplot.scm"
                      ) ;string-append
        ) ;string-quote
        " "
        (string-quote (url->system (find-binary-gnuplot)))
        " "
        image-format
      ) ;string-append
    ) ;define

    (define (gnuplot-launchers)
      (let ((l (list (list :launch "pdf" (gen-launcher "pdf"))
                 (list :launch "svg" (gen-launcher "svg"))
                 (list :launch "png" (gen-launcher "png"))
               ) ;list
            ) ;l
           ) ;
        (if (has-binary-gs?)
          (append l (list (list :launch "eps" (gen-launcher "eps"))))
          l
        ) ;if
      ) ;let
    ) ;define

    (define (all-gnuplot-launchers)
      (cons (list :launch (gen-launcher "pdf")) (gnuplot-launchers))
    ) ;define

    (define (init-gnuplot)
      (plugin-configure gnuplot
        (:require (and (has-binary-goldfish?) (has-binary-gnuplot?)))
        ,(#_apply-values (all-gnuplot-launchers))
        (:serializer ,gnuplot-serialize)
        (:session "Gnuplot")
      ) ;plugin-configure
    ) ;define
  ) ;begin
) ;define-library

(import (session gnuplot))
(init-gnuplot)
