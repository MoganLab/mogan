
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : goldfish.scm
;; DESCRIPTION : goldfish Binary plugin
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary goldfish) (:use (binary common)))

(define (goldfish-binary-candidates)
  (cond ((os-windows?)
         (list (string-append (url->system (get-texmacs-path))
                 "/plugins/goldfish/bin/goldfish.exe"
               ) ;string-append
         ) ;list
        ) ;
        (else (list (string-append (url->system (get-texmacs-path))
                      "/plugins/goldfish/bin/goldfish"
                    ) ;string-append
              ) ;list
        ) ;else
  ) ;cond
) ;define

(tm-define (find-binary-goldfish)
  (:synopsis "Find the url to the goldfish binary, return (url-none) if not found"
  ) ;:synopsis
  (find-binary (goldfish-binary-candidates) "goldfish")
) ;tm-define

(tm-define (has-binary-goldfish?) (not (url-none? (find-binary-goldfish))))

(tm-define (version-binary-goldfish) (version-binary (find-binary-goldfish)))
