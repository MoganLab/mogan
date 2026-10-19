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

(define-library (liii date-time internal timestamp-types)
  (import (scheme base)
    (liii date-time date)
    (liii date-time clock-time)
    (liii date-time internal database)
    (liii date-time internal date-rd)
  ) ;import
  (export %make-timestamp timestamp? %timestamp-date %timestamp-clock-time
    %timestamp-timezone %timestamp-fold timestamp-date timestamp-clock-time
    timestamp-timezone timestamp-fold timestamp-ymd timestamp-year
    timestamp-month timestamp-day timestamp-hms timestamp-hour timestamp-minute
    timestamp-second timestamp->local-timepoint timestamp-timezone-offset
    timestamp-timezone-abbreviation timestamp-timezone-dst?
  ) ;export
  (begin

    (define-record-type <timestamp>
      (%make-timestamp date time timezone fold)
      timestamp?
      (date %timestamp-date)
      (time %timestamp-clock-time)
      (timezone %timestamp-timezone)
      (fold %timestamp-fold)
    ) ;define-record-type

    (define (timestamp-date ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-date: expected a timestamp" ts)
      ) ;unless
      (%timestamp-date ts)
    ) ;define

    (define (timestamp-clock-time ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-clock-time: expected a timestamp" ts)
      ) ;unless
      (%timestamp-clock-time ts)
    ) ;define

    (define (timestamp-timezone ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-timezone: expected a timestamp" ts)
      ) ;unless
      (%timestamp-timezone ts)
    ) ;define

    (define (timestamp-fold ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-fold: expected a timestamp" ts)
      ) ;unless
      (%timestamp-fold ts)
    ) ;define

    (define (timestamp-ymd ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-ymd: expected a timestamp" ts)
      ) ;unless
      (date-ymd (%timestamp-date ts))
    ) ;define

    (define (timestamp-year ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-year: expected a timestamp" ts)
      ) ;unless
      (date-year (%timestamp-date ts))
    ) ;define

    (define (timestamp-month ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-month: expected a timestamp" ts)
      ) ;unless
      (date-month (%timestamp-date ts))
    ) ;define

    (define (timestamp-day ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-day: expected a timestamp" ts)
      ) ;unless
      (date-day (%timestamp-date ts))
    ) ;define

    (define (timestamp-hms ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-hms: expected a timestamp" ts)
      ) ;unless
      (clock-time-hms (%timestamp-clock-time ts))
    ) ;define

    (define (timestamp-hour ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-hour: expected a timestamp" ts)
      ) ;unless
      (clock-time-hour (%timestamp-clock-time ts))
    ) ;define

    (define (timestamp-minute ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-minute: expected a timestamp" ts)
      ) ;unless
      (clock-time-minute (%timestamp-clock-time ts))
    ) ;define

    (define (timestamp-second ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-second: expected a timestamp" ts)
      ) ;unless
      (clock-time-second (%timestamp-clock-time ts))
    ) ;define

    (define (timestamp->local-timepoint ts)
      (let* ((days (- (date->rata-die (%timestamp-date ts)) unix-epoch-rd))
             (clk (%timestamp-clock-time ts))
            ) ;
        (+ (* days 86400)
          (* (clock-time-hour clk) 3600)
          (* (clock-time-minute clk) 60)
          (clock-time-second clk)
        ) ;+
      ) ;let*
    ) ;define

    (define (%timestamp-time-range ts)
      (let* ((local-tp (timestamp->local-timepoint ts))
             (tz (%timestamp-timezone ts))
             (fold (%timestamp-fold ts))
             (offsets-vec (find-offset/wall tz local-tp))
            ) ;
        (vector-ref offsets-vec (min fold (- (vector-length offsets-vec) 1)))
      ) ;let*
    ) ;define

    (define (timestamp-timezone-offset ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-timezone-offset: expected a timestamp" ts)
      ) ;unless
      (time-range-offset (%timestamp-time-range ts))
    ) ;define

    (define (timestamp-timezone-abbreviation ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-timezone-abbreviation: expected a timestamp" ts)
      ) ;unless
      (time-range-designation (%timestamp-time-range ts))
    ) ;define

    (define (timestamp-timezone-dst? ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp-timezone-dst?: expected a timestamp" ts)
      ) ;unless
      (time-range-dst? (%timestamp-time-range ts))
    ) ;define

  ) ;begin
) ;define-library
