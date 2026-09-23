;;
;; MODULE      : ocr-title.scm
;; DESCRIPTION : ocr-title mock stub for community edition
;;

(define-library (liii ocr-title)
  (export parse-title-prefix)
  (import (scheme base))
  (begin
    (define (parse-title-prefix title)
      (list 'plain 1 title)
    ) ;define
  ) ;begin
) ;define-library
