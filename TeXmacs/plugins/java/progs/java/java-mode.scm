;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : java-mode.scm
;; DESCRIPTION : Java Language mode
;; COPYRIGHT   : (C) 2025  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (java java-mode) (:use (kernel texmacs tm-modes)))

(texmacs-modes (in-java% (== (get-env "prog-language") "java"))
  (in-prog-java% #t in-prog% in-java%)
) ;texmacs-modes
