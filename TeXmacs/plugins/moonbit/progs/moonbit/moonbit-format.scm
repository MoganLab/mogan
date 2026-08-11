
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-prog_moonbit.scm
;; DESCRIPTION : prog format for Moonbit
;; COPYRIGHT   : (C) 2025  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (moonbit moonbit-format))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Moonbit source files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format moonbit (:name "Moonbit source code") (:suffix "moonbit" "mbt"))

(define (texmacs->moonbit x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '()))
) ;define

(define (moonbit->texmacs x . opts)
  (code->texmacs x)
) ;define

(define (moonbit-snippet->texmacs x . opts)
  (code-snippet->texmacs x)
) ;define

(converter texmacs-tree moonbit-document (:function texmacs->moonbit))

(converter moonbit-document texmacs-tree (:function moonbit->texmacs))

(converter texmacs-tree moonbit-snippet (:function texmacs->moonbit))

(converter moonbit-snippet texmacs-tree (:function moonbit-snippet->texmacs))
