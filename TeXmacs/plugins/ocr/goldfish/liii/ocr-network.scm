;;
;; MODULE      : ocr-network.scm
;; DESCRIPTION : ocr-network mock stub for community edition
;;

(define-library (liii ocr-network)
  (export set-ocr-auth-provider!
    content-to-either
    response-to-either
    upload-base64-body
    upload-base64
    upload-base64-async
    recognize-body
    recognize
    recognize-async
  ) ;export
  (import (scheme base))
  (begin
    (define (set-ocr-auth-provider! headers-proc site-proc)
      #t
    ) ;define

    (define (content-to-either res)
      #f
    ) ;define

    (define (response-to-either res)
      #f
    ) ;define

    (define (upload-base64-body base64-str)
      ""
    ) ;define

    (define (upload-base64 base64-str)
      #f
    ) ;define

    (define (upload-base64-async base64-str callback)
      #f
    ) ;define

    (define (recognize-body key mode)
      ""
    ) ;define

    (define (recognize key mode)
      #f
    ) ;define

    (define (recognize-async key mode callback)
      #f
    ) ;define
  ) ;begin
) ;define-library
