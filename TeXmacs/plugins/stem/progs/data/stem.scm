
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : data/stem.scm
;; DESCRIPTION : STEM (.stem) data format
;; COPYRIGHT   : (C) 2025  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data stem))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Scheme format for TeXmacs source files (no information loss)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (stem-recognizes? s)
  (and (string? s) (string-starts? s "(document (TeXmacs")))

(define-format stem
  (:name "STEM")
  (:suffix "stem")
  (:must-recognize stem-recognizes?))

(define (texmacs->stem t)
  (texmacs->stm (herk-tree->utf8-tree t)))

(define (stem->texmacs text)
  (utf8-tree->herk-tree (stm->texmacs text)))

(define (stem-snippet->texmacs text)
  (utf8-tree->herk-tree (stm-snippet->texmacs text)))

(converter texmacs-tree stem-document
  (:function texmacs->stem))

(converter stem-document texmacs-tree
  (:function stem->texmacs))

(converter texmacs-tree stem-snippet
  (:function texmacs->stem))

(converter stem-snippet texmacs-tree
  (:function stem-snippet->texmacs))
