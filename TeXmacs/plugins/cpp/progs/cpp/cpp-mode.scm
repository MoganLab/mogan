;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : cpp-mode.scm
;; DESCRIPTION : C++ Language mode
;; COPYRIGHT   : (C) 2025  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (cpp cpp-mode) (:use (kernel texmacs tm-modes)))

(texmacs-modes (in-cpp% (== (get-env "prog-language") "cpp"))
  (in-prog-cpp% #t in-prog% in-cpp%)
) ;texmacs-modes
