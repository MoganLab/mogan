
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : goldfish-edit.scm
;; DESCRIPTION : Editing goldfish programs
;; COPYRIGHT   : (C) 2024   Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code goldfish-edit) (:use (prog prog-edit)))

(texmacs-modes (in-goldfish% (== (get-env "prog-language") "goldfish"))
  (in-prog-goldfish% #t in-prog% in-goldfish%)
) ;texmacs-modes

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Automatic insertion, highlighting and selection of brackets and quotes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (goldfish-bracket-open lbr rbr) (bracket-open lbr rbr "\\"))

(tm-define (goldfish-bracket-close lbr rbr) (bracket-close lbr rbr "\\"))

(tm-define (kbd-select-enlarge)
  (:require prog-select-brackets?)
  (:mode in-prog-goldfish?)
  (program-select-enlarge "(" ")")
) ;tm-define

(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-goldfish?)
  (select-brackets-after-movement "([{" ")]}" "\\")
) ;tm-define

(kbd-map (:mode in-prog-goldfish?)
 ("{" (goldfish-bracket-open "{" "}"))
 ("}" (goldfish-bracket-close "{" "}"))
 ("(" (goldfish-bracket-open "(" ")"))
 (")" (goldfish-bracket-close "(" ")"))
 ("[" (goldfish-bracket-open "[" "]"))
 ("]" (goldfish-bracket-close "[" "]"))
 ("\"" (goldfish-bracket-open "\"" "\""))
) ;kbd-map
