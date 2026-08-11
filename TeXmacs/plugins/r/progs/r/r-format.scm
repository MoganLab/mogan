;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MODULE      : r.scm
;; DESCRIPTION : R format definition (minimal)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (r r-format))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; R source files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format r
  (:name "R source code")
  (:suffix "r" "R"))

(define (texmacs->r x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '())))

(define (r->texmacs x . opts)
  (code->texmacs x))

(define (r-snippet->texmacs x . opts)
  (code-snippet->texmacs x))

(converter texmacs-tree r-document
  (:function texmacs->r))

(converter r-document texmacs-tree
  (:function r->texmacs))
  
(converter texmacs-tree r-snippet
  (:function texmacs->r))

(converter r-snippet texmacs-tree
  (:function r-snippet->texmacs))