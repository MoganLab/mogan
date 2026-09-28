
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

(texmacs-module (stem stem-format))

(import (liii goldfmt stem))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Scheme format for TeXmacs source files (no information loss)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format stem (:name "STEM") (:suffix "stem"))

(tm-define (texmacs->stem t)
  (format-stem-string (texmacs->stm (herk-tree->utf8-tree t)))
) ;tm-define

(tm-define (stem->texmacs text) (utf8-tree->herk-tree (stm->texmacs text)))

(tm-define (stem-snippet->texmacs text)
  (utf8-tree->herk-tree (stm-snippet->texmacs text))
) ;tm-define

(converter texmacs-tree stem-document (:function texmacs->stem))

(converter stem-document texmacs-tree (:function stem->texmacs))

(converter texmacs-tree stem-snippet (:function texmacs->stem))

(converter stem-snippet texmacs-tree (:function stem-snippet->texmacs))
