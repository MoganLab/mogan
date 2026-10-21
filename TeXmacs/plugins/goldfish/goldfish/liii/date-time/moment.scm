;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
;; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
;; License for the specific language governing permissions and limitations
;; under the License.
;;

;; Copyright (C) 2026 Peter McGoron
;; Copyright (C) 2024-2026 Arvydas Silanskas
;;
;; Permission is hereby granted, free of charge, to any person obtaining
;; a copy of this software and associated documentation files (the
;; "Software"), to deal in the Software without restriction, including
;; without limitation the rights to use, copy, modify, merge, publish,
;; distribute, sublicense, and/or sell copies of the Software, and to
;; permit persons to whom the Software is furnished to do so, subject to
;; the following conditions:
;;
;; The above copyright notice and this permission notice shall be
;; included in all copies or substantial portions of the Software.
;;
;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
;; LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
;; OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
;; WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;;

(define-library (liii date-time moment)
  (import (scheme base) (liii date-time date))
  (export make-moment moment? moment-date moment-second-of-day moment=? moment<?
    moment<=? moment>? moment>=?
  ) ;export
  (begin

    (define-record-type <moment>
      (%make-moment date second)
      moment?
      (date %moment-date)
      (second %moment-second-of-day)
    ) ;define-record-type

    (define (valid-moment? d s)
      (and (date? d) (real? s) (exact? s) (<= 0 s) (< s 86400))
    ) ;define

    (define (make-moment date second)
      (unless (valid-moment? date second)
        (error 'type-error "make-moment: invalid parameters" date second)
      ) ;unless
      (%make-moment date second)
    ) ;define

    (define (moment-date m)
      (unless (moment? m)
        (error 'type-error "moment-date: expected a moment" m)
      ) ;unless
      (%moment-date m)
    ) ;define

    (define (moment-second-of-day m)
      (unless (moment? m)
        (error 'type-error "moment-second-of-day: expected a moment" m)
      ) ;unless
      (%moment-second-of-day m)
    ) ;define

    (define (%moment-compare-impl m1 m2)
      (let ((d1 (%moment-date m1))
            (d2 (%moment-date m2))
            (s1 (%moment-second-of-day m1))
            (s2 (%moment-second-of-day m2))
           ) ;
        (cond ((date<? d1 d2) -1)
              ((date>? d1 d2) 1)
              ((< s1 s2) -1)
              ((> s1 s2) 1)
              (else 0)
        ) ;cond
      ) ;let
    ) ;define

    (define (make-transitive-comparator pred)
      (lambda (m1 m2 . rest)
        (unless (moment? m1)
          (error 'type-error "moment comparison: expected a moment" m1)
        ) ;unless
        (unless (moment? m2)
          (error 'type-error "moment comparison: expected a moment" m2)
        ) ;unless
        (let loop
          ((prev m1) (curr m2) (rem rest))
          (and (pred (%moment-compare-impl prev curr))
            (if (null? rem)
              #t
              (let ((nxt (car rem)))
                (unless (moment? nxt)
                  (error 'type-error "moment comparison: expected a moment" nxt)
                ) ;unless
                (loop curr nxt (cdr rem))
              ) ;let
            ) ;if
          ) ;and
        ) ;let
      ) ;lambda
    ) ;define

    (define moment=? (make-transitive-comparator zero?))
    (define moment<? (make-transitive-comparator negative?))
    (define moment<=? (make-transitive-comparator (lambda (c) (<= c 0))))
    (define moment>? (make-transitive-comparator positive?))
    (define moment>=? (make-transitive-comparator (lambda (c) (>= c 0))))

  ) ;begin
) ;define-library
