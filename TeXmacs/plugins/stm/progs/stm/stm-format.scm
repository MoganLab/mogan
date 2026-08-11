
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : data/stm.scm
;; DESCRIPTION : stm data format
;; COPYRIGHT   : (C) 2003  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (stm stm-format))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Scheme format for TeXmacs (no information loss)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (stm-recognizes? s)
  (and (string? s) (string-starts? s "(document (TeXmacs"))
) ;define

(define-format stm
  (:name "TeXmacs Scheme")
  (:suffix "stm")
  (:must-recognize stm-recognizes?)
) ;define-format

(converter texmacs-tree stm-document (:function texmacs->stm))

(converter stm-document texmacs-tree (:function stm->texmacs))

(converter texmacs-tree stm-snippet (:function texmacs->stm))

(converter stm-snippet texmacs-tree (:function stm-snippet->texmacs))
