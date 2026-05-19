
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tikz.scm
;; DESCRIPTION : TikZ Binary plugin (latex / dvips)
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary tikz)
  (:use (binary common)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; latex
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (latex-binary-candidates)
  (cond ((os-macos?)
         (list "/Library/TeX/texbin/latex"
           "/usr/texbin/latex"
           "/opt/homebrew/bin/latex"
           "/usr/local/bin/latex"
         ))
        ((os-win32?)
         (list
          "C:\\Program Files*\\MiKTeX*\\miktex\\bin\\x64\\latex.exe"
          "C:\\Program Files*\\MiKTeX*\\miktex\\bin\\latex.exe"
         ))
        (else
         (list "/usr/bin/latex"
           "/usr/local/bin/latex"
         ))))

(tm-define (find-binary-latex)
  (:synopsis "Find the url to the latex binary, return (url-none) if not found")
  (find-binary (latex-binary-candidates) "latex"))

(tm-define (has-binary-latex?)
  (not (url-none? (find-binary-latex))))

(tm-define (version-binary-latex)
  (version-binary (find-binary-latex)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; dvips
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (dvips-binary-candidates)
  (cond ((os-macos?)
         (list "/Library/TeX/texbin/dvips"
           "/usr/texbin/dvips"
           "/opt/homebrew/bin/dvips"
           "/usr/local/bin/dvips"
         ))
        ((os-win32?)
         (list
          "C:\\Program Files*\\MiKTeX*\\miktex\\bin\\x64\\dvips.exe"
          "C:\\Program Files*\\MiKTeX*\\miktex\\bin\\dvips.exe"
         ))
        (else
         (list "/usr/bin/dvips"
           "/usr/local/bin/dvips"
         ))))

(tm-define (find-binary-dvips)
  (:synopsis "Find the url to the dvips binary, return (url-none) if not found")
  (find-binary (dvips-binary-candidates) "dvips"))

(tm-define (has-binary-dvips?)
  (not (url-none? (find-binary-dvips))))

(tm-define (version-binary-dvips)
  (version-binary (find-binary-dvips)))
