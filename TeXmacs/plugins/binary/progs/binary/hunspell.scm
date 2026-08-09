
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : hunspell.scm
;; DESCRIPTION : Hunspell Binary plugin
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary hunspell) (:use (binary common)))

(define (hunspell-binary-candidates)
  (cond ((os-macos?) (list "/opt/homebrew/bin/hunspell" "/usr/local/bin/hunspell"))
        ((os-windows?) (list))
        (else (list "/usr/bin/hunspell"))
  ) ;cond
) ;define

(tm-define (find-binary-hunspell)
  (:synopsis "Find the url to the hunspell binary, return (url-none) if not found"
  ) ;:synopsis
  (find-binary (hunspell-binary-candidates) "hunspell")
) ;tm-define

(tm-define (has-binary-hunspell?) (not (url-none? (find-binary-hunspell))))

(tm-define (version-binary-hunspell) (version-binary (find-binary-hunspell)))
