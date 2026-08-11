
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : postscript.scm
;; DESCRIPTION : Postscript Image plugin
;; COPYRIGHT   : (C) 2003  Joris van der Hoeven
;;                   2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (image postscript) (:use (binary convert) (binary gs)))

(converter postscript-file
  pdf-file
  (:require (has-binary-gs?))
  (:function-with-options gs-eps-to-pdf)
) ;converter

(converter postscript-file
  png-file
  (:require (has-binary-convert?))
  (:shell ,(url->system (find-binary-convert)) from to)
) ;converter

(converter postscript-file
  png-file
  (:require (has-binary-gs?))
  (:function-with-options gs-eps-to-png)
) ;converter
