
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : r7rs-edit.scm
;; DESCRIPTION : Editing r7rs programs
;; COPYRIGHT   : (C) 2024   Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code r7rs-edit)
  (:use (prog prog-edit)))

(texmacs-modes
  (in-r7rs% (== (get-env "prog-language") "r7rs"))
  (in-prog-r7rs% #t in-prog% in-r7rs%))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Automatic insertion, highlighting and selection of brackets and quotes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (r7rs-bracket-open lbr rbr)
  (bracket-open lbr rbr "\\"))

(tm-define (r7rs-bracket-close lbr rbr)
  (bracket-close lbr rbr "\\"))

(tm-define (kbd-select-enlarge)
  (:require prog-select-brackets?)
  (:mode in-prog-r7rs?)
  (program-select-enlarge "(" ")"))

(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-r7rs?)
  (select-brackets-after-movement "([{" ")]}" "\\"))

(kbd-map
  (:mode in-prog-r7rs?)
  ("{" (r7rs-bracket-open "{" "}" ))
  ("}" (r7rs-bracket-close "{" "}" ))
  ("(" (r7rs-bracket-open "(" ")" ))
  (")" (r7rs-bracket-close "(" ")" ))
  ("[" (r7rs-bracket-open "[" "]" ))
  ("]" (r7rs-bracket-close "[" "]" ))
  ("\"" (r7rs-bracket-open "\"" "\"" )))
