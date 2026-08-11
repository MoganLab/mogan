;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MODULE      : bash.scm
;; DESCRIPTION : Bash format definition (minimal)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (bash bash-format))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Bash source files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format bash (:name "Bash shell script") (:suffix "sh" "bash"))

(define (texmacs->bash x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '()))
) ;define

(define (bash->texmacs x . opts)
  (code->texmacs x)
) ;define

(define (bash-snippet->texmacs x . opts)
  (code-snippet->texmacs x)
) ;define

(converter texmacs-tree bash-document (:function texmacs->bash))

(converter bash-document texmacs-tree (:function bash->texmacs))

(converter texmacs-tree bash-snippet (:function texmacs->bash))

(converter bash-snippet texmacs-tree (:function bash-snippet->texmacs))
