(define-library (liii base)
  (import (scheme base) (srfi srfi-2) (srfi srfi-8))
  (export and-let*
    receive
    define*
    lambda*
    procedure-source
    procedure-arglist
    arity
    defined?
    object->string
    eval-string
    signature
    copy
    keyword?
    string->keyword
    symbol->keyword
    keyword->symbol
    loose-car
    loose-cdr
    compose
    identity
    any?
    typed-lambda
    make-hook
    hook-functions
    with-output-to-string
    with-input-from-string
    call-with-input-string
    call-with-output-string
    reverse!
  ) ;export
  (begin

    (define (loose-car pair-or-empty)
      (if (eq? '() pair-or-empty) '() (car pair-or-empty))
    ) ;define

    (define (loose-cdr pair-or-empty)
      (if (eq? '() pair-or-empty) '() (cdr pair-or-empty))
    ) ;define

    (define identity (lambda (x) x))

    (define (compose . fs)
      (if (null? fs)
        (lambda (x) x)
        (lambda (x) ((car fs) ((apply compose (cdr fs)) x)))
      ) ;if
    ) ;define

    (define (any? x)
      #t
    ) ;define

    (define-macro (typed-lambda args . body)
      (if (symbol? args)
        (apply lambda args body)
        (let ((new-args (copy args)))
          (do ((p new-args (cdr p)))
            ((not (pair? p)))
            (if (pair? (car p)) (set-car! p (caar p)))
          ) ;do
          `(lambda ,new-args
             ,@(map (lambda (arg)
                      (if (pair? arg)
                        `(unless (,(cadr arg) ,(car arg))
                           (error (#_quote type-error)
                             ,"~S is not ~S~%"
                             (quote ,(car arg))
                             (quote ,(cadr arg))))
                        (values)))
                 args)
             ,@body)
        ) ;let
      ) ;if
    ) ;define-macro

  ) ;begin
) ;define-library
