
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-macos.scm
;; DESCRIPTION : Initialize the 'macos' plugin (Mac OS look and feel keymap)
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(when (like-macos?)
  (kbd-map (:profile macos)

    ;; standard macOS keyboard shortcuts
    ("macos ;" (interactive-spell))
    ("macos ?" (interactive docgrep-in-doc))
    ("macos [" (cursor-history-backward))
    ("macos ]" (cursor-history-forward))
    ("macos _" (make 'nbhyph))
    ("macos up" (go-start))
    ("macos down" (go-end))
    ("macos left" (kbd-select-if-active traverse-left))
    ("macos right" (kbd-select-if-active traverse-right))
    ("macos S-left" (kbd-select kbd-start-line))
    ("macos S-right" (kbd-select kbd-end-line))
    ("macos S-up" (kbd-select go-start))
    ("macos S-down" (kbd-select go-end))

    ("search macos g" (search-next-match #t))
    ("search macos G" (search-next-match #f))

    ;; not yet supported
    ;; ("macos :" (display-spelling-window))
    ;; ("macos ," (open-preferences))
    ;; ("macos A-/" (toggle-antialising))
    ;; ("macos #" (capture-screen-to-file))
    ;; ("macos C-#" (capture-screen-to-clipboard))
    ;; ("macos $" (capture-selection-to-file))
    ;; ("macos C-$" (capture-selection-to-clipboard))
    ;; ("macos C" (show-colors-window))
    ;; ("macos C-c" (copy-style))
    ;; ("macos A-c" (copy-formatting))
    ;; ("macos C-d" (show-definition-word))
    ;; ("macos A-d" (toggle-doc))
    ;; ("macos e" (search-selection))
    ;; ("macos h" (hide-window))
    ;; ("macos A-h" (hide-other-windows))
    ;; ("macos A-i" (show-inspector-window))
    ;; ("macos j" (scroll-to-selection))
    ;; ("macos m" (minimize-window))
    ;; ("macos A-m" (minimize-all-windows))
    ;; ("macos P" (printer-setup))
    ;; ("macos t" (show-fonts-window))
    ;; ("macos A-t" (toggle-toolbar))
    ;; ("macos C-v" (paste-style))
    ;; ("macos C-V" (paste-match-style))
    ;; ("macos A-v" (paste-formatting))
    ;; ("macos A-w" (safely-kill-all-windows))
    ;; ("macos C-x" (cut-style))       ;; TeXmacs addition
    ;; ("macos A-x" (cut-formatting))  ;; TeXmacs addition

    ;; further shortcuts for MacOS look and feel
    ("macos r" (interactive-replace))
    ("macos F" (toggle-full-screen-mode))
    ("macos C-f" (toggle-full-screen-edit-mode))

    ("macos S-=" (zoom-in (sqrt (sqrt 2.0))))
    ("macos S-+" (zoom-in (sqrt (sqrt 2.0))))
    ("macos S--" (zoom-out (sqrt (sqrt 2.0))))
    ("macos S-_" (zoom-out (sqrt (sqrt 2.0))))

    ("altcmd x" (interactive footer-eval))
    ("A-x" (interactive exec-interactive-command))

    ("A-space var" (make 'nbsp))

    ("C-a" (kbd-start-line))
    ("C-e" (kbd-end-line))
    ("C-b" (kbd-left))
    ("C-f" (kbd-right))
    ("C-g" (selection-cancel))
    ("C-k" (kill-paragraph))
    ("C-l" (refresh-window))
    ("C-y" (yank-paragraph))
    ("C-n" (kbd-down))
    ("C-p" (kbd-up))
    ("C-h" (kbd-backspace))
    ("C-d" (kbd-delete))
    ("A-q" (make 'symbol))

    ("C-O" (toggle-source-mode))
    ("C-P" (toggle-preamble-mode))
    ("M-A-v" (interactive-paste-special))
  ) ;kbd-map
) ;when
