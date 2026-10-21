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

;; Copyright (C) 2024-2026 Arvydas Silanskas
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

(define-library (liii date-time time-delta)
  (import (scheme base)
    (liii date-time date)
    (liii date-time internal date-arithmetic)
  ) ;import
  (export make-time-delta time-delta? time-delta-years time-delta-months
    time-delta-weeks time-delta-days time-delta-hours time-delta-minutes
    time-delta-seconds years-delta months-delta weeks-delta days-delta
    hours-delta minutes-delta seconds-delta time-delta+ time-delta-
    time-delta-negate date+ date-
  ) ;export
  (begin

    (define-record-type <time-delta>
      (%make-time-delta years months weeks days hours minutes seconds)
      time-delta?
      (years %time-delta-years)
      (months %time-delta-months)
      (weeks %time-delta-weeks)
      (days %time-delta-days)
      (hours %time-delta-hours)
      (minutes %time-delta-minutes)
      (seconds %time-delta-seconds)
    ) ;define-record-type

    (define (valid-delta-components? y m w d h min sec)
      (and (integer? y)
        (exact? y)
        (integer? m)
        (exact? m)
        (integer? w)
        (exact? w)
        (integer? d)
        (exact? d)
        (integer? h)
        (exact? h)
        (integer? min)
        (exact? min)
        (rational? sec)
        (exact? sec)
      ) ;and
    ) ;define

    (define (make-time-delta years months weeks days hours minutes seconds)
      (unless (valid-delta-components? years months weeks days hours minutes seconds)
        (error 'type-error
          "make-time-delta: invalid parameters"
          years
          months
          weeks
          days
          hours
          minutes
          seconds
        ) ;error
      ) ;unless
      (%make-time-delta years months weeks days hours minutes seconds)
    ) ;define

    (define (time-delta-years dt)
      (unless (time-delta? dt)
        (error 'type-error "time-delta-years: expected a time-delta" dt)
      ) ;unless
      (%time-delta-years dt)
    ) ;define

    (define (time-delta-months dt)
      (unless (time-delta? dt)
        (error 'type-error "time-delta-months: expected a time-delta" dt)
      ) ;unless
      (%time-delta-months dt)
    ) ;define

    (define (time-delta-weeks dt)
      (unless (time-delta? dt)
        (error 'type-error "time-delta-weeks: expected a time-delta" dt)
      ) ;unless
      (%time-delta-weeks dt)
    ) ;define

    (define (time-delta-days dt)
      (unless (time-delta? dt)
        (error 'type-error "time-delta-days: expected a time-delta" dt)
      ) ;unless
      (%time-delta-days dt)
    ) ;define

    (define (time-delta-hours dt)
      (unless (time-delta? dt)
        (error 'type-error "time-delta-hours: expected a time-delta" dt)
      ) ;unless
      (%time-delta-hours dt)
    ) ;define

    (define (time-delta-minutes dt)
      (unless (time-delta? dt)
        (error 'type-error "time-delta-minutes: expected a time-delta" dt)
      ) ;unless
      (%time-delta-minutes dt)
    ) ;define

    (define (time-delta-seconds dt)
      (unless (time-delta? dt)
        (error 'type-error "time-delta-seconds: expected a time-delta" dt)
      ) ;unless
      (%time-delta-seconds dt)
    ) ;define

    (define (years-delta v)
      (unless (and (integer? v) (exact? v))
        (error 'type-error "years-delta: expected an exact integer" v)
      ) ;unless
      (%make-time-delta v 0 0 0 0 0 0)
    ) ;define

    (define (months-delta v)
      (unless (and (integer? v) (exact? v))
        (error 'type-error "months-delta: expected an exact integer" v)
      ) ;unless
      (%make-time-delta 0 v 0 0 0 0 0)
    ) ;define

    (define (weeks-delta v)
      (unless (and (integer? v) (exact? v))
        (error 'type-error "weeks-delta: expected an exact integer" v)
      ) ;unless
      (%make-time-delta 0 0 v 0 0 0 0)
    ) ;define

    (define (days-delta v)
      (unless (and (integer? v) (exact? v))
        (error 'type-error "days-delta: expected an exact integer" v)
      ) ;unless
      (%make-time-delta 0 0 0 v 0 0 0)
    ) ;define

    (define (hours-delta v)
      (unless (and (integer? v) (exact? v))
        (error 'type-error "hours-delta: expected an exact integer" v)
      ) ;unless
      (%make-time-delta 0 0 0 0 v 0 0)
    ) ;define

    (define (minutes-delta v)
      (unless (and (integer? v) (exact? v))
        (error 'type-error "minutes-delta: expected an exact integer" v)
      ) ;unless
      (%make-time-delta 0 0 0 0 0 v 0)
    ) ;define

    (define (seconds-delta v)
      (unless (and (rational? v) (exact? v))
        (error 'type-error "seconds-delta: expected an exact number" v)
      ) ;unless
      (%make-time-delta 0 0 0 0 0 0 v)
    ) ;define

    (define (time-delta+ . dts)
      (for-each (lambda (dt)
                  (unless (time-delta? dt)
                    (error 'type-error "time-delta+: expected a time-delta" dt)
                  ) ;unless
                ) ;lambda
        dts
      ) ;for-each
      (let loop
        ((rem dts) (y 0) (m 0) (w 0) (d 0) (h 0) (min 0) (s 0))
        (if (null? rem)
          (%make-time-delta y m w d h min s)
          (let ((curr (car rem)))
            (loop (cdr rem)
              (+ y (%time-delta-years curr))
              (+ m (%time-delta-months curr))
              (+ w (%time-delta-weeks curr))
              (+ d (%time-delta-days curr))
              (+ h (%time-delta-hours curr))
              (+ min (%time-delta-minutes curr))
              (+ s (%time-delta-seconds curr))
            ) ;loop
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (time-delta-negate dt)
      (unless (time-delta? dt)
        (error 'type-error "time-delta-negate: expected a time-delta" dt)
      ) ;unless
      (%make-time-delta (- (%time-delta-years dt))
        (- (%time-delta-months dt))
        (- (%time-delta-weeks dt))
        (- (%time-delta-days dt))
        (- (%time-delta-hours dt))
        (- (%time-delta-minutes dt))
        (- (%time-delta-seconds dt))
      ) ;%make-time-delta
    ) ;define

    (define (time-delta- dt1 . rest)
      (unless (time-delta? dt1)
        (error 'type-error "time-delta-: expected a time-delta" dt1)
      ) ;unless
      (if (null? rest)
        (time-delta-negate dt1)
        (let ((sub (apply time-delta+ rest)))
          (time-delta+ dt1 (time-delta-negate sub))
        ) ;let
      ) ;if
    ) ;define

    (define (date+ date dt)
      (unless (date? date)
        (error 'type-error "date+: expected a date" date)
      ) ;unless
      (unless (time-delta? dt)
        (error 'type-error "date+: expected a time-delta" dt)
      ) ;unless
      (unless (and (zero? (%time-delta-hours dt))
                (zero? (%time-delta-minutes dt))
                (zero? (%time-delta-seconds dt))
              ) ;and
        (error 'type-error "date+: time-delta cannot have hours, minutes or seconds" dt)
      ) ;unless
      (date-add-components date
        (%time-delta-years dt)
        (%time-delta-months dt)
        (%time-delta-weeks dt)
        (%time-delta-days dt)
      ) ;date-add-components
    ) ;define

    (define (date- date dt)
      (unless (date? date)
        (error 'type-error "date-: expected a date" date)
      ) ;unless
      (unless (time-delta? dt)
        (error 'type-error "date-: expected a time-delta" dt)
      ) ;unless
      (date+ date (time-delta-negate dt))
    ) ;define

  ) ;begin
) ;define-library
