
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : gnuplot.scm
;; DESCRIPTION : gnuplot Binary plugin
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary gnuplot) (:use (binary common)))

(define (gnuplot-binary-candidates)
  (cond ((os-macos?) (list "/opt/homebrew/bin/gnuplot" "/usr/local/bin/gnuplot"))
        ((os-win32?)
         (list "C:\\Program Files*\\gnuplot\\bin\\gnuplot.exe"
           "$USERPROFILE\\scoop\\apps\\gnuplot\\current\\bin\\gnuplot.exe"
         ) ;list
        ) ;
        (else (list "/usr/bin/gnuplot"))
  ) ;cond
) ;define

(tm-define (find-binary-gnuplot)
  (:synopsis "Find the url to the gnuplot binary, return (url-none) if not found")
  (find-binary (gnuplot-binary-candidates) "gnuplot")
) ;tm-define

(tm-define (has-binary-gnuplot?) (not (url-none? (find-binary-gnuplot))))

(tm-define (version-binary-gnuplot) (version-binary (find-binary-gnuplot)))
