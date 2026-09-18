;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : julia-mode.scm
;; DESCRIPTION : Julia Language mode
;; COPYRIGHT   : (C) 2025  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (julia julia-mode) (:use (kernel texmacs tm-modes)))

(texmacs-modes (in-julia% (== (get-env "prog-language") "julia"))
  (in-prog-julia% #t in-prog% in-julia%)
) ;texmacs-modes
