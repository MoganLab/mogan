(define-library (liii error)
  (export ???
    os-error
    file-not-found-error
    not-a-directory-error
    file-exists-error
    timeout-error
    type-error
    type-error?
    key-error
    value-error
    index-error
  ) ;export
  (begin

    (define (os-error . args)
      (apply error (cons 'os-error args))
    ) ;define

    (define (file-not-found-error . args)
      (apply error
        (cons 'file-not-found-error args)
      ) ;apply
    ) ;define

    (define (not-a-directory-error . args)
      (apply error
        (cons 'not-a-directory-error args)
      ) ;apply
    ) ;define

    (define (file-exists-error . args)
      (apply error
        (cons 'file-exists-error args)
      ) ;apply
    ) ;define

    (define (timeout-error . args)
      (apply error (cons 'timeout-error args))
    ) ;define

    (define (type-error . args)
      (apply error (cons 'type-error args))
    ) ;define

    (define (type-error? err)
      (not (null? (member err
                    '(type-error wrong-type-arg)
                  ) ;member
           ) ;null?
      ) ;not
    ) ;define

    (define (key-error . args)
      (apply error (cons 'key-error args))
    ) ;define

    (define (value-error . args)
      (apply error (cons 'value-error args))
    ) ;define

    (define (index-error . args)
      (apply error (cons 'index-error args))
    ) ;define

    (define (??? . args)
      (apply error (cons '??? args))
    ) ;define
  ) ;begin
) ;define-library
