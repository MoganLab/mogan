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

(define-library (liii date-time internal date-types)
  (import (scheme base))
  (export %make-date date? %date-year %date-month %date-day make-date date-year
    date-month date-day date-ymd leap-year? days-in-month-raw
  ) ;export
  (begin

    (define-record-type <date>
      (%make-date year month day)
      date?
      (year %date-year)
      (month %date-month)
      (day %date-day)
    ) ;define-record-type

    (define (leap-year? year)
      (unless (and (integer? year) (exact? year))
        (error 'type-error "leap-year?: year must be an exact integer" year)
      ) ;unless
      (cond
       ((not (zero? (floor-remainder year 4))) #f)
       ((not (zero? (floor-remainder year 100))) #t)
       (else (zero? (floor-remainder year 400)))
      ) ;cond
    ) ;define

    (define (days-in-month-raw is-leap-year month)
      (case month
       ((1 3 5 7 8 10 12) 31)
       ((4 6 9 11) 30)
       ((2) (if is-leap-year 29 28))
       (else #f)
      ) ;case
    ) ;define

    (define (valid-ymd? y m d)
      (and (integer? y)
        (exact? y)
        (integer? m)
        (exact? m)
        (integer? d)
        (exact? d)
        (<= 1 m 12)
        (let ((max-d (days-in-month-raw (leap-year? y) m)))
          (and max-d (<= 1 d max-d))
        ) ;let
      ) ;and
    ) ;define

    (define (make-date year month day)
      (unless (valid-ymd? year month day)
        (error 'type-error "make-date: invalid year, month or day" year month day)
      ) ;unless
      (%make-date year month day)
    ) ;define

    (define (date-year d)
      (unless (date? d)
        (error 'type-error "date-year: expected a date" d)
      ) ;unless
      (%date-year d)
    ) ;define

    (define (date-month d)
      (unless (date? d)
        (error 'type-error "date-month: expected a date" d)
      ) ;unless
      (%date-month d)
    ) ;define

    (define (date-day d)
      (unless (date? d)
        (error 'type-error "date-day: expected a date" d)
      ) ;unless
      (%date-day d)
    ) ;define

    (define (date-ymd d)
      (unless (date? d)
        (error 'type-error "date-ymd: expected a date" d)
      ) ;unless
      (values (%date-year d) (%date-month d) (%date-day d))
    ) ;define

  ) ;begin
) ;define-library
