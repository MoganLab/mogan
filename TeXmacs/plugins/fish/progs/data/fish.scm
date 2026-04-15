;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : fish.scm
;; DESCRIPTION : prog format for fish
;; COPYRIGHT   : (C) 2022-2025  Darcy Shen, Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data fish))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; FISH source files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format fish
  (:name "FISH source code")
  (:suffix "fis" "FIS")
) ;define-format

(define (texmacs->fish x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '()))
) ;define

(define (fish->texmacs x . opts)
  (code->texmacs x)
) ;define

(define (fish-snippet->texmacs x . opts)
  (code-snippet->texmacs x)
) ;define

(converter texmacs-tree fish-document
  (:function texmacs->fish)
) ;converter

(converter fish-document texmacs-tree
  (:function fish->texmacs)
) ;converter

(converter texmacs-tree fish-snippet
  (:function texmacs->fish)
) ;converter

(converter fish-snippet texmacs-tree
  (:function fish-snippet->texmacs)
) ;converter
