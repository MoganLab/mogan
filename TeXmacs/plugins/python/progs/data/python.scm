
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : python.scm
;; DESCRIPTION : prog format for Python
;; COPYRIGHT   : (C) 2022  Darcy Shen, Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data python))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; python source files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format python (:name "Python source code") (:suffix "py" "pants"))

(define (texmacs->python x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '()))
) ;define

(define (python->texmacs x . opts)
  (code->texmacs x)
) ;define

(define (python-snippet->texmacs x . opts)
  (code-snippet->texmacs x)
) ;define

(converter texmacs-tree python-document (:function texmacs->python))

(converter python-document texmacs-tree (:function python->texmacs))

(converter texmacs-tree python-snippet (:function texmacs->python))

(converter python-snippet texmacs-tree (:function python-snippet->texmacs))
