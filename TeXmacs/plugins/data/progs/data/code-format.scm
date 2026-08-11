
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : code.scm
;; DESCRIPTION : prog format
;; COPYRIGHT   : (C) 2022  Darcy Shen, Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data code-format))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; C++ source files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format cpp (:name "C++ source code") (:suffix "cpp" "cc" "hpp" "hh"))

(define (texmacs->cpp x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '()))
) ;define

(define (cpp->texmacs x . opts)
  (code->texmacs x)
) ;define

(define (cpp-snippet->texmacs x . opts)
  (code-snippet->texmacs x)
) ;define

(converter texmacs-tree cpp-document (:function texmacs->cpp))

(converter cpp-document texmacs-tree (:function cpp->texmacs))

(converter texmacs-tree cpp-snippet (:function texmacs->cpp))

(converter cpp-snippet texmacs-tree (:function cpp-snippet->texmacs))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Scheme source files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format scheme (:name "Scheme source code") (:suffix "scm"))

(define (texmacs->scheme x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '()))
) ;define

(define (scheme->texmacs x . opts)
  (verbatim->texmacs x (acons "verbatim->texmacs:encoding" "SourceCode" '()))
) ;define

(define (scheme-snippet->texmacs x . opts)
  (verbatim-snippet->texmacs x
    (acons "verbatim->texmacs:encoding" "SourceCode" '())
  ) ;verbatim-snippet->texmacs
) ;define

(converter texmacs-tree scheme-document (:function texmacs->scheme))

(converter scheme-document texmacs-tree (:function scheme->texmacs))

(converter texmacs-tree scheme-snippet (:function texmacs->scheme))

(converter scheme-snippet texmacs-tree (:function scheme-snippet->texmacs))
