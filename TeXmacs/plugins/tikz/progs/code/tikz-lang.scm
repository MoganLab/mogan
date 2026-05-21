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
    (extra_chars "_.-")
    (constant
      "true" "false" "none" "solid" "dashed" "dotted" "thick" "thin" "ultra" "very" "semithick"
      "help" "lines" "densely" "loosely" "double" "double distance" "smooth" "tension")
    (constant_identifier
      "red" "green" "blue" "cyan" "magenta" "yellow" "black" "white" "gray" "darkgray"
      "lightgray" "brown" "lime" "olive" "orange" "pink" "purple" "teal" "violet" "help lines")
    (constant_type
      "circle" "rectangle" "coordinate" "ellipse" "diamond" "trapezium" "semicircle"
      "regular polygon" "star" "isosceles triangle" "kite" "dart" "circular sector" "cylinder")
    (declare_function
      "draw" "node" "path" "fill" "clip" "filldraw" "shadedraw" "shade" "select" "foreach"
      "definecolor" "colorlet" "tikzset" "tikzstyle" "useasboundingbox" "matrix" "pic"
      "graph" "calendar" "scoped" "scope" "pgfextra" "pgfmathsetmacro" "pgfmathtruncatemacro")
    (declare_module
      "arrows" "shapes" "backgrounds" "calc" "positioning" "fit" "petri" "mindmap" "intersections"
      "tangent" "shapes.geometric" "shapes.symbols" "shapes.arrows" "shapes.multipart"
      "shapes.callouts" "shapes.misc" "svg.path")
    (variable_identifier
      "above" "below" "left" "right" "anchor" "above left" "above right" "below left" "below right"
      "mid" "base" "inner sep" "inner xsep" "inner ysep" "outer sep" "outer xsep" "outer ysep"
      "minimum height" "minimum width" "minimum size" "font" "node font" "text" "text width" "align"
      "line width" "opacity" "fill opacity" "draw opacity" "text opacity" "shading" "shading angle"
      "top color" "bottom color" "left color" "right color" "inner color" "outer color"
      "variable" "samples" "domain" "preaction" "postaction" "start angle" "end angle" "radius"
      "x radius" "y radius" "in" "out" "looseness" "bend" "step" "xstep" "ystep" "bend pos"
      "parabola height" "mark" "mark size" "mark options" "double" "double distance" "scale"
      "xscale" "yscale" "rotate" "rotate around" "shift" "xshift" "yshift" "xslant" "yslant" "transform shape")
    (keyword
      "at" "cycle" "circle" "rectangle" "ellipse" "arc" "to" "grid" "step" "controls" "plot"
      "coordinates" "parabola" "sin" "cos" "child" "edge" "svg")
    (keyword_control
      "begin" "end" "foreach" "usetikzlibrary")))

;;------------------------------------------------------------------------------
;; Operators definition
;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "tikz") (== key "operator")))
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
  ("syntax:tikz:constant_identifier" "#228b22" notify-tikz-syntax)
  ("syntax:tikz:constant_type" "#0000c0" notify-tikz-syntax)
  ("syntax:tikz:constant_number" "#3030b0" notify-tikz-syntax)
  ("syntax:tikz:constant_string" "dark grey" notify-tikz-syntax)
  ("syntax:tikz:constant_char" "#333333" notify-tikz-syntax)
  ("syntax:tikz:variable_identifier" "#a020f0" notify-tikz-syntax)
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
