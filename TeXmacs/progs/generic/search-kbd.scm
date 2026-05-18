;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : search-kbd.scm
;; DESCRIPTION : search-widget kbd
;; COPYRIGHT   : (C) Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic search-kbd) (:use (generic search-widgets)))

(kbd-map (:require (inside-search-or-replace-buffer?))
 ("std ?" (make 'select-region))
 ("std 1" (insert '(wildcard "x")))
 ("std 2" (insert '(wildcard "y")))
 ("std 3" (insert '(wildcard "z")))
 ;; 导航快捷键
 ("pageup" (search-next-match #f))
 ("pagedown" (search-next-match #t))
 ("home" (search-extreme-match #f))
 ("end" (search-extreme-match #t))
 ;; 功能键支持
 ("F3" (search-next-match #t))
 ("S-F3" (search-next-match #f))
 ;; 特殊组合键
 ("std return" (search-rotate-match #f))
 ("return" (search-rotate-match #t))
 ("up" (search-replace-up-down #f))
 ("down" (search-replace-up-down #t))
) ;kbd-map

;; 替换缓冲区专有快捷键
(kbd-map (:require (inside-replace-buffer?))
 ("return"
   (replace-one (window->buffer (auxiliary-buffer->window (replace-buffer)))
     (replace-buffer)
   ) ;replace-one
 ) ;
 ("std return"
   (replace-all (window->buffer (auxiliary-buffer->window (replace-buffer)))
     (replace-buffer)
   ) ;replace-all
 ) ;
) ;kbd-map
