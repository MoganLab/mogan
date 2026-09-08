(define-library (liii base64)
  (import (scheme base) (liii base) (liii bitwise) (liii error))
  (export string-base64-encode bytevector-base64-encode base64-encode
    string-base64-decode bytevector-base64-decode base64-decode
  ) ;export
  (begin
    (define (bytevector-base64-encode bv)
      (when (not (bytevector? bv))
        (type-error "bytevector-base64-encode: input must be bytevector" bv)
      ) ;when
      (g_bytevector-base64-encode bv)
    ) ;define

    (define (string-base64-encode str)
      (when (not (string? str))
        (type-error "string-base64-encode: input must be string" str)
      ) ;when
      (utf8->string (bytevector-base64-encode (string->utf8 str)))
    ) ;define

    (define (base64-encode x)
      (cond ((string? x) (string-base64-encode x))
            ((bytevector? x) (bytevector-base64-encode x))
            (else (type-error "input must be string or bytevector" x))
      ) ;cond
    ) ;define

    (define (bytevector-base64-decode bv)
      (when (not (bytevector? bv))
        (type-error "bytevector-base64-decode: input must be bytevector" bv)
      ) ;when
      (g_bytevector-base64-decode bv)
    ) ;define

    (define (string-base64-decode str)
      (when (not (string? str))
        (type-error "string-base64-decode: input must be string" str)
      ) ;when
      (utf8->string (bytevector-base64-decode (string->utf8 str)))
    ) ;define

    (define (base64-decode x)
      (cond ((string? x) (string-base64-decode x))
            ((bytevector? x) (bytevector-base64-decode x))
            (else (type-error "input must be string or bytevector" x))
      ) ;cond
    ) ;define
  ) ;begin
) ;define-library
