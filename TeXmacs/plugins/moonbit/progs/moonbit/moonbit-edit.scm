;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : moonbit-edit.scm
;; DESCRIPTION : editing Moonbit programs
;; COPYRIGHT   : (C) 2025  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (moonbit moonbit-edit)
  (:use (prog prog-edit) (moonbit moonbit-mode))
) ;texmacs-module

(tm-define (get-tabstop) (:mode in-prog-moonbit?) 2)

(tm-define (kbd-paste)
  (:mode in-prog-moonbit?)
  (clipboard-paste-import "moonbit" "primary")
) ;tm-define
