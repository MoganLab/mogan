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
  (export test check check-set-mode! check:proc
    check-catch check-report check-failed?
    check-true check-false check-float
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

    (define* (check-float a b (epsilon 1e-10))
      (or (= a b)
          (< (abs (- a b)) epsilon)
      ) ;or
    ) ;define*

  ) ;begin
) ;define-library

