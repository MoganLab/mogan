(define-library (liii base)
  (import (scheme base)
    (srfi srfi-2)
    (srfi srfi-8)
  ) ;import
  (export and-let*
    receive
    define*
    procedure-source
    procedure-arglist
    arity
    defined?
    object->string
    eval-string
    signature
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
  ) ;export
  (begin

    (define (loose-car pair-or-empty)
      (if (eq? '() pair-or-empty)
        '()
        (car pair-or-empty)
      ) ;if
    ) ;define

    (define (loose-cdr pair-or-empty)
      (if (eq? '() pair-or-empty)
        '()
        (cdr pair-or-empty)
      ) ;if
    ) ;define

    (define identity (lambda (x) x))

    (define (compose . fs)
      (if (null? fs)
        (lambda (x) x)
        (lambda (x)
         ((car fs) ((apply compose (cdr fs)) x))
        ) ;lambda
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
            (if (pair? (car p))
              (set-car! p (caar p))
            ) ;if
          ) ;do
          `(lambda ,new-args ,@(map (lambda (arg) (if (pair? arg) `(unless (,(cadr arg) ,(car arg)) (error (#_quote type-error) ,"~S is not ~S~%" (quote ,(car arg)) (quote ,(cadr arg)))) (values))) args) ,@body)
        ) ;let
      ) ;if
    ) ;define-macro

  ) ;begin
) ;define-library
