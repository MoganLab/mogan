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

(define-library (liii date-time internal date-rd)
  (import (scheme base) (liii date-time internal date-types))
  (export unix-epoch-rd days-since-year-start calc-date->rata-die
    calc-rata-die->date calc-date->mjd calc-mjd->date
  ) ;export
  (begin

    (define days/4y (+ (* 4 365) 1))
    (define days/100y (+ (* 100 365) 24))
    (define days/400y (+ (* 4 days/100y) 1))
    (define mjd-rd-offset (- 2400000 1721424))

    (define (days-since-year-start* year month day is-leap)
      (let loop
        ((days 0) (i 1))
        (if (= i month)
          (+ days day)
          (loop (+ days (days-in-month-raw is-leap i)) (+ 1 i))
        ) ;if
      ) ;let
    ) ;define

    (define (days-since-year-start d)
      (let-values (((year month day) (date-ymd d)))
        (days-since-year-start* year month day (leap-year? year))
      ) ;let-values
    ) ;define

    (define (apply-days-after-year-start year days)
      (let ((is-leap (leap-year? year)))
        (let loop
          ((days days) (m 1))
          (let ((max-d (days-in-month-raw is-leap m)))
            (if (> days max-d) (loop (- days max-d) (+ 1 m)) (make-date year m days))
          ) ;let
        ) ;let
      ) ;let
    ) ;define

    (define (rd-for-year year)
      (let* ((curr-year-adjust (if (leap-year? year) -366 -365))
             (part400s (* days/400y (floor-quotient year 400)))
             (y (floor-remainder year 400))
             (part100s (* days/100y (floor-quotient y 100)))
             (y (floor-remainder y 100))
             (part4s (* days/4y (floor-quotient y 4)))
             (y (floor-remainder y 4))
             (rest (* y 365))
            ) ;
        (+ part400s part100s part4s rest curr-year-adjust)
      ) ;let*
    ) ;define

    (define (date->rata-die* year month day)
      (+ (rd-for-year year) (days-since-year-start* year month day (leap-year? year)))
    ) ;define

    (define (calc-date->rata-die d)
      (unless (date? d)
        (error 'type-error "date->rata-die: expected a date" d)
      ) ;unless
      (let-values (((year m day) (date-ymd d)))
        (if (>= year 0)
          (date->rata-die* year m day)
          (let* ((n (+ 1 (floor-quotient (abs year) 400)))
                 (rd1 (date->rata-die* (+ year (* n 400)) m day))
                 (rd2 (date->rata-die* (+ 1 (* n 400)) 1 1))
                ) ;
            (+ 1 (- rd1 rd2))
          ) ;let*
        ) ;if
      ) ;let-values
    ) ;define

    (define unix-epoch-rd (calc-date->rata-die (make-date 1970 1 1)))

    (define (rata-die->date* rd)
      (let* ((rd (- rd 1))
             (part400s (* 400 (floor-quotient rd days/400y)))
             (rd (floor-remainder rd days/400y))
             (part100s (* 100 (min (floor-quotient rd days/100y) 3)))
             (rd
               (- rd (* (min (floor-quotient rd days/100y) 3) days/100y))
             ) ;rd
             (part4s (* 4 (min (floor-quotient rd days/4y) 24)))
             (rd
               (- rd (* (min (floor-quotient rd days/4y) 24) days/4y))
             ) ;rd
             (rest (min (floor-quotient rd 365) 3))
             (rd (- rd (* rest 365)))
             (year (+ part400s part100s part4s rest))
            ) ;
        (apply-days-after-year-start (+ 1 year) (+ 1 rd))
      ) ;let*
    ) ;define

    (define (calc-rata-die->date rd)
      (unless (and (integer? rd) (exact? rd))
        (error 'type-error "rata-die->date: expected an exact integer" rd)
      ) ;unless
      (if (>= rd 1)
        (rata-die->date* rd)
        (let* ((n (+ 1 (floor-quotient (abs rd) days/400y)))
               (d* (rata-die->date* (+ (* n days/400y) rd)))
              ) ;
          (let-values (((y m day) (date-ymd d*)))
            (%make-date (- y (* n 400)) m day)
          ) ;let-values
        ) ;let*
      ) ;if
    ) ;define

    (define (calc-date->mjd d)
      (unless (date? d)
        (error 'type-error "date->mjd: expected a date" d)
      ) ;unless
      (- (calc-date->rata-die d) mjd-rd-offset)
    ) ;define

    (define (calc-mjd->date mjd)
      (unless (and (integer? mjd) (exact? mjd))
        (error 'type-error "mjd->date: expected an exact integer" mjd)
      ) ;unless
      (calc-rata-die->date (+ mjd-rd-offset mjd))
    ) ;define

  ) ;begin
) ;define-library
