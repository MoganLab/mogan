;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : german-kbd.scm
;; DESCRIPTION : keystrokes for the German language
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (lang german-kbd) (:use (math math-kbd)))

(kbd-map (:mode in-math-german?)
  ;; ("u n d" (make 'infix-and))
  ;; ("u n d space" (make 'infix-and))
  ("space u" " u")
  ("space u var" (begin (kbd-space) (insert "<upsilon>")))
  ("space u n" " un")
  ("space u n d" (make 'infix-and))
  ("space u n d space" (make 'infix-and))
  ;; ("u n d var" "und")
  ;; ("o d e r" (make 'infix-or))
  ;; ("o d e r space" (make 'infix-or))
  ("space o" " o")
  ("space o var" (begin (kbd-space) (insert "<omicron>")))
  ("space o d" " od")
  ("space o d e" " ode")
  ("space o d e r" (make 'infix-or))
  ("space o d e r space" (make 'infix-or))
  ;; ("o d e r var" "oder")
  ;; ("g d w" (make 'infix-iff))
  ;; ("g d w space" (make 'infix-iff))
  ("space g" " g")
  ("space g var" (begin (kbd-space) (insert "<gamma>")))
  ("space g var var" (begin (kbd-space) (insert "<matheuler>")))
  ("space g d" " gd")
  ("space g d w" (make 'infix-iff))
  ("space g d w space" (make 'infix-iff))
  ;; ("g d w var" "gdw")
  ("f u r space" "fur ")
  ("f u r space a l l e" (make 'prefix-for-all))
  ("f u r space a l l e space" (make 'prefix-for-all))
  ("f u e r space" "fuer ")
  ("f u e r space a l l e" (make 'prefix-for-all))
  ("f u e r space a l l e space" (make 'prefix-for-all))
) ;kbd-map
