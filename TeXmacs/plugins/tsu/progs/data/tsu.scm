
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : data/tsu.scm
;; DESCRIPTION : tsu data format
;; COPYRIGHT   : (C) 2024   Darcy Shen
;;               (C) 2026   Eshaan Gupta
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data tsu))

(define-format tsu
  (:name "TSU")
  (:suffix "tsu"))

(tm-define (texmacs->tsu t)
  (serialize-tsu (herk-tree->utf8-tree t)))

(tm-define (tsu->texmacs t)
  (utf8-tree->herk-tree (parse-tsu t)))

(define (tsu-snippet->texmacs t)
  (utf8-tree->herk-tree (parse-tsu-snippet t)))

(converter tsu-document texmacs-tree
  (:function tsu->texmacs))

(converter texmacs-tree tsu-document
  (:function texmacs->tsu))

(converter tsu-snippet texmacs-tree
  (:function tsu-snippet->texmacs))

(converter texmacs-tree tsu-snippet
  (:function texmacs->tsu))
