;;
;; MODULE      : ocr-poller.scm
;; DESCRIPTION : ocr-poller mock stub for community edition
;;

(define-library (liii ocr-poller)
  (export ensure-ocr-poller set-ocr-poll-delayer! ocr-poll-delay)
  (import (scheme base))
  (begin
    (define poll-delayer (lambda (thunk) (thunk)))

    (define (set-ocr-poll-delayer! delayer)
      (set! poll-delayer delayer)
    ) ;define

    (define (ocr-poll-delay thunk)
      (poll-delayer thunk)
    ) ;define

    (define (ensure-ocr-poller)
      #t
    ) ;define
  ) ;begin
) ;define-library
