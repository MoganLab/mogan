;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : french-kbd.scm
;; DESCRIPTION : keystrokes for the French language
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (lang french-kbd) (:use (math math-kbd)))

(kbd-map (:mode in-math-french?)
  ;; ("e t" (make 'infix-and))
  ;; ("e t space" (make 'infix-and))
  ("space e" " e")
  ("space e var" (begin (kbd-space) (insert "<varepsilon>")))
  ("space e var var" (begin (kbd-space) (insert "<mathe>")))
  ("space e var var var" (begin (kbd-space) (insert "<epsilon>")))
  ("space e var var var var" (begin (kbd-space) (insert "<backepsilon>")))
  ("space e t" (make 'infix-and))
  ("space e t space" (make 'infix-and))
  ;; ("e t var" "et")
  ;; ("o u" (make 'infix-or))
  ;; ("o u space" (make 'infix-or))
  ("space o" " o")
  ("space o var" (begin (kbd-space) (insert "<omicron>")))
  ("space o u" (make 'infix-or))
  ("space o u space" (make 'infix-or))
  ;; ("o u var" "ou")
  ;; ("s s i" (make 'infix-iff))
  ;; ("s s i space" (make 'infix-iff))
  ("space s" " s")
  ("space s var" (begin (kbd-space) (insert "<sigma>")))
  ("space s var var" (begin (kbd-space) (insert "<varsigma>")))
  ("space s s" " ss")
  ("space s s i" (make 'infix-iff))
  ("space s s i space" (make 'infix-iff))
  ;; ("s s i var" "ssi")
  ("p o u r space" "pour ")
  ("p o u r space t o u t" (make 'prefix-for-all))
  ("p o u r space t o u t space" (make 'prefix-for-all))
) ;kbd-map
