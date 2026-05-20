
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : pdflatex.scm
;; DESCRIPTION : TikZ Binary plugin (pdflatex)
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;                   2026  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary pdflatex)
  (:use (binary common)))

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
