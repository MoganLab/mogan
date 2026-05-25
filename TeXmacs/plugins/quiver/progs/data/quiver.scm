;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : quiver.scm
;; DESCRIPTION : prog format for Quiver
;; COPYRIGHT   : (C) 2026 (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data quiver))

;;------------------------------------------------------------------------------
;; Format definition
;;

(define-format quiver
  (:name "Quiver source code")
  (:suffix "quiver"))

;;------------------------------------------------------------------------------
;; Conversion functions
;;

(define (texmacs->quiver x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '())))

(define (quiver->texmacs x . opts)
  (code->texmacs x))

(define (quiver-snippet->texmacs x . opts)
  (code-snippet->texmacs x))

;;------------------------------------------------------------------------------
;; Converter registration
;;

(converter texmacs-tree quiver-document
  (:function texmacs->quiver))

(converter quiver-document texmacs-tree
  (:function quiver->texmacs))

(converter texmacs-tree quiver-snippet
  (:function texmacs->quiver))

(converter quiver-snippet texmacs-tree
  (:function quiver-snippet->texmacs))
