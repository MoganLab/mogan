;
; Copyright (C) 2024 The Goldfish Scheme Authors
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;

(define-library (liii check)
  (export test check check-approx check-set-mode! check:proc
    check-catch check-report check-failed?
    check-true check-false
  ) ;export
  (import (srfi srfi-78)
          (rename (srfi srfi-78)
                  (check-report srfi-78-check-report)
          ) ;rename
  ) ;import
  (begin

    (define-macro (check-true body)
      `(check ,body => #t)
    ) ;define-macro

    (define-macro (check-false body)
      `(check ,body => #f)
    ) ;define-macro

    (define default-check-approx-rel-tol 1e-12)
    (define default-check-approx-abs-tol 1e-12)

    (define (parse-check-approx-options options)
      (let loop ((remaining options)
                 (rel-tol default-check-approx-rel-tol)
                 (abs-tol default-check-approx-abs-tol))
        (cond
          ((null? remaining)
           (cons rel-tol abs-tol)
          ) ;
          ((null? (cdr remaining))
           (error "check-approx option requires a value" (car remaining))
          ) ;
          ((equal? (car remaining) :rel-tol)
           (loop (cddr remaining) (cadr remaining) abs-tol)
          ) ;
          ((equal? (car remaining) :abs-tol)
           (loop (cddr remaining) rel-tol (cadr remaining))
          ) ;
          (else
           (error "check-approx unrecognized option" (car remaining))
          ) ;else
        ) ;cond
      ) ;let
    ) ;define

    (define-macro (check-approx expr => expected . options)
      (let* ((parsed (parse-check-approx-options options))
             (rel-tol (car parsed))
             (abs-tol (cdr parsed)))
        `(check:proc ',expr
                     (lambda () ,expr)
                     ,expected
                     (lambda (actual expected)
                       (and (number? actual)
                            (number? expected)
                            (number? ,rel-tol)
                            (number? ,abs-tol)
                            (or (= actual expected)
                                (let*
                                  ((difference (abs (- actual expected)))
                                   (relative-tolerance (abs ,rel-tol))
                                   (absolute-tolerance (abs ,abs-tol))
                                   (scale (max (abs actual) (abs expected)))
                                   (limit
                                     (max absolute-tolerance
                                          (* relative-tolerance scale))))
                                  (<= difference limit)
                                ) ;let*
                            ) ;or
                       ) ;and
                     ) ;lambda
        ) ;quasiquote
      ) ;let*
    ) ;define-macro

    (define-macro (check-catch error-id body)
      `(check
        (catch ,error-id
          (lambda () ,body)
          (lambda args ,error-id))
        => ,error-id)
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
