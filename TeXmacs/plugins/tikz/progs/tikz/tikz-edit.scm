;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tikz-edit.scm
;; DESCRIPTION : Editing TikZ code
;; COPYRIGHT   : (C) 2026  Jack Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code tikz-edit)
  (:use (prog prog-edit)
    (code tikz-mode)))

(tm-define (get-tabstop)
  (:mode in-prog-tikz?)
  2)

(tm-define (tikz-bracket-open lbr rbr)
  (bracket-open lbr rbr "\\"))

(tm-define (tikz-bracket-close lbr rbr)
  (bracket-close lbr rbr "\\"))

(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-tikz?)
  (select-brackets-after-movement "([{" ")]}" "\\"))

(tm-define (kbd-paste)
  (:mode in-prog-tikz?)
  (clipboard-paste-import "tikz" "primary"))

(kbd-map
  (:mode in-prog-tikz?)
  ("A-tab" (insert-tabstop))
  ("cmd S-tab" (remove-tabstop))
  ("{" (tikz-bracket-open "{" "}"))
  ("}" (tikz-bracket-close "{" "}"))
  ("(" (tikz-bracket-open "(" ")"))
  (")" (tikz-bracket-close "(" ")"))
  ("[" (tikz-bracket-open "[" "]"))
  ("]" (tikz-bracket-close "[" "]"))
  ("\"" (tikz-bracket-open "\"" "\""))
  ("'" (tikz-bracket-open "'" "'")))
