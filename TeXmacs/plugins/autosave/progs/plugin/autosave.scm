
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : autosave.scm
;; DESCRIPTION : Entry point of the (plugin autosave) module
;;
;; This module serves as the entry point loaded by init-autosave.scm.
;; It MUST NOT contain (import ...) statements.
;;
;; Rationale: S7's R7RS (import ...) modifies the evaluation environment
;; in a way that breaks the texmacs module system when the importing module
;; is loaded directly by (use-modules ...). By keeping this top-level loader
;; free of (import ...), the deeper dependency module (autosave-impl) can
;; safely use (import ...).
;;
;; COPYRIGHT   : (C) 2026  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (plugin autosave) (:use (plugin autosave-impl)))
