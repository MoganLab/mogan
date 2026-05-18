
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : gif.scm
;; DESCRIPTION : GIF Image plugin
;; COPYRIGHT   : (C) 2003  Joris van der Hoeven
;;                   2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (image gif)
  (:use (binary convert)))

(converter gif-file postscript-document
  (:function image->psdoc))

(converter gif-file postscript-file
  (:require (has-binary-convert?))
  (:shell ,(url->system (find-binary-convert)) from to))
