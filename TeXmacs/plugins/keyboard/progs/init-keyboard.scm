;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-keyboard.scm
;; DESCRIPTION : Initialize the keyboard plugin
;; COPYRIGHT   : (C) 2026  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (keyboard) (:use (texmacs texmacs tm-view)))

(lazy-keyboard (utils automate auto-kbd) in-auto?)
(lazy-keyboard (texmacs keyboard prefix-kbd) always?)
(lazy-keyboard (generic generic-kbd) always?)
(lazy-keyboard (generic search-kbd))
(lazy-keyboard (text text-kbd) in-text?)
(lazy-keyboard (keyboard text-kbd-utf8) in-text?)
(lazy-keyboard (source source-kbd) always?)
(lazy-keyboard (table table-kbd) in-table?)
(lazy-keyboard (graphics graphics-kbd) in-active-graphics?)
(lazy-keyboard (education edu-kbd) in-edu-text?)
(lazy-keyboard (dynamic fold-kbd) always?)
(lazy-keyboard (dynamic scripts-kbd) always?)
(lazy-keyboard (dynamic calc-kbd) always?)
(lazy-keyboard (doc tmdoc-kbd) in-manual?)
(lazy-keyboard (doc apidoc-kbd) developer-mode?)
(lazy-keyboard (link link-kbd) with-linking-tool?)
(lazy-keyboard (version version-kbd) with-versioning-tool?)

(lazy-keyboard-force #t)

;; 默认关闭；开启时才加载 (keyboard emoji) 注册 emoji kbd-map

(define (notify-emoji-keyboard var val)
  (when (== val "on")
    (when (not (defined? 'enable-emoji-keyboard))
      (use-modules (keyboard emoji))
    ) ;when
    (enable-emoji-keyboard)
  ) ;when
) ;define

(define-preferences ("emoji keyboard" "off" notify-emoji-keyboard))

(delayed (:idle 0)
  (kbd-map (:require (or (full-screen?) (full-screen-edit?)))
   ("escape" (exit-fullscreen) "Exit full screen")
   ("M-" (exit-fullscreen) "Exit full screen")
   ("A-" (exit-fullscreen) "Exit full screen")
  ) ;kbd-map
) ;delayed
