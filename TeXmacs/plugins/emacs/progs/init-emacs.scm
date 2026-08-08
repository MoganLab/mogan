
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-emacs.scm
;; DESCRIPTION : Initialize the 'emacs' plugin (Emacs look and feel keymap)
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(when (like-emacs?)
  (kbd-map (:profile emacs)

    ;; standard Emacs shortcuts
    ("emacs a" (kbd-start-line))
    ("emacs b" (kbd-left))
    ;; ("emacs d" (remove-text #t))
    ("emacs d" (kbd-delete))
    ("emacs e" (kbd-end-line))
    ("emacs f" (kbd-right))
    ("emacs g" (selection-cancel))
    ("emacs j" (insert-return))
    ("emacs k" (kill-paragraph))
    ("emacs l" (refresh-window))
    ("emacs m" (insert-return))
    ("emacs n" (kbd-down))
    ("emacs p" (kbd-up))
    ("emacs q" (make 'symbol))
    ("emacs r" (interactive-search))
    ("emacs s" (interactive-search))
    ("emacs v" (kbd-page-down))
    ("emacs w" (kbd-cut))
    ("emacs y" (kbd-paste))
    ("emacs insert" (kbd-copy))
    ("emacs _" (undo 0))
    ("emacs /" (undo 0))

    ("emacs:meta v" (kbd-page-up))
    ("emacs:meta w" (kbd-copy))
    ("emacs:meta x" (interactive exec-interactive-command))
    ("emacs:meta X" (interactive footer-eval))
    ("emacs:meta <" (go-start))
    ("emacs:meta >" (go-end))
    ("emacs:meta $" (interactive-spell))
    ("emacs:meta %" (interactive-replace))
    ("emacs:meta backspace" (backward-kill-word))
    ("emacs:meta delete" (kill-word))

    ("emacs:prefix b" (interactive go-to-buffer))
    ("emacs:prefix h" (select-all))
    ("emacs:prefix k" (close-document))
    ("emacs:prefix K" (close-document*))
    ("emacs:prefix C-c" (safely-quit-TeXmacs))
    ("emacs:prefix C-f" (interactive load-document))
    ("emacs:prefix C-i" (make 'indent))
    ("emacs:prefix C-s" (save-buffer))
    ("emacs:prefix C-w" (interactive save-buffer-as))

    ("search emacs s" (search-next-match #t))
    ("search emacs r" (search-next-match #f))

    ;; not implemented
    ;; ("emacs h ..." (help ...))
    ;; ("emacs l" (recenter-window))
    ;; ("emacs o" (open-line))
    ("emacs t" (new-document))
    ;; ("emacs u" (universal-argument))
    ;; ("emacs z" (suspend-texmacs))
    ;; ("emacs \\" (toggle-input-method))
    ;; ("emacs ]" (abort-recursive-edit))
    ;; ("emacs:meta !" (shell-command))
    ;; ("emacs:meta (" (insert-parentheses))
    ;; ("emacs:meta )" (move-past-closed-and-reindent))
    ;; ("emacs:meta *" (pop-tag-mark))                  ;; conflict altcmd *
    ;; ("emacs:meta ," (loops-tag-continue))
    ;; ("emacs:meta ." (find-tag))
    ;; ("emacs:meta /" (dabbrev-expand))                ;; conflict altcmd /
    ;; ("emacs:meta \\" (delete-horizontal-space))      ;; conflict altcmd \
    ;; ("emacs:meta :" (interactive footer-eval))       ;; conflict altcmd :
    ;; ("emacs:meta ;" (comment-dwim))                  ;; conflict altcmd ;
    ;; ("emacs:meta =" (count-lines-region))
    ;; ("emacs:meta {" (backward-paragraph))
    ;; ("emacs:meta |" (shell-command-on-region))
    ;; ("emacs:meta }" (forward-paragraph))
    ;; ("emacs:meta @" (mark-word))
    ;; ("emacs:meta a" (traverse-up))                   ;; conflict altcmd a
    ;; ("emacs:meta b" (traverse-left))
    ;; ("emacs:meta c" (capitalize-word))
    ;; ("emacs:meta e" (traverse-down))                 ;; conflict altcmd e
    ;; ("emacs:meta f" (traverse-right))                ;; conflict altcmd f
    ;; ("emacs:meta h" (mark-paragraph))
    ;; ("emacs:meta i" (tab-to-tab-stop))               ;; conflict altcmd i
    ;; ("emacs:meta j" (indent-new-command-line))
    ;; ("emacs:meta l" (downcase-word))                 ;; conflict altcmd l
    ;; ("emacs:meta m" (back-to-indentation))
    ;; ("emacs:meta q" (fill-paragraph))
    ;; ("emacs:meta r" (move-to-window-line))
    ;; ("emacs:meta t" (transpose-words))               ;; conflict altcmd t
    ;; ("emacs:meta u" (upcase-word))
    ;; ("emacs:meta y" (yank-pop))
    ;; ("emacs:meta z" (zap-to-char))
    ;; ("emacs:prefix delete" (backward-kill-sentence))
    ;; ("emacs:prefix `" (next-error))
    ;; ("emacs:prefix 0" (delete-window))
    ;; ("emacs:prefix 1" (delete-other-windows))
    ;; ("emacs:prefix 2" (split-window-vertically))
    ;; ("emacs:prefix 3" (split-window-horizontally))
    ;; ("emacs:prefix d" (dired))
    ;; ("emacs:prefix f" (set-fill-column))
    ;; ("emacs:prefix i" (interactive insert-buffer))
    ;; ("emacs:prefix l" (count-lines-page))
    ;; ("emacs:prefix m" (compose-mail))
    ;; ("emacs:prefix o" (other-window))
    ;; ("emacs:prefix s" (save-some-buffers))
    ;; ("emacs:prefix u" (advertised-undo))
    ;; ("emacs:prefix z" (repeat))
    ;; ("emacs:prefix C-@" (pop-global-mark))
    ;; ("emacs:prefix C-d" (list-directory))
    ;; ("emacs:prefix C-e" (eval-last-expression))
    ;; ("emacs:prefix C-l" (downcase-region))
    ;; ("emacs:prefix C-n" (set-goal-column))
    ;; ("emacs:prefix C-o" (delete-blank-lines))
    ;; ("emacs:prefix C-p" (mark-page))
    ;; ("emacs:prefix C-q" (toggle-read-only))
    ;; ("emacs:prefix C-r" (interactive load-readonly-buffer))
    ;; ("emacs:prefix C-t" (transpose-lines))
    ;; ("emacs:prefix C-u" (upcase-region))
    ;; ("emacs:prefix C-v" (interactive load-alternate-buffer))
    ;; ("emacs:prefix C-x" (exchange-point-and-mark))
    ;; ("emacs:prefix C-z" (suspend-texmacs))

    ;; further shortcuts for the Emacs mode
    ("F2" (open-document))
    ("S-F2" (open-document*))
    ("C-F2" (revert-buffer))
    ("M-F2" (new-document))
    ("M-S-F2" (new-document*))
    ;; ("M-C-F2" (clone-window))
    ("F3" (save-buffer))
    ("S-F3" (choose-file save-buffer-as "Save TeXmacs file" "action_save_as"))
    ("F4" (preview-buffer))
    ("S-F4" (print-buffer))
    ("C-F4" (interactive print-to-file))
    ("M-F4" (interactive print-pages))
    ("M-S-F4" (interactive print-pages-to-file))

    ("emacs =" (interactive-replace))
    ("emacs:meta g" (kbd-cancel))
    ("emacs:meta [" (undo 0))
    ("emacs:meta ]" (redo 0))

    ("A-C-tab" (geometry-circulate #t))
    ("A-C-S-tab" (geometry-circulate #f))
    ("A-C-[" (geometry-slower))
    ("A-C-]" (geometry-faster))

    ("C-<" (cursor-history-backward))
    ("C->" (cursor-history-forward))
    ("C-!" (cursor-history-add (cursor-path)))
    ("C-#" (numbered-toggle (focus-tree)))
    ("C-*" (alternate-toggle (focus-tree)))
    ("C-%" (inactive-toggle (focus-tree)))
    ("C-+" (zoom-in (sqrt (sqrt 2.0))))
    ("C--" (zoom-out (sqrt (sqrt 2.0))))
    ("C-0" (change-zoom-factor 1.0))

    ("C-7" (fit-all-to-screen))
    ("C-8" (fit-to-screen))
    ("C-9" (fit-to-screen-width))
  ) ;kbd-map

  (kbd-map (:profile emacs)
    (:require (and (not (in-prog?)) (not (in-verbatim?))))
    ("A-tab" (kbd-alternate-tab))
    ("A-S-tab" (kbd-shift-alternate-tab))
    ("A-space" (make-space "0.2spc"))
    ("A-S-space" (make-space "-0.2spc"))
    ("M-space" (make-space "0.2spc"))
    ("M-S-space" (make-space "-0.2spc"))
  ) ;kbd-map
) ;when
