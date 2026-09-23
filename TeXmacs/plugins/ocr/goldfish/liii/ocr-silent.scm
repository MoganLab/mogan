;;
;; MODULE      : ocr-silent.scm
;; DESCRIPTION : ocr-silent mock stub for community edition
;;

(define-library (liii ocr-silent)
  (export ocr-recognize-silent)
  (import (scheme base))
  (begin
    (define (ocr-recognize-silent raw)
      #t
    ) ;define
  ) ;begin
) ;define-library
