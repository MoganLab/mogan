
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : aspell.scm
;; DESCRIPTION : aspell Binary plugin
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary aspell) (:use (binary common)))

(define (aspell-binary-candidates)
  (cond ((os-macos?) (list "/opt/homebrew/bin/aspell" "/usr/local/bin/aspell"))
        ((os-windows?)
         (list "$USERPROFILE\\scoop\\apps\\aspell\\current\\bin\\aspell.exe")
        ) ;
        (else (list "/usr/bin/aspell"))
  ) ;cond
) ;define

(tm-define (find-binary-aspell)
  (:synopsis "Find the url to the aspell binary, return (url-none) if not found")
  (find-binary (aspell-binary-candidates) "aspell")
) ;tm-define

(tm-define (has-binary-aspell?) (not (url-none? (find-binary-aspell))))

(tm-define (version-binary-aspell) (version-binary (find-binary-aspell)))
