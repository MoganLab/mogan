;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : r7rs-mode.scm
;; DESCRIPTION : R7RS Language mode
;; COPYRIGHT   : (C) 2024-2025  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (r7rs r7rs-mode) (:use (kernel texmacs tm-modes)))

(texmacs-modes (in-r7rs% (== (get-env "prog-language") "r7rs"))
  (in-prog-r7rs% #t in-prog% in-r7rs%)
) ;texmacs-modes
