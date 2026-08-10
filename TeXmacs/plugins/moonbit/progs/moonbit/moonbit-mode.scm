
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : moonbit-lang.scm
;; DESCRIPTION : Moonbit Language mode
;; COPYRIGHT   : (C) 2025  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code moonbit-mode)
  (:use (kernel texmacs tm-modes)))

(texmacs-modes
  (in-moonbit% (== (get-env "prog-language") "moonbit"))
  (in-prog-moonbit% #t in-prog% in-moonbit%))
