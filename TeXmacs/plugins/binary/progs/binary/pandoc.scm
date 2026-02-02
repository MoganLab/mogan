;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : pandoc-binary.scm
;; DESCRIPTION : Pandoc Binary plugin
;; COPYRIGHT   : (C) 2024  ATQlove
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary pandoc)
  (:use (binary common)))

(define (pandoc-binary-candidates)
  (cond ((os-macos?)
         (list "/usr/local/bin/pandoc"
               "/opt/homebrew/bin/pandoc"))
        ((os-win32?)
         (list "$TEXMACS_PATH\\bin\\pandoc.exe"
               "C:\\Program Files\\Pandoc\\pandoc.exe"))
        (else
         (list "/usr/bin/pandoc" "/usr/local/bin/pandoc"))))

(tm-define (find-binary-pandoc)
  (:synopsis "Find the url to the pandoc binary, return (url-none) if not found")
  (find-binary (pandoc-binary-candidates) "pandoc"))

(tm-define (has-binary-pandoc?)
  (not (url-none? (find-binary-pandoc))))

(tm-define (version-binary-pandoc)
  (version-binary (find-binary-pandoc)))
