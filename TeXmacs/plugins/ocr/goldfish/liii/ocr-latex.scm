;;
;; MODULE      : ocr-latex.scm
;; DESCRIPTION : ocr-latex mock stub for community edition
;;

(define-library (liii ocr-latex)
  (export ocr-latex-refine
    ocr-latex-preprocess
    ocr-latex-split-aligned-suffix
    math-replace-whitelist
    apply-math-whitelist
    preprocess-math-in-markdown
  ) ;export
  (import (scheme base))
  (begin
    (define math-replace-whitelist '())

    (define (apply-math-whitelist s)
      s
    ) ;define

    (define (ocr-latex-refine s)
      s
    ) ;define

    (define (ocr-latex-preprocess s)
      s
    ) ;define

    (define (ocr-latex-split-aligned-suffix s)
      (cons s "")
    ) ;define

    (define (preprocess-math-in-markdown s)
      s
    ) ;define
  ) ;begin
) ;define-library
