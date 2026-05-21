;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tikz-lang.scm
;; DESCRIPTION : TikZ language support for syntax highlighting
;; COPYRIGHT   : (C) 2026  Jack Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code tikz-lang)
  (:use (prog default-lang)))

;;------------------------------------------------------------------------------
;; Keywords definition
;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "tikz") (== key "keyword")))
  `(,(string->symbol key)
    (constant
      "true" "false" "none" "solid" "dashed" "dotted" "thick" "thin" "ultra" "very" "semithick" "help" "lines")
    (declare_function
      "draw" "node" "path" "fill" "clip" "coordinate" "usetikzlibrary" "begin" "end" "filldraw" "shadedraw" "shade" "select" "foreach" "definecolor" "tikzset")
    (keyword
      "at" "cycle" "circle" "rectangle" "ellipse" "arc" "to" "grid" "step" "controls" "plot" "coordinates")))

;;------------------------------------------------------------------------------
;; Operators definition
;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "tikz") (== key "operator")))
  `(,(string->symbol key)
    (operator
      "+" "-" "*" "/" "\\" "^" "_" "=" "!" "?"
      ";" "," ":" "(" ")" "[" "]" "{" "}" "&" "|" "$"
      "--" "->" "<-" "<->")))

;;------------------------------------------------------------------------------
;; Comments definition
;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "tikz") (== key "comment")))
  `(,(string->symbol key)
    (inline "%")))

;;------------------------------------------------------------------------------
;; Preferences for syntax highlighting
;;

(define (notify-tikz-syntax var val)
  (syntax-read-preferences "tikz"))

(define-preferences
  ("syntax:tikz:none" "red" notify-tikz-syntax)
  ("syntax:tikz:comment" "brown" notify-tikz-syntax)
  ("syntax:tikz:error" "dark red" notify-tikz-syntax)
  ("syntax:tikz:constant" "#4040c0" notify-tikz-syntax)
  ("syntax:tikz:constant_number" "#3030b0" notify-tikz-syntax)
  ("syntax:tikz:constant_string" "dark grey" notify-tikz-syntax)
  ("syntax:tikz:constant_char" "#333333" notify-tikz-syntax)
  ("syntax:tikz:declare_function" "#0000c0" notify-tikz-syntax)
  ("syntax:tikz:declare_type" "#0000c0" notify-tikz-syntax)
  ("syntax:tikz:declare_module" "#0000c0" notify-tikz-syntax)
  ("syntax:tikz:operator" "#8b008b" notify-tikz-syntax)
  ("syntax:tikz:operator_openclose" "#B02020" notify-tikz-syntax)
  ("syntax:tikz:operator_field" "#888888" notify-tikz-syntax)
  ("syntax:tikz:operator_special" "orange" notify-tikz-syntax)
  ("syntax:tikz:keyword" "#309090" notify-tikz-syntax)
  ("syntax:tikz:keyword_conditional" "#309090" notify-tikz-syntax)
  ("syntax:tikz:keyword_control" "#008080ff" notify-tikz-syntax))
