;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tikz-lang.scm
;; DESCRIPTION : the TikZ Language
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;                   2026  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code tikz-lang) (:use (prog default-lang)))

(tm-define (parser-feature lan key)
  (:require (and (== lan "tikz") (== key "keyword")))
  `(,(string->symbol key)
    (keyword "draw"
      "fill"
      "path"
      "node"
      "coordinate"
      "tikzpicture"
      "end"
      "begin"
      "foreach"
      "def"
      "let"
      "if"
      "else"
      "fi"
      "scope"
      "endscope"
      "pgfdeclarelayer"
      "pgfsetlayers"
      "usetikzlibrary"
      "usepgflibrary"
      "definecolor"
      "colorlet"))
) ;tm-define

(tm-define (parser-feature lan key)
  (:require (and (== lan "tikz") (== key "string")))
  `(,(string->symbol key)
    (bool_features)
    (escape_sequences "\\" "\"" "'" "b" "f" "n" "r" "t"))
) ;tm-define
