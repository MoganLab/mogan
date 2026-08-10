;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : fish-mode.scm
;; DESCRIPTION : fish language mode
;; COPYRIGHT   : (C) 2025  vesita
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code fish-mode)
  (:use (kernel texmacs tm-modes))
) ;texmacs-module

(texmacs-modes
  (in-fish% (== (get-env "prog-language") "fish"))
  (in-prog-fish% #t in-prog% in-fish%)
) ;texmacs-modes
