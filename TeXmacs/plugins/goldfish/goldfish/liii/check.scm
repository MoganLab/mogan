(define-library (liii check)
  (export test
    check
    check-approx
    check-set-mode!
    check:proc
    check-catch
    check-report
    check-failed?
    check-true
    check-false
  ) ;export
  (import (srfi srfi-78)
    (rename (srfi srfi-78)
      (check-report srfi-78-check-report)
    ) ;rename
  ) ;import
  (begin

    (define-macro (check-true body)
      `(check ,body => ,#t)
    ) ;define-macro

    (define-macro (check-false body)
      `(check ,body => ,#f)
    ) ;define-macro

    (define default-check-approx-rel-tol
      1e-12
    ) ;define
    (define default-check-approx-abs-tol
      1e-12
    ) ;define

    (define (parse-check-approx-options options)
      (let loop
        ((remaining options)
         (rel-tol default-check-approx-rel-tol)
         (abs-tol default-check-approx-abs-tol)
        ) ;
        (cond ((null? remaining)
               (cons rel-tol abs-tol)
              ) ;
              ((null? (cdr remaining))
               (error "check-approx option requires a value"
                 (car remaining)
               ) ;error
              ) ;
              ((equal? (car remaining) :rel-tol)
               (loop (cddr remaining)
                 (cadr remaining)
                 abs-tol
               ) ;loop
              ) ;
              ((equal? (car remaining) :abs-tol)
               (loop (cddr remaining)
                 rel-tol
                 (cadr remaining)
               ) ;loop
              ) ;
              (else (error "check-approx unrecognized option"
                      (car remaining)
                    ) ;error
              ) ;else
        ) ;cond
      ) ;let
    ) ;define

    (define-macro (check-approx
                    expr
                    =>
                    expected
                    .
                    options
                  ) ;
      (let* ((parsed (parse-check-approx-options options)
             ) ;parsed
             (rel-tol (car parsed))
             (abs-tol (cdr parsed))
            ) ;
        `(check:proc (quote ,expr) (lambda ,() ,expr) ,expected (lambda (actual expected) (and (number? actual) (number? expected) (number? ,rel-tol) (number? ,abs-tol) (or (= actual expected) (let* ((difference (abs (- actual expected))) (relative-tolerance (abs ,rel-tol)) (absolute-tolerance (abs ,abs-tol)) (scale (max (abs actual) (abs expected))) (limit (max absolute-tolerance (* relative-tolerance scale)))) (<= difference limit))))))
      ) ;let*
    ) ;define-macro

    (define-macro (check-catch error-id body)
      `(check (catch ,error-id (lambda ,() ,body) (lambda args ,error-id)) => ,error-id)
    ) ;define-macro

    (define-macro (test left right)
      `(check ,left => ,right)
    ) ;define-macro

    (define (check-report . msg)
      (if (not (null? msg))
        (begin
          (display (car msg))
        ) ;begin
      ) ;if
      (srfi-78-check-report)
      (if (check-failed?) (exit -1))
    ) ;define
  ) ;begin
) ;define-library
