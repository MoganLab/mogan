
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tikz.scm
;; DESCRIPTION : TikZ Binary plugin (latex / dvips)
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;                   2026  (Jack) Yansong Li
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
;; pdflatex
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (pdflatex-binary-candidates)
  (cond ((os-macos?)
         (list "/Library/TeX/texbin/pdflatex"
           "/usr/texbin/pdflatex"
           "/opt/homebrew/bin/pdflatex"
           "/usr/local/bin/pdflatex"
         ))
        ((os-win32?)
         (list
          "C:\\Program Files*\\MiKTeX*\\miktex\\bin\\x64\\pdflatex.exe"
          "C:\\Program Files*\\MiKTeX*\\miktex\\bin\\pdflatex.exe"
         ))
        (else
         (list "/usr/bin/pdflatex"
           "/usr/local/bin/pdflatex"
         ))))

(tm-define (find-binary-pdflatex)
  (:synopsis "Find the url to the pdflatex binary, return (url-none) if not found")
  (find-binary (pdflatex-binary-candidates) "pdflatex"))

(tm-define (has-binary-pdflatex?)
  (not (url-none? (find-binary-pdflatex))))

(tm-define (version-binary-pdflatex)
  (version-binary (find-binary-pdflatex)))
