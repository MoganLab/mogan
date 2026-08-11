
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : png.scm
;; DESCRIPTION : PNG Image plugin
;; COPYRIGHT   : (C) 2003  Joris van der Hoeven
;;                   2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (image png) (:use (binary inkscape) (binary convert)))

;; png -> postscript (the latter one which meets the requirements will work)
(converter png-file
  postscript-file
  (:require (has-binary-convert?))
  (:shell ,(url->system (find-binary-convert)) from to)
) ;converter

(converter png-file
  postscript-file
  (:require (has-binary-inkscape?))
  (:shell ,(url->system (find-binary-inkscape)) from "-o" to)
) ;converter

;; we do not need png -> pdf, because the hummus pdfwriter supports png image
