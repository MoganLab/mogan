
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-goldfish.scm
;; DESCRIPTION : Initialize Goldfish plugin
;; COPYRIGHT   : (C) 2024   Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (binary goldfish))

(lazy-format (data r7rs) r7rs)

(define (goldfish-serialize lan t)
  (let* ((u (pre-serialize lan t)) (s (texmacs->code (stree->tree u) "SourceCode")))
    (string-append s "\n<EOF>\n")
  ) ;let*
) ;define

(define (goldfish-launch mode)
  (string-append (string-quote (url->system (find-binary-goldfish)))
    " "
    (string-quote (string-append (url->system (get-texmacs-path))
                    "/plugins/goldfish/goldfish/tm-goldfish.scm"
                  ) ;string-append
    ) ;string-quote
    " -m "
    mode
  ) ;string-append
) ;define

(plugin-configure goldfish
  (:require (has-binary-goldfish?))
  (:launch ,(goldfish-launch "default"))
  (:launch "sicp" ,(goldfish-launch "sicp"))
  (:launch "r7rs" ,(goldfish-launch "r7rs"))
  (:serializer ,goldfish-serialize)
  (:session "Goldfish Scheme")
) ;plugin-configure
