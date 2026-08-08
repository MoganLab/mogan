
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-gnome.scm
;; DESCRIPTION : Initialize the 'gnome' plugin (Gnome look and feel keymap)
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(when (like-gnome?)
  (kbd-map (:profile gnome)

    ;; standard Gnome shortcuts
    ("gnome d" (remove-text #t))
    ("gnome h" (interactive-replace))
    ("gnome k" (kill-paragraph))
    ("gnome left" (traverse-left))
    ("gnome right" (traverse-right))
    ("gnome home" (go-start))
    ("gnome end" (go-end))
    ("gnome S-left" (kbd-select traverse-left))
    ("gnome S-right" (kbd-select traverse-right))
    ("gnome S-home" (kbd-select go-start))
    ("gnome S-end" (kbd-select go-end))

    ("F14" (undo 0))
    ("F16" (kbd-copy))
    ("F18" (kbd-paste))
    ("F20" (kbd-cut))
    ("C-insert" (kbd-copy))
    ("S-insert" (kbd-paste))
    ("S-delete" (kbd-cut))
    ("gnome c" (kbd-copy))
    ("gnome v" (kbd-paste))
    ("gnome x" (kbd-cut))

    ("search F3" (search-next-match #t))
    ("search S-F3" (search-next-match #f))
    ("search gnome g" (search-next-match #t))
    ("search gnome G" (search-next-match #f))

    ;; not yet implemented
    ;; ("gnome delete" (delete-end-word))
    ;; ("gnome backspace" (delete-start-word))
    ;; ("forward" (next-tab))
    ;; ("back" (previous-tab))

    ;; further shortcuts for Gnome look and feel
    ("gnome g" (selection-cancel))
    ("gnome l" (refresh-window))
    ("gnome F" (interactive-search))

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

  (kbd-map (:profile gnome)
    (:require (and (not (in-prog?)) (not (in-verbatim?))))
    ("M-space" (make-space "0.2spc"))
    ("M-S-space" (make-space "-0.2spc"))
  ) ;kbd-map
) ;when
