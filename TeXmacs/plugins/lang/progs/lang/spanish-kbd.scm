
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : spanish-kbd.scm
;; DESCRIPTION : keystrokes for the Spanish language
;; COPYRIGHT   : (C) 2026  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (lang spanish-kbd) (:use (text text-kbd)))

(kbd-map (:mode in-spanish?)
 ("! var" "<#A1>")
 ("? var" "<#BF>")
 ("! `" "<#A1>")
 ("? `" "<#BF>")
 ("! accent:grave" "<#A1>")
 ("? accent:grave" "<#BF>")
) ;kbd-map
