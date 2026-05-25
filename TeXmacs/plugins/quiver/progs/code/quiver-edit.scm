;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : quiver-edit.scm
;; DESCRIPTION : Editing Quiver code
;; COPYRIGHT   : (C) 2026 (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code quiver-edit)
  (:use (prog prog-edit)
    (code quiver-mode)))

(tm-define (get-tabstop)
  (:mode in-prog-quiver?)
  2)

(define (quiver-bracket-open lbr rbr)
  (bracket-open lbr rbr "\\"))

(define (quiver-bracket-close lbr rbr)
  (bracket-close lbr rbr "\\"))

(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-quiver?)
  (select-brackets-after-movement "([{" ")]}" "\\"))

(tm-define (kbd-paste)
  (:mode in-prog-quiver?)
  (clipboard-paste-import "quiver" "primary"))

(kbd-map
  (:mode in-prog-quiver?)
  ("A-tab" (insert-tabstop))
  ("cmd S-tab" (remove-tabstop))
  ("{" (quiver-bracket-open "{" "}"))
  ("}" (quiver-bracket-close "{" "}"))
  ("(" (quiver-bracket-open "(" ")"))
  (")" (quiver-bracket-close "(" ")"))
  ("[" (quiver-bracket-open "[" "]"))
  ("]" (quiver-bracket-close "[" "]"))
  ("\"" (quiver-bracket-open "\"" "\""))
  ("'" (quiver-bracket-open "'" "'")))
