
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : esperanto-kbd.scm
;; DESCRIPTION : keystrokes for the Esperanto language
;; COPYRIGHT   : (C) 2017  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (lang esperanto-kbd) (:use (text text-kbd)))

(kbd-map (:mode in-esperanto?)
 ("c var" "<#109>")
 ("g var" "<#11D>")
 ("h var" "<#125>")
 ("j var" "<#135>")
 ("s var" "<#15D>")
 ("u var" "<#16D>")
 ("C var" "<#108>")
 ("G var" "<#11C>")
 ("H var" "<#124>")
 ("J var" "<#134>")
 ("S var" "<#15C>")
 ("U var" "<#16C>")
) ;kbd-map
