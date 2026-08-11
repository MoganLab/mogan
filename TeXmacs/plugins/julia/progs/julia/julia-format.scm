
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-prog_julia.scm
;; DESCRIPTION : prog format for Julia
;; COPYRIGHT   : (C) 2022  Darcy Shen, Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (julia julia-format))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Julia source files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format julia (:name "Julia source code") (:suffix "jl"))

(define (texmacs->julia x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '()))
) ;define

(define (julia->texmacs x . opts)
  (code->texmacs x)
) ;define

(define (julia-snippet->texmacs x . opts)
  (code-snippet->texmacs x)
) ;define

(converter texmacs-tree julia-document (:function texmacs->julia))

(converter julia-document texmacs-tree (:function julia->texmacs))

(converter texmacs-tree julia-snippet (:function texmacs->julia))

(converter julia-snippet texmacs-tree (:function julia-snippet->texmacs))
