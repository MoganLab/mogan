
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : gnuplot-lang.scm
;; DESCRIPTION : the Gnuplot Language
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code gnuplot-lang) (:use (prog default-lang)))

(tm-define (parser-feature lan key)
  (:require (and (== lan "gnuplot") (== key "keyword")))
  `(,(string->symbol key) (keyword "set" "plot"))
) ;tm-define

(tm-define (parser-feature lan key)
  (:require (and (== lan "gnuplot") (== key "string")))
  `(,(string->symbol key)
    (bool_features)
    (escape_sequences "\\" "\"" "'" "b" "f" "n" "r" "t"))
) ;tm-define
