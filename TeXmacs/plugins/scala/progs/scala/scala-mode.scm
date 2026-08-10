
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : scala-lang.scm
;; DESCRIPTION : Scala Language mode
;; COPYRIGHT   : (C) 2017-2020  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code scala-mode)
  (:use (kernel texmacs tm-modes)))

(texmacs-modes
  (in-scala% (== (get-env "prog-language") "scala"))
  (in-prog-scala% #t in-prog% in-scala%))
