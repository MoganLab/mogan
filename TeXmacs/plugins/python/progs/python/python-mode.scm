;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : python-mode.scm
;; DESCRIPTION : Python Language mode
;; COPYRIGHT   : (C) 2025  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (python python-mode) (:use (kernel texmacs tm-modes)))

(texmacs-modes (in-python% (== (get-env "prog-language") "python"))
  (in-prog-python% #t in-prog% in-python%)
) ;texmacs-modes
