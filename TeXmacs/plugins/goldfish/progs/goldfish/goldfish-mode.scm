;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : goldfish-mode.scm
;; DESCRIPTION : Goldfish Language mode
;; COPYRIGHT   : (C) 2024-2025  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (goldfish goldfish-mode) (:use (kernel texmacs tm-modes)))

(texmacs-modes (in-goldfish% (== (get-env "prog-language") "goldfish"))
  (in-prog-goldfish% #t in-prog% in-goldfish%)
) ;texmacs-modes
