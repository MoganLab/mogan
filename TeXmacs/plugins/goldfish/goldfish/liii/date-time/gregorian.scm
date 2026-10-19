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

(define-library (liii date-time gregorian)
  (import (scheme base) (liii date-time date))
  (export leap-year? days-in-month day-name->weekday first-in-month
    last-in-month nth-in-month
  ) ;export
  (begin

    (define (days-in-month year month)
      (unless (and (integer? year) (exact? year))
        (error 'type-error "days-in-month: year must be an exact integer" year)
      ) ;unless
      (unless (and (integer? month) (exact? month) (<= 1 month 12))
        (error 'type-error
          "days-in-month: month must be an integer between 1 and 12"
          month
        ) ;error
      ) ;unless
      (days-in-month-raw (leap-year? year) month)
    ) ;define

    (define (day-name->weekday name)
      (case name
       ((monday) 1)
       ((tuesday) 2)
       ((wednesday) 3)
       ((thursday) 4)
       ((friday) 5)
       ((saturday) 6)
       ((sunday) 7)
       (else (error 'type-error "day-name->weekday: not a valid weekday symbol" name))
      ) ;case
    ) ;define

    (define (valid-weekday? wd)
      (and (integer? wd) (exact? wd) (<= 1 wd 7))
    ) ;define

    (define (first-in-month year month weekday)
      (unless (valid-weekday? weekday)
        (error 'type-error
          "first-in-month: weekday must be an integer between 1 and 7"
          weekday
        ) ;error
      ) ;unless
      (let ((first-weekday (date-iso-weekday (make-date year month 1))))
        (if (< weekday first-weekday)
          (+ 8 (- weekday first-weekday))
          (+ 1 (- weekday first-weekday))
        ) ;if
      ) ;let
    ) ;define

    (define (last-in-month year month weekday)
      (unless (valid-weekday? weekday)
        (error 'type-error
          "last-in-month: weekday must be an integer between 1 and 7"
          weekday
        ) ;error
      ) ;unless
      (let* ((last-day (days-in-month year month))
             (day-of-week (date-iso-weekday (make-date year month last-day)))
            ) ;
        (if (<= weekday day-of-week)
          (- last-day (- day-of-week weekday))
          (- last-day (- (+ day-of-week 7) weekday))
        ) ;if
      ) ;let*
    ) ;define

    (define (nth-in-month year month weekday n)
      (unless (and (integer? n) (exact? n) (>= n 1))
        (error 'type-error "nth-in-month: n must be a positive integer" n)
      ) ;unless
      (let* ((first-day (first-in-month year month weekday))
             (target-day (+ first-day (* 7 (- n 1))))
            ) ;
        (if (<= target-day (days-in-month year month)) target-day #f)
      ) ;let*
    ) ;define

  ) ;begin
) ;define-library
