
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-kde.scm
;; DESCRIPTION : Initialize the 'kde' plugin (KDE look and feel keymap)
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(when (like-kde?)
  (kbd-map (:profile kde)

    ;; standard KDE shortcuts
    ("kde d" (remove-text #t))
    ("kde k" (kill-paragraph))
    ("kde r" (interactive-replace))
    ("kde left" (traverse-left))
    ("kde right" (traverse-right))
    ("kde home" (go-start))
    ("kde end" (go-end))
    ("kde S-left" (kbd-select traverse-left))
    ("kde S-right" (kbd-select traverse-right))
    ("kde S-home" (kbd-select go-start))
    ("kde S-end" (kbd-select go-end))

    ("F14" (undo 0))
    ("F16" (kbd-copy))
    ("F18" (kbd-paste))
    ("F20" (kbd-cut))
    ("C-insert" (kbd-copy))
    ("S-insert" (kbd-paste))
    ("S-delete" (kbd-cut))

    ("search F3" (search-next-match #t))
    ("search S-F3" (search-next-match #f))

    ;; not yet implemented
    ;; ("kde N" (add-tab))
    ;; ("kde delete" (delete-end-word))
    ;; ("kde backspace" (delete-start-word))
    ;; ("forward" (next-tab))
    ;; ("back" (previous-tab))

    ;; further shortcuts for KDE look and feel
    ("kde g" (selection-cancel))
    ("kde l" (refresh-window))
    ("kde F" (interactive-search))

    ("cmd q" (make 'symbol))
    ("altcmd g" (kbd-cancel))
    ("altcmd x" (interactive footer-eval))
    ("A-x" (interactive exec-interactive-command))
    ("altcmd $" (interactive-spell))

    ("C-P" (toggle-preamble-mode))
    ("C-O" (toggle-source-mode))

    ("structured:cmd left" (kbd-select-if-active traverse-left))
    ("structured:cmd right" (kbd-select-if-active traverse-right))
  ) ;kbd-map

  ;; C-insert/S-insert/S-delete/F16/F18/F20 等传统备用键注册晚于 generic-kbd 的
  ;; std 主键，而菜单快捷键反查（kbd-find-inv-binding）按后注册先出取键，会把
  ;; 复制/剪切/粘贴显示成 Ctrl+Ins/Shift+Del/Shift+Ins（或 F16/F18/F20）。这里
  ;; 重绑 std 主键把 C-c/C-x/C-v 顶回队首；std v/std V 的偏好分支须与
  ;; generic-kbd.scm 的 kbd-apply-magic-paste-shortcut 保持一致。
  (kbd-map (:profile kde) ("std c" (kbd-copy)) ("std x" (kbd-cut)))
  (if (== (get-preference "magic-paste-shortcut") "ctrl+v")
    (kbd-map ("std v" (kbd-magic-paste)) ("std V" (kbd-paste)))
    (kbd-map ("std v" (kbd-paste)) ("std V" (kbd-magic-paste)))
  ) ;if

  (kbd-map (:profile kde)
    (:require (and (not (in-prog?)) (not (in-verbatim?))))
    ("M-space" (make-space "0.2spc"))
    ("M-S-space" (make-space "-0.2spc"))
  ) ;kbd-map
) ;when
