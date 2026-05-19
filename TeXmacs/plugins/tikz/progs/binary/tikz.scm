;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tikz.scm
;; DESCRIPTION : LaTeX Binary plugin for TikZ
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;                   2026  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary tikz) (:use (binary common)))

(define (tikz-binary-candidates)
  (cond ((os-macos?)
         (list "/Library/TeX/texbin/pdflatex"
           "/usr/local/bin/pdflatex"
           "/opt/homebrew/bin/pdflatex"
           "/Library/TeX/texbin/xelatex"
           "/usr/local/bin/xelatex"
           "/opt/homebrew/bin/xelatex"
           "/Library/TeX/texbin/lualatex"
           "/usr/local/bin/lualatex"
           "/opt/homebrew/bin/lualatex"
           "/Library/TeX/texbin/latex"
           "/usr/local/bin/latex"
           "/opt/homebrew/bin/latex"
         ) ;list
        ) ;
        ((os-win32?)
         (list "C:\\Program Files\\MiKTeX\\miktex\\bin\\x64\\pdflatex.exe"
           "C:\\texlive\\2024\\bin\\win32\\pdflatex.exe"
           "C:\\texlive\\2023\\bin\\win32\\pdflatex.exe"
         ) ;list
        ) ;
        (else (list "/usr/bin/pdflatex"
                "/usr/local/bin/pdflatex"
                "/usr/bin/xelatex"
                "/usr/local/bin/xelatex"
                "/usr/bin/lualatex"
                "/usr/local/bin/lualatex"
                "/usr/bin/latex"
                "/usr/local/bin/latex"
              ) ;list
        ) ;else
  ) ;cond
) ;define

(tm-define (find-binary-tikz)
  (:synopsis "Find the url to the LaTeX binary, return (url-none) if not found")
  (find-binary (tikz-binary-candidates) "tikz")
) ;tm-define

(tm-define (has-binary-tikz?) (not (url-none? (find-binary-tikz))))

(tm-define (version-binary-tikz) (version-binary (find-binary-tikz)))
