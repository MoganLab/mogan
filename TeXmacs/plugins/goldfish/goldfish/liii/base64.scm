(define-library (liii base64)
  (import (scheme base) (liii base) (liii bitwise) (liii error))
  (export string-base64-encode
    bytevector-base64-encode
    base64-encode
    string-base64-decode
    bytevector-base64-decode
    base64-decode
  ) ;export
  (begin
    (define bytevector-base64-encode
      (typed-lambda ((bv bytevector?))
        (g_bytevector-base64-encode bv (bytevector-length bv))
      ) ;typed-lambda
    ) ;define

    (define string-base64-encode
      (typed-lambda ((str string?))
        (utf8->string (bytevector-base64-encode (string->utf8 str)))
      ) ;typed-lambda
    ) ;define

    (define (base64-encode x)
      (cond ((string? x) (string-base64-encode x))
            ((bytevector? x) (bytevector-base64-encode x))
            (else (type-error "input must be string or bytevector"))
      ) ;cond
    ) ;define

    (define (bytevector-base64-decode bv)
      (g_bytevector-base64-decode bv (bytevector-length bv))
    ) ;define

    (define string-base64-decode
      (typed-lambda ((str string?))
        (utf8->string (bytevector-base64-decode (string->utf8 str)))
      ) ;typed-lambda
    ) ;define

    (define (base64-decode x)
      (cond ((string? x) (string-base64-decode x))
            ((bytevector? x) (bytevector-base64-decode x))
            (else (type-error "input must be string or bytevector"))
      ) ;cond
    ) ;define
  ) ;begin
) ;define-library
