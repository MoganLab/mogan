
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : generic-kbd.scm
;; DESCRIPTION : general keyboard shortcuts for all modes
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic generic-kbd)
  (:use (texmacs keyboard prefix-kbd)
    (utils edit variants)
    (utils edit auto-close)
    (utils edit selections)
    (utils library cursor)
    (generic document-edit)
    (generic generic-edit)
    (generic ghost-text)
    (generic diff-text)
    (generic format-edit)
    (generic format-geometry-edit)
    (source source-edit)
    (texmacs texmacs tm-files)
    (texmacs texmacs tm-print)
    (doc help-funcs)
  ) ;:use
) ;texmacs-module
(debug-message "keyboard" "(generic generic-kbd): registering kbd-map ...\n")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; General shortcuts for all modes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map ("F1" (interactive docgrep-in-doc))
 ("S-F1" (interactive docgrep-in-src))
 ;; FIXME: S-F1 should be 'What is This?'

 ("<" "<less>")
 (">" "<gtr>")
 ("(" (make-bracket-open "(" ")"))
 (")" (make-bracket-close ")" "("))
 ("[" (make-bracket-open "[" "]"))
 ("]" (make-bracket-close "]" "["))
 ("{" (make-bracket-open "{" "}"))
 ("}" (make-bracket-close "}" "{"))
 ("\\" (if (or (inside? 'hybrid) (in-prog?)) (insert "\\") (make-hybrid)))
 ("\\ var" "\\")
 ("\\ var var" "<setminus>")
 ("$" (make 'math))
 ("$ var" "$")

 ("-" "-")
 ("space" (kbd-space))
 ("tab" (kbd-tab))
 ("enter" (kbd-return))
 ("return" (kbd-return))
 ("S-space" (kbd-shift-space))
 ("S-tab" (kbd-shift-tab))
 ("S-return" (kbd-shift-return))
 ("C-return" (kbd-control-return))
 ("C-S-return" (kbd-shift-control-return))
 ("A-return" (kbd-alternate-return))
 ("A-S-return" (kbd-shift-alternate-return))

 ("delete" (kbd-delete))
 ("backspace" (kbd-backspace))
 ("left" (kbd-left))
 ("right" (kbd-right))
 ("up" (kbd-up))
 ("down" (kbd-down))
 ("home" (kbd-start-line))
 ("end" (kbd-end-line))
 ("pageup" (kbd-page-up))
 ("pagedown" (kbd-page-down))
 ("S-delete" (kbd-delete))
 ("S-backspace" (kbd-backspace))
 ("S-left" (kbd-select kbd-left))
 ("S-right" (kbd-select kbd-right))
 ("S-up" (kbd-select kbd-up))
 ("S-down" (kbd-select kbd-down))
 ("S-home" (kbd-select kbd-start-line))
 ("S-end" (kbd-select kbd-end-line))
 ("S-pageup" (kbd-select kbd-page-up))
 ("S-pagedown" (kbd-select kbd-page-down))

 ("structured:cmd delete" (remove-structure-upwards))
 ("structured:cmd backspace" (remove-structure-upwards))
 ("structured:cmd up" (kbd-select-if-active traverse-up))
 ("structured:cmd down" (kbd-select-if-active traverse-down))
 ("structured:cmd pageup" (traverse-previous))
 ("structured:cmd pagedown" (traverse-next))
 ("structured:cmd section" (traverse-previous-section-title))
 ("structured:cmd S-left" (kbd-select traverse-left))
 ("structured:cmd S-right" (kbd-select traverse-right))
 ("structured:cmd S-up" (kbd-select traverse-up))
 ("structured:cmd S-down" (kbd-select traverse-down))
 ("structured:cmd S-pageup" (kbd-select traverse-previous))
 ("structured:cmd S-pagedown" (kbd-select traverse-next))
 ("structured:cmd space" (kbd-select-enlarge))
 ("structured:cmd tab" (variant-circulate (focus-tree) #t))
 ("structured:cmd S-tab" (variant-circulate (focus-tree) #f))
 ("structured:cmd *" (alternate-toggle (focus-tree)))
 ("structured:cmd #" (numbered-toggle (focus-tree)))
 ("A-S-down" (variant-circulate (focus-tree) #t))
 ("A-S-up" (variant-circulate (focus-tree) #f))

 ("structured:move delete" (structured-exit-right))
 ("structured:move backspace" (structured-exit-left))
 ("structured:move left" (structured-left))
 ("structured:move right" (structured-right))
 ("structured:move up" (structured-up))
 ("structured:move down" (structured-down))
 ("structured:move home" (structured-start))
 ("structured:move end" (structured-end))
 ("structured:move pageup" (structured-top))
 ("structured:move pagedown" (structured-bottom))
 ("structured:move S-left" (kbd-select structured-left))
 ("structured:move S-right" (kbd-select structured-right))
 ("structured:move S-up" (kbd-select structured-up))
 ("structured:move S-down" (kbd-select structured-down))
 ("structured:move S-home" (kbd-select structured-start))
 ("structured:move S-end" (kbd-select structured-end))
 ("structured:move S-pageup" (kbd-select structured-top))
 ("structured:move S-pagedown" (kbd-select structured-bottom))

 ("structured:insert delete" (structured-remove-right))
 ("structured:insert backspace" (structured-remove-left))
 ("structured:insert left" (structured-insert-left))
 ("structured:insert right" (structured-insert-right))
 ("structured:insert up" (structured-insert-up))
 ("structured:insert down" (structured-insert-down))
 ("structured:insert home" (structured-insert-start))
 ("structured:insert end" (structured-insert-end))
 ("structured:insert pageup" (structured-insert-top))
 ("structured:insert pagedown" (structured-insert-bottom))

 ("structured:geometry delete" (geometry-reset))
 ("structured:geometry backspace" (geometry-reset))
 ("structured:geometry left" (geometry-left))
 ("structured:geometry right" (geometry-right))
 ("structured:geometry up" (geometry-up))
 ("structured:geometry down" (geometry-down))
 ("structured:geometry home" (geometry-start))
 ("structured:geometry end" (geometry-end))
 ("structured:geometry pageup" (geometry-top))
 ("structured:geometry pagedown" (geometry-bottom))
 ("structured:geometry tab" (geometry-circulate #t))
 ("structured:geometry S-tab" (geometry-circulate #f))
 ("structured:geometry [" (geometry-slower))
 ("structured:geometry ]" (geometry-faster))

 ("special left" (special-left))
 ("special right" (special-right))
 ("special up" (special-up))
 ("special down" (special-down))
 ("special home" (special-first))
 ("special end" (special-last))
 ("special pageup" (special-previous))
 ("special pagedown" (special-next))
 ("special return" (special-return))
 ("special S-return" (special-shift-return))
 ("special [" (special-back))
 ("special ]" (special-forward))

 ("altcmd \\" (make-hybrid))
 ("altcmd a" (make-tree))
 ("altcmd R" (make-rigid))
 ("altcmd =" (make 'hgroup))
 ("altcmd |" (make 'vgroup))
 ("altcmd :" (make 'line-break))
 ("altcmd ;" (make 'new-line))
 ("altcmd return" (make 'next-line))
 ("altcmd /" (make 'no-break))

 ("C-!" (make-label))
 ("C-?" (make 'reference))
 ("C-? var" (make 'eqref))
 ("C-? var var" (make 'pageref))

 ("extra e" (edit-focus-macro))
 ("extra r" (edit-previous-macro))
 ("extra m" (edit-focus-macro-source))
 ;; ("extra p" (toggle-preamble-mode))
 ;; ("extra s" (toggle-source-mode))

 ("accent:hat" "^")
 ("accent:deadhat" "^")
 ("accent:tilde" "~")
 ("accent:acute" "'")
 ("accent:grave" "`")

 ("symbol \\" "\\")
 ("symbol \"" "\"")
 ("symbol $" "$")
 ("symbol &" "&")
 ("symbol #" "#")
 ("symbol �" "�")
 ("symbol %" "%")
 ("symbol _" "_")
 ("symbol ^" "^")
 ("symbol (" "(")
 ("symbol )" ")")
 ("symbol [" "[")
 ("symbol ]" "]")
 ("symbol {" "{")
 ("symbol }" "}")
 ("symbol |" "|")

 ("undo" (noop) (undo 0))
 ("redo" (noop) (redo 0))
 ("cancel" (noop) (kbd-cancel))
 ("cut" (noop) (kbd-cut))
 ("paste" (noop) (kbd-paste))
 ("copy" (noop) (kbd-copy))
 ("find" (noop) (interactive-search))
 ("search find" (search-next-match #t))
 ("search again" (search-next-match #t))

 ("cmd t" (make 'tabular))
 ("cmd t var" (make 'tabular*))
 ("cmd t var var" (make 'wide-tabular))
 ("cmd t var var var" (make 'block))
 ("cmd t var var var var" (make 'block*))
 ("cmd t var var var var var" (make 'wide-block))
 ("std j" (toggle-chat-sidebar))
) ;kbd-map

(kbd-map (:require (list-structured-insert-context?))
 ("structured:insert delete" (structured-remove-down))
 ("structured:insert backspace" (structured-remove-up))
) ;kbd-map

(kbd-map (:mode in-hybrid?)
 ("space" (hybrid-kbd-space))
 ("{" (hybrid-kbd-curly-left))
 ("}" (hybrid-kbd-curly-right))
 ("\\" (hybrid-kbd-backslash))
 ("_" (hybrid-kbd-sub))
 ("_ var" "_")
 ("^" (hybrid-kbd-sup))
 ("^ var" "^")
) ;kbd-map

(kbd-map (:mode in-smart-ref?)
 ("std ?" (make 'smart-ref))
 ("std ? var" (make 'reference))
 ("std ? var var" (make 'pageref))
) ;kbd-map


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Standard cross-platform keybindings
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map (:profile std)

  ;; standard cross-platform shortcuts
  ("std a" (select-all))
  ("std b" (toggle-bold))
  ("std c" (kbd-copy))
  ("std f" (interactive-search))
  ("std i" (toggle-italic))
  ("std t" (new-document))
  ("std n" (new-document))
  ("std N" (new-document*))
  ("std o" (open-document))
  ("std L" (clipboard-copy-export "latex" "primary"))
  ("std p" (preview-buffer))
  ("std q" (safely-quit-TeXmacs))
  ("std R" (update-document "all"))
  ("std s" (save-buffer))
  ("std S" (choose-file save-buffer-as "Save TeXmacs file" "action_save_as"))
  ("std u" (toggle-underlined))
  ("std A-v" (interactive-paste-special))
  ("std w" (close-document))
  ("std W" (close-document*))
  ("std x" (kbd-cut))
  ("std z" (undo 0))
  ("std Z" (redo 0))
  ("std +" (zoom-in (sqrt (sqrt 2.0))))
  ("std =" (zoom-in (sqrt (sqrt 2.0))))
  ("std -" (zoom-out (sqrt (sqrt 2.0))))
  ("std 0" (change-zoom-factor 1.0))
  ("std 1" (switch-to-view-index 0))
  ("std 2" (switch-to-view-index 1))
  ("std 3" (switch-to-view-index 2))
  ("std 4" (switch-to-view-index 3))
  ("std 5" (switch-to-view-index 4))
  ("std 6" (switch-to-view-index 5))

  ;; not yet implemented
  ;; ("std t" (add-tab))
  ;; ("std tab" (next-tab))
  ;; ("std S-tab" (previous-tab))

  ;; extras
  ("std 7" (fit-all-to-screen))
  ("std 8" (fit-to-screen))
  ("std 9" (fit-to-screen-width))
  ("search std f" (search-next-match #t))
  ;; added for convenience
  ("search std F" (search-next-match #f))
) ;kbd-map

;; std v / std V 按偏好绑死，避免 dispatch 中间层导致菜单快捷键反查失败（参见 devel/0394.md）。
;; 抽成函数：改 magic-paste-shortcut 偏好时由 notify 重绑（kbd-map 对同一 key 覆盖，见
;; kbd-insert-key-binding / kbd-delete-key-binding2）。
(tm-define (kbd-apply-magic-paste-shortcut)
  (if (== (get-preference "magic-paste-shortcut") "ctrl+v")
    (kbd-map ("std v" (kbd-magic-paste)) ("std V" (kbd-paste)))
    (kbd-map ("std v" (kbd-paste)) ("std V" (kbd-magic-paste)))
  ) ;if
) ;tm-define

(kbd-apply-magic-paste-shortcut)
;; added for convenience
(debug-message "keyboard" "(generic generic-kbd): kbd-map registered\n")
