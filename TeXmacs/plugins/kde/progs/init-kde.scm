
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

  (kbd-map (:profile kde)
    (:require (and (not (in-prog?)) (not (in-verbatim?))))
    ("M-space" (make-space "0.2spc"))
    ("M-S-space" (make-space "-0.2spc"))
  ) ;kbd-map
) ;when
