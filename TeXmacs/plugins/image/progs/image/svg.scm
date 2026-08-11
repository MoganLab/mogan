
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : svg.scm
;; DESCRIPTION : SVG Image plugin
;; COPYRIGHT   : (C) 2003  Joris van der Hoeven
;;                   2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (image svg)
  (:use (binary rsvg-convert) (binary inkscape) (binary convert))
) ;texmacs-module

(converter svg-file
  png-file
  (:require (has-binary-inkscape?))
  (:shell ,(url->system (find-binary-inkscape))
    "-z"
    "-d"
    "600"
    from
    "--export-png"
    to
  ) ;:shell
) ;converter

(converter svg-file
  png-file
  (:require (has-binary-rsvg-convert?))
  (:function-with-options svg2png-by-rsvg-convert)
) ;converter

;; svg -> pdf (the latter one which meets the requirements will work)
(converter svg-file
  pdf-file
  (:require (has-binary-rsvg-convert?))
  (:shell ,(url->system (find-binary-rsvg-convert)) "-f pdf" "-o" to from)
) ;converter

(converter svg-file
  pdf-file
  (:require (has-binary-inkscape?))
  (:shell ,(url->system (find-binary-inkscape)) from "-o" to)
) ;converter

;; svg -> postscript (the latter one which meets the requirements will work)
(converter svg-file
  postscript-file
  (:require (has-binary-rsvg-convert?))
  (:shell ,(url->system (find-binary-rsvg-convert)) "-f eps" "-o" to from)
) ;converter

(converter svg-file
  postscript-file
  (:require (has-binary-inkscape?))
  (:shell ,(url->system (find-binary-inkscape)) from "-o" to)
) ;converter
