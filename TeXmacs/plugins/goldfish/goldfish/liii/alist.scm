(define-library (liii alist)
  (import (liii base)
    (liii list)
    (liii error)
    (scheme case-lambda)
  ) ;import
  (export alist?
    alist-cons
    alist-ref
    alist-ref/default
    vector->alist
  ) ;export
  (begin

    (define (alist? l)
      (and (list? l) (every pair? l))
    ) ;define

    (define alist-ref
      (case-lambda
       ((alist key)
        (alist-ref alist
          key
          (lambda ()
            (key-error "alist-ref: key not found "
              key
            ) ;key-error
          ) ;lambda
        ) ;alist-ref
       ) ;
       ((alist key thunk)
        (alist-ref alist key thunk eqv?)
       ) ;
       ((alist key thunk =)
        (let ((value (assoc key alist =)))
          (if value (cdr value) (thunk))
        ) ;let
       ) ;
      ) ;case-lambda
    ) ;define

    (define alist-ref/default
      (case-lambda
       ((alist key default)
        (alist-ref alist
          key
          (lambda () default)
        ) ;alist-ref
       ) ;
       ((alist key default =)
        (alist-ref alist
          key
          (lambda () default)
          =
        ) ;alist-ref
       ) ;
      ) ;case-lambda
    ) ;define

    (define vector->alist
      (typed-lambda ((x vector?))
        (if (zero? (length x))
          '()
          (let loop
            ((x (vector->list x)) (n 0))
            (cons (cons n (car x))
              (if (null? (cdr x))
                '()
                (loop (cdr x) (+ n 1))
              ) ;if
            ) ;cons
          ) ;let
        ) ;if
      ) ;typed-lambda
    ) ;define
  ) ;begin
) ;define-library
