;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : julia.scm
;; DESCRIPTION : julia Binary plugin
;; COPYRIGHT   : (C) 2026 Tianyou Liu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary julia)
  (:use (binary common)))

(define (julia-binary-candidates)
  (cond ((os-macos?)
         (list "/Applications/Julia-*.app/Contents/Resources/julia/bin/julia"
               "$HOME/Applications/Julia-*.app/Contents/Resources/julia/bin/julia"
               "/opt/homebrew/bin/julia"
               "/usr/local/bin/julia"))
        ((os-win32?)
         (list "$LOCALAPPDATA/Programs/Julia*/bin/julia.exe"
               "C:\\Program Files*\\Julia*\\bin\\julia.exe"))
        (else
         (list "/usr/bin/julia"))))

(tm-define (find-binary-julia)
  (:synopsis "Find the url to the julia binary, return (url-none) if not found")
  (find-binary (julia-binary-candidates) "julia"))

(tm-define (has-binary-julia?)
  (not (url-none? (find-binary-julia))))

(tm-define (version-binary-julia)
  (version-binary (find-binary-julia)))
