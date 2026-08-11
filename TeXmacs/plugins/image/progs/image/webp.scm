
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : webp.scm
;; DESCRIPTION : WebP Image plugin
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (image webp) (:use (binary inkscape) (binary convert)))

;; webp -> postscript (the latter one which meets the requirements will work)
(converter webp-file
  postscript-file
  (:require (has-binary-convert?))
  (:shell ,(url->system (find-binary-convert)) from to)
) ;converter

(converter webp-file
  postscript-file
  (:require (has-binary-inkscape?))
  (:shell ,(url->system (find-binary-inkscape)) from "-o" to)
) ;converter
