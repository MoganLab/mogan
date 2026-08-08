
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-windows.scm
;; DESCRIPTION : Initialize the 'windows' plugin (Windows look and feel keymap)
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(when (like-windows?)
  (kbd-map (:profile windows)

    ;; standard Windows shortcuts
    ("windows F4" (close-document))
    ("windows S-F4" (close-document*))
    ("windows left" (traverse-left))
    ("windows right" (traverse-right))
    ("windows home" (go-start))
    ("windows end" (go-end))
    ("windows S-left" (kbd-select traverse-left))
    ("windows S-right" (kbd-select traverse-right))
    ("windows S-home" (kbd-select go-start))
    ("windows S-end" (kbd-select go-end))
    ("windows S-space" (make 'nbsp))
    ("windows _" (make 'nbhyph))
    ("windows A-." "<ldots>")
    ("windows A-c" (make 'copyright))
    ("windows e" (make 'footnote))
    ("windows F" (make 'footnote))
    ("windows h" (interactive-replace))
    ("windows K" (toggle-small-caps))
    ("windows A-r" (make 'registered))
    ("windows A-t" (make 'trademark))
    ("windows y" (redo 0))

    ("F2" (interactive-replace))
    ("S-delete" (kbd-cut))
    ("S-insert" (kbd-paste))
    ("C-insert" (kbd-copy))
    ("A-F4" (close-document))
    ("A-S-F4" (close-document*))

    ("search windows g" (search-next-match #t))
    ("search windows G" (search-next-match #f))
    ("search F3" (search-next-match #t))
    ("search S-F3" (search-next-match #f))

    ;; not yet implemented
    ;; ("F4" (go-to-different-folder))
    ;; ("F5" (refresh-window))
    ;; ("F6" (switch-to-next-pane))
    ;; ("F8" (kbd-select-enlarge))
    ;; ("F9" (refresh-web-page))
    ;; ("C-F9" (insert-field))
    ;; ("F10" (menu-bar-options))
    ;; ("S-F10" (open-contextual-menu))
    ;; ("windows F4" (close-mdi-window))
    ;; ("windows F6" (next-tab))
    ;; ("windows S-F6" (previous-tab))
    ;; ("windows delete" (delete-end-word))
    ;; ("windows backspace" (delete-start-word))
    ;; ("windows tab" (switch-to-next-child))
    ;; ("windows escape" (open-start-menu))
    ;; ("windows S-escape" (open-task-manager))
    ;; ("windows S-return" (insert-section-break))
    ;; ("windows C" (copy-formatting))
    ;; ("windows D" (insert-endnote))
    ;; ("windows E" (review-toggle-track-changes))
    ;; ("windows g" (go-to-location))
    ;; ("windows A-m" (review-insert-comment))
    ;; ("windows V" (paste-formatting))
    ;; ("windows A-y" (search-repeat))
    ;; ("windows <" (decrease-font-size))
    ;; ("windows >" (increase-font-size))
    ;; ("windows [" (decrease-font-size-one-point))
    ;; ("windows ]" (increase-font-size-one-point))
    ;; ("windows ` `" (open-single-quotation))
    ;; ("windows ' '" (close-single-quotation))
    ;; ("windows ` C-`" (open-double-quotation))
    ;; ("windows ' C-'" (close-double-quotation))
    ;; ("S-delete" (delete-selection-immediately))
    ;; ("A-F6" (switch-to-next-window))
    ;; ("A-tab" (switch-to-next-program))
    ;; ("A-down" (open-drop-down-list-box))
    ;; ("A-space" (open-system-menu))
    ;; ("A-return" (open-properties-window))
    ;; ("A-I" (insert-citation-entry))
    ;; ("A-O" (insert-toc-entry))
    ;; ("A-X" (insert-index-entry))
    ;; ("A--" (open-child-system-menu))
    ;; ("A-_" (open-menu))
    ;; ("M-F1" (run-dialog-box))
    ;; ("M-tab" (cycle-taskbar-button))
    ;; ("M-space" (show-keyboard-shortcuts))
    ;; ("M-c" (open-control-panel))
    ;; ("M-d" (minimize-all-open-windows))
    ;; ("M-e" (open-explorer-window))
    ;; ("M-C-f" (find-computer))
    ;; ("M-f" (open-finder-window))
    ;; ("M-i" (open-mouse-properties-window))
    ;; ("M-k" (open-keyboard-properties-window))
    ;; ("M-l" (log-off-windows))
    ;; ("M-m" (minimize-all-windows))
    ;; ("M-M" (unminimize-all-windows))
    ;; ("M-p" (start-print-manager))
    ;; ("M-r" (run-dialog-box))
    ;; ("M-s" (toggle-caps-lock))
    ;; ("M-v" (start-clipboard))
    ;; ("forward" (next-tab))
    ;; ("back" (previous-tab))

    ;; further shortcuts for Windows look and feel
    ("windows g" (selection-cancel))
    ("windows l" (refresh-window))
    ("windows =" (change-zoom-factor 1.0))

    ("cmd q" (make 'symbol))
    ("altcmd g" (kbd-cancel))
    ("altcmd x" (interactive footer-eval))
    ("A-x" (interactive exec-interactive-command))
    ("altcmd $" (interactive-spell))

    ("structured:cmd left" (kbd-select-if-active traverse-left))
    ("structured:cmd right" (kbd-select-if-active traverse-right))

    ("C-O" (toggle-source-mode))
    ("C-P" (toggle-preamble-mode))
  ) ;kbd-map

  (kbd-map (:profile windows)
    (:require (and (not (in-prog?)) (not (in-verbatim?))))
    ("M-space" (make-space "0.2spc"))
    ("M-S-space" (make-space "-0.2spc"))
  ) ;kbd-map
) ;when
