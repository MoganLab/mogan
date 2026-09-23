;;
;; MODULE      : ocr-wait.scm
;; DESCRIPTION : ocr-wait mock stub for community edition
;;

(define-library (liii ocr-wait)
  (export ocr-wait-open ocr-wait-close set-ocr-wait-provider!)
  (import (scheme base))
  (begin
    (define open-provider (lambda (msg on-cancel) #f))
    (define close-provider (lambda () #f))

    (define (set-ocr-wait-provider! open-proc close-proc)
      (set! open-provider open-proc)
      (set! close-provider close-proc)
    ) ;define

    (define (ocr-wait-open msg on-cancel)
      (open-provider msg on-cancel)
    ) ;define

    (define (ocr-wait-close)
      (close-provider)
    ) ;define
  ) ;begin
) ;define-library
