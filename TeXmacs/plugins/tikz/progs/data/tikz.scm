;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tikz.scm
;; DESCRIPTION : prog format for TikZ
;; COPYRIGHT   : (C) 2026  Jack Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data tikz))

;;------------------------------------------------------------------------------
;; Format definition
;;

(define-format tikz
  (:name "TikZ source code")
  (:suffix "tikz"))

;;------------------------------------------------------------------------------
;; Conversion functions
;;

(define (texmacs->tikz x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '())))

(define (tikz->texmacs x . opts)
  (code->texmacs x))

(define (tikz-snippet->texmacs x . opts)
  (code-snippet->texmacs x))

;;------------------------------------------------------------------------------
;; Converter registration
;;

(converter texmacs-tree tikz-document
  (:function texmacs->tikz))

(converter tikz-document texmacs-tree
  (:function tikz->texmacs))

(converter texmacs-tree tikz-snippet
  (:function texmacs->tikz))

(converter tikz-snippet texmacs-tree
  (:function tikz-snippet->texmacs))
