
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : gnuplot.scm
;; DESCRIPTION : prog format for Gnuplot
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data gnuplot))

(define-format gnuplot (:name "Gnuplot Source Code") (:suffix "gp"))

(define (texmacs->gnuplot x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '()))
) ;define

(define (gnuplot->texmacs x . opts)
  (code->texmacs x)
) ;define

(define (gnuplot-snippet->texmacs x . opts)
  (code-snippet->texmacs x)
) ;define

(converter texmacs-tree gnuplot-document (:function texmacs->gnuplot))

(converter gnuplot-document texmacs-tree (:function gnuplot->texmacs))

(converter texmacs-tree gnuplot-snippet (:function texmacs->gnuplot))

(converter gnuplot-snippet texmacs-tree (:function gnuplot-snippet->texmacs))
