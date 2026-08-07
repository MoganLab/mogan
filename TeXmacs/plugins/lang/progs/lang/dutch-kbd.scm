;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : dutch-kbd.scm
;; DESCRIPTION : keystrokes for the Dutch language
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (lang dutch-kbd) (:use (math math-kbd)))

(kbd-map (:mode in-math-dutch?)
  ;; ("e n" (make 'infix-and))
  ;; ("e n space" (make 'infix-and))
  ("space e" " e")
  ("space e var" (begin (kbd-space) (insert "<varepsilon>")))
  ("space e var var" (begin (kbd-space) (insert "<mathe>")))
  ("space e var var var" (begin (kbd-space) (insert "<epsilon>")))
  ("space e var var var var" (begin (kbd-space) (insert "<backepsilon>")))
  ("space e n" (make 'infix-and))
  ("space e n space" (make 'infix-and))
  ;; ("e n var" "en")
  ;; ("o f" (make 'infix-or))
  ;; ("o f space" (make 'infix-or))
  ("space o" " o")
  ("space o var" (begin (kbd-space) (insert "<omicron>")))
  ("space o f" (make 'infix-or))
  ("space o f space" (make 'infix-or))
  ;; ("o f var" "of")
  ;; ("d e s d a" (make 'infix-iff))
  ;; ("d e s d a space" (make 'infix-iff))
  ("space d" " d")
  ("space d var" (begin (kbd-space) (insert "<delta>")))
  ("space d var var" (begin (kbd-space) (insert "<mathd>")))
  ("space d var var var" (begin (kbd-space) (insert "<partial>")))
  ("space d e" " de")
  ("space d e s" " des")
  ("space d e s d" " desd")
  ("space d e s d a" (make 'infix-iff))
  ("space d e s d a space" (make 'infix-iff))
  ;; ("d e s d a var" "desda")
  ("v o o r space" "voor ")
  ("v o o r space a l l e" (make 'prefix-for-all))
  ("v o o r space a l l e space" (make 'prefix-for-all))
) ;kbd-map
