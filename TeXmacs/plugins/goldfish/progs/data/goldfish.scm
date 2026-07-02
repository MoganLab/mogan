
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : goldfish.scm
;; DESCRIPTION : prog format for goldfish
;; COPYRIGHT   : (C) 2022  Darcy Shen, Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data goldfish))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; goldfish source files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format goldfish
  (:name "Goldfish source code")
  (:suffix "scm" ".sld" ".ss")
) ;define-format

(define (texmacs->goldfish x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '()))
) ;define

(define (goldfish->texmacs x . opts)
  (code->texmacs x)
) ;define

(define (goldfish-snippet->texmacs x . opts)
  (code-snippet->texmacs x)
) ;define

(converter texmacs-tree goldfish-document (:function texmacs->goldfish))

(converter goldfish-document texmacs-tree (:function goldfish->texmacs))

(converter texmacs-tree goldfish-snippet (:function texmacs->goldfish))

(converter goldfish-snippet texmacs-tree (:function goldfish-snippet->texmacs))
