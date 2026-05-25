;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : quiver-lang.scm
;; DESCRIPTION : Quiver language support for syntax highlighting
;; COPYRIGHT   : (C) 2026 (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code quiver-lang)
  (:use (prog default-lang)))

;;------------------------------------------------------------------------------
;; Keywords definition
;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "quiver") (== key "keyword")))
  `(,(string->symbol key)
    (extra_chars "_.-")
    (constant
      "true" "false" "none" "solid" "dashed" "dotted" "thick" "thin" "ultra" "very" "semithick"
      "smooth" "tension")
    (constant_identifier
      "red" "green" "blue" "cyan" "magenta" "yellow" "black" "white" "gray" "darkgray"
      "lightgray" "brown" "lime" "olive" "orange" "pink" "purple" "teal" "violet")
    (constant_type
      "circle" "rectangle" "coordinate" "ellipse" "diamond" "trapezium" "semicircle"
      "regular polygon" "star" "cylinder")
    (declare_function
      "draw" "node" "path" "fill" "clip" "filldraw" "shadedraw" "shade" "select" "foreach"
      "definecolor" "colorlet" "tikzset" "tikzstyle" "arrow" "ar")
    (declare_module
      "arrows" "shapes" "calc" "positioning" "fit" "intersections" "shapes.geometric" "svg.path")
    (variable_identifier
      "above" "below" "left" "right" "anchor" "above left" "above right" "below left" "below right"
      "mid" "base" "inner sep" "outer sep" "minimum height" "minimum width" "minimum size"
      "font" "node font" "text" "text width" "align" "line width" "opacity" "fill opacity"
      "draw opacity" "text opacity" "scale" "rotate" "shift" "xshift" "yshift")
    (keyword
      "at" "cycle" "circle" "rectangle" "ellipse" "arc" "to" "grid" "step" "controls" "plot"
      "coordinates" "parabola" "sin" "cos" "child" "edge")
    (keyword_control
      "begin" "end" "foreach" "usetikzlibrary" "tikzcd")))

;;------------------------------------------------------------------------------
;; Operators definition
;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "quiver") (== key "operator")))
  `(,(string->symbol key)
    (operator
      "+" "-" "*" "/" "\\" "^" "_" "=" "!" "?"
      ";" "," ":" "&" "|" "$")
    (operator_openclose
      "(" ")" "[" "]" "{" "}")
    (operator_special
      "--" "->" "<-" "<->" "++" "+")))

;;------------------------------------------------------------------------------
;; Comments definition
;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "quiver") (== key "comment")))
  `(,(string->symbol key)
    (inline "%")))

;;------------------------------------------------------------------------------
;; Preferences for syntax highlighting
;;

(define (notify-quiver-syntax var val)
  (syntax-read-preferences "quiver"))

(define-preferences
  ("syntax:quiver:none" "red" notify-quiver-syntax)
  ("syntax:quiver:comment" "brown" notify-quiver-syntax)
  ("syntax:quiver:error" "dark red" notify-quiver-syntax)
  ("syntax:quiver:constant" "#4040c0" notify-quiver-syntax)
  ("syntax:quiver:constant_identifier" "#228b22" notify-quiver-syntax)
  ("syntax:quiver:constant_type" "#0000c0" notify-quiver-syntax)
  ("syntax:quiver:constant_number" "#3030b0" notify-quiver-syntax)
  ("syntax:quiver:constant_string" "dark grey" notify-quiver-syntax)
  ("syntax:quiver:constant_char" "#333333" notify-quiver-syntax)
  ("syntax:quiver:variable_identifier" "#a020f0" notify-quiver-syntax)
  ("syntax:quiver:declare_function" "#0000c0" notify-quiver-syntax)
  ("syntax:quiver:declare_type" "#0000c0" notify-quiver-syntax)
  ("syntax:quiver:declare_module" "#0000c0" notify-quiver-syntax)
  ("syntax:quiver:declare_class" "#0000c0" notify-quiver-syntax)
  ("syntax:quiver:declare_variable" "#0000c0" notify-quiver-syntax)
  ("syntax:quiver:keyword" "#0000c0" notify-quiver-syntax)
  ("syntax:quiver:keyword_control" "#0000c0" notify-quiver-syntax)
  ("syntax:quiver:operator" "#0000c0" notify-quiver-syntax)
  ("syntax:quiver:operator_openclose" "#000000" notify-quiver-syntax)
  ("syntax:quiver:operator_special" "#d2691e" notify-quiver-syntax)
  ("syntax:quiver:macro" "#c71585" notify-quiver-syntax)
  ("syntax:quiver:matched_bracket" "#ffffc0" notify-quiver-syntax))
