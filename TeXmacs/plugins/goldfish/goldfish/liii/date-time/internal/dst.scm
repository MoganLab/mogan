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

(define-library (liii date-time internal dst)
  (import (scheme base)
    (liii date-time date)
    (liii date-time gregorian)
    (liii date-time time-delta)
    (liii date-time internal util)
    (liii date-time internal tz)
  ) ;import
  (export tzrule tzrule? tzrule-std-name tzrule-std-offset tzrule-dst-transition
    tzrule-dst-name tzrule-dst-offset tzrule-std-transition add-offset
    transition-unix in-offset find-an-offset-rule offset-rule
  ) ;export
  (begin

    (define-record-type <tzrule>
      (tzrule std-name std-offset dst-transition dst-name dst-offset std-transition)
      tzrule?
      (std-name tzrule-std-name)
      (std-offset tzrule-std-offset)
      (dst-transition tzrule-dst-transition)
      (dst-name tzrule-dst-name)
      (dst-offset tzrule-dst-offset)
      (std-transition tzrule-std-transition)
    ) ;define-record-type

    (define epoch (date->rata-die (make-date 1970 1 1)))

    (define (month-week-day->rd year month week weekday)
      (let* ((wd (if (zero? weekday) 7 weekday))
             (day (if (= week 5) (last-in-month year month wd) (nth-in-month year month wd week))
             ) ;day
            ) ;
        (date->rata-die (make-date year month day))
      ) ;let*
    ) ;define

    (define (zero-based-day->rd year zero-based-day)
      (+ (date->rata-die (make-date year 1 1)) zero-based-day)
    ) ;define

    (define (one-based-day->rd year one-based-day)
      (if (<= one-based-day 59)
        (+ (date->rata-die (make-date year 1 1)) (- one-based-day 1))
        (+ (date->rata-die (make-date year 3 1)) (- one-based-day 60))
      ) ;if
    ) ;define

    (define (tz-offset->seconds offset)
      (* (if (tz-offset-sign-negative? offset) -1 1)
        (+ (* (tz-offset-hour offset) 3600)
          (* (tz-offset-minute offset) 60)
          (tz-offset-second offset)
        ) ;+
      ) ;*
    ) ;define

    (define (transition-local year transition)
      (let* ((date (tz-transition-date transition))
             (time (tz-transition-time transition))
             (rd
               (cond ((tz-julian-with-leap-day? date)
                      (zero-based-day->rd year (tz-julian-with-leap-day:date date))
                     ) ;
                     ((tz-julian-without-leap-day? date)
                      (one-based-day->rd year (tz-julian-without-leap-day:date date))
                     ) ;
                     ((tz-month-week-day? date)
                      (month-week-day->rd year
                        (tz-month-week-day:month date)
                        (tz-month-week-day:week date)
                        (tz-month-week-day:day date)
                      ) ;month-week-day->rd
                     ) ;
                     (else (error 'type-error "invalid date in transition" date))
               ) ;cond
             ) ;rd
             (rd (- rd epoch))
             (time-seconds (tz-offset->seconds time))
            ) ;
        (+ (* rd 86400) time-seconds)
      ) ;let*
    ) ;define

    (define (add-offset unix offset)
      (+ unix
        (* (time-delta-hours offset) 3600)
        (* (time-delta-minutes offset) 60)
        (time-delta-seconds offset)
      ) ;+
    ) ;define

    (define (transition-unix offset year transition)
      (let ((local (transition-local year transition)))
        (- local
          (* (time-delta-hours offset) 3600)
          (* (time-delta-minutes offset) 60)
          (time-delta-seconds offset)
        ) ;-
      ) ;let
    ) ;define

    (define (in-offset time year subindex dst->std std->dst dst-offset std-offset)
      (let-values (((year+ new-subindex) (truncate/ subindex 2)))
        (let ((year (+ year year+)))
          (case subindex
           ((-1)
            (cond
             ((< time (dst->std (- year 1) std-offset)) -1)
             ((< time (dst->std (- year 1) dst-offset)) #f)
             ((<= (std->dst year std-offset) time) 1)
             (else 0)
            ) ;cond
           ) ;
           ((0)
            (cond ((< time (std->dst year std-offset)) -1)
                  ((< time (std->dst year dst-offset)) #f)
                  ((<= (dst->std year dst-offset) time) 1)
                  (else 0)
            ) ;cond
           ) ;
           ((1)
            (cond ((< time (dst->std year dst-offset)) -1)
                  ((< time (dst->std year std-offset)) #f)
                  ((<= (std->dst (+ year 1) std-offset) time) 1)
                  (else 0)
            ) ;cond
           ) ;
           (else (error 'type-error "invalid subindex" subindex))
          ) ;case
        ) ;let
      ) ;let-values
    ) ;define

    (define (calculate-year time)
      (let-values (((days-since-1970 rem) (floor/ time 86400)))
        (date-year (rata-die->date (+ days-since-1970 epoch)))
      ) ;let-values
    ) ;define

    (define (find-an-offset-rule local-time dst->std std->dst dst-offset std-offset)
      (let loop
        ((year (calculate-year local-time)) (subindex 0))
        (case (in-offset local-time year subindex dst->std std->dst dst-offset std-offset)
              ((0) (values year subindex))
              ((-1)
               (case subindex
                ((0) (loop year -1))
                ((-1) (loop (- year 1) 1))
                ((1) (loop year 0))
               ) ;case
              ) ;
              ((1)
               (case subindex
                ((0) (loop year 1))
                ((-1) (loop year 0))
                ((1) (loop (+ year 1) -1))
               ) ;case
              ) ;
              ((#f) (values year subindex))
              (else (error 'type-error "invalid adjustment in find-an-offset-rule"))
        ) ;case
      ) ;let
    ) ;define

    (define (offset-rule make-tr time year subindex dst->std std->dst dst-offset
              std-offset dst-designation std-designation
            ) ;offset-rule
      (let-values (((year+ subindex) (truncate/ subindex 2)))
        (let* ((year (+ year year+))
               (change (in-offset time year subindex dst->std std->dst dst-offset std-offset))
              ) ;
          (if (eqv? change 0)
            (vector
              (case subindex
               ((0)
                (make-tr (std->dst year dst-offset)
                  (dst->std year dst-offset)
                  dst-offset
                  #t
                  dst-designation
                ) ;make-tr
               ) ;
               ((1)
                (make-tr (dst->std year std-offset)
                  (std->dst (+ year 1) std-offset)
                  std-offset
                  #f
                  std-designation
                ) ;make-tr
               ) ;
               ((-1)
                (make-tr (dst->std (- year 1) std-offset)
                  (std->dst year std-offset)
                  std-offset
                  #f
                  std-designation
                ) ;make-tr
               ) ;
              ) ;case
            ) ;vector
            #()
          ) ;if
        ) ;let*
      ) ;let-values
    ) ;define

  ) ;begin
) ;define-library
