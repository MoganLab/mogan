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

(define-library (liii date-time internal timestamp-conv)
  (import (scheme base)
    (scheme case-lambda)
    (liii date-time date)
    (liii date-time clock-time)
    (liii date-time moment)
    (liii date-time time-delta)
    (liii date-time timezone)
    (liii date-time internal leap-seconds)
    (liii date-time internal database)
    (liii date-time internal date-rd)
    (liii date-time internal timestamp-types)
    (liii date-time internal timestamp-math)
    (liii date-time internal timestamp-construct)
  ) ;import
  (export timestamp+ timestamp- timestamp-in-timezone moment->timestamp
    timestamp->moment posix-time->utc-timestamp timestamp->utc-posix-time
  ) ;export
  (begin

    (define (timestamp-in-timezone ts tz)
      (unless (and (timestamp? ts) (timezone? tz))
        (error 'type-error "timestamp-in-timezone: invalid parameters" ts tz)
      ) ;unless
      (if (eqv? tz (%timestamp-timezone ts))
        ts
        (let* ((utc-ts (timestamp-in-utc ts))
               (tp (timestamp->local-timepoint utc-ts))
               (range (find-offset/utc tz tp))
               (offset (time-range-offset range))
               (local-ts (ts-no-rules utc-ts offset))
               (base-ts (date+clock-time->timestamp (%timestamp-date local-ts)
                          (%timestamp-clock-time local-ts)
                          tz
                        ) ;date+clock-time->timestamp
               ) ;base-ts
               (offsets-vec (find-offset/wall tz (timestamp->local-timepoint base-ts)))
               (fold?
                 (and (> (vector-length offsets-vec) 1)
                   (= (time-delta->seconds offset)
                     (time-delta->seconds (time-range-offset (vector-ref offsets-vec 1)))
                   ) ;=
                 ) ;and
               ) ;fold?
              ) ;
          (if fold?
            (date+clock-time->timestamp (%timestamp-date base-ts)
              (%timestamp-clock-time base-ts)
              tz
              1
            ) ;date+clock-time->timestamp
            base-ts
          ) ;if
        ) ;let*
      ) ;if
    ) ;define

    (define (moment->timestamp m tz)
      (unless (and (moment? m) (timezone? tz))
        (error 'type-error "moment->timestamp: invalid parameters" m tz)
      ) ;unless
      (let* ((d (moment-date m))
             (secs (moment-second-of-day m))
             (days (- (date->rata-die d) unix-epoch-rd))
             (tp (+ (* days 86400) secs))
            ) ;
        (let-values (((leap? off) (leapsecond-info/tai tp)))
          (let* ((secs (- secs off))
                 (secs-full (floor secs))
                 (secs-frac (- secs secs-full))
                 (secs secs-full)
                 (date-diff (floor-quotient secs 86400))
                 (d2 (if (zero? date-diff) d (date+ d (days-delta date-diff))))
                 (rem-sec (floor-remainder secs 86400))
                 (h (floor-quotient rem-sec 3600))
                 (rem-sec2 (floor-remainder rem-sec 3600))
                 (min (floor-quotient rem-sec2 60))
                 (s (+ (floor-remainder rem-sec2 60) secs-frac))
                 (s (if leap? (+ s 1) s))
                 (clk (make-clock-time h min s))
                 (utc-ts (date+clock-time->timestamp d2 clk (utc-timezone)))
                ) ;
            (timestamp-in-timezone utc-ts tz)
          ) ;let*
        ) ;let-values
      ) ;let*
    ) ;define

    (define (timestamp->moment ts)
      (let* ((utc-ts (timestamp-in-utc ts))
             (tp (floor (timestamp->local-timepoint utc-ts)))
            ) ;
        (let-values (((leap? off) (leapsecond-info/utc tp)))
          (let* ((clk (%timestamp-clock-time utc-ts))
                 (h (clock-time-hour clk))
                 (m (clock-time-minute clk))
                 (s (clock-time-second clk))
                 (total-secs (+ (* h 3600) (* m 60) s off))
                 (secs-full (floor total-secs))
                 (secs-frac (- total-secs secs-full))
                 (secs-int secs-full)
                 (d (%timestamp-date utc-ts))
                 (date-diff (floor-quotient secs-int 86400))
                 (d2 (if (zero? date-diff) d (date+ d (days-delta date-diff))))
                 (sec-of-day (+ (floor-remainder secs-int 86400) secs-frac))
                ) ;
            (make-moment d2 sec-of-day)
          ) ;let*
        ) ;let-values
      ) ;let*
    ) ;define

    (define (timestamp->utc-posix-time ts)
      (let* ((utc-ts (timestamp-in-timezone ts (utc-timezone)))
             (leap? (>= (clock-time-second (%timestamp-clock-time utc-ts)) 60))
             (tp (timestamp->local-timepoint utc-ts))
            ) ;
        (if leap? (- tp 1) tp)
      ) ;let*
    ) ;define

    (define posix-time->utc-timestamp
      (case-lambda
       ((seconds)
        (unless (and (exact? seconds) (rational? seconds))
          (error 'type-error
            "posix-time->utc-timestamp: expected an exact rational"
            seconds
          ) ;error
        ) ;unless
        (let* ((days (floor-quotient seconds 86400))
               (d (rata-die->date (+ unix-epoch-rd days)))
               (rem-sec (floor-remainder seconds 86400))
               (h (floor-quotient rem-sec 3600))
               (rem-sec2 (floor-remainder rem-sec 3600))
               (m (floor-quotient rem-sec2 60))
               (s (floor-remainder rem-sec2 60))
               (clk (make-clock-time h m s))
              ) ;
          (date+clock-time->timestamp d clk (utc-timezone))
        ) ;let*
       ) ;
       ((seconds nano-seconds)
        (unless (and (integer? seconds)
                  (exact? seconds)
                  (integer? nano-seconds)
                  (exact? nano-seconds)
                ) ;and
          (error 'type-error
            "posix-time->utc-timestamp: expected exact integers"
            seconds
            nano-seconds
          ) ;error
        ) ;unless
        (posix-time->utc-timestamp (+ seconds (/ nano-seconds 1000000000)))
       ) ;
      ) ;case-lambda
    ) ;define

    (define (timestamp+ ts dt)
      (unless (and (timestamp? ts) (time-delta? dt))
        (error 'type-error "timestamp+: expected timestamp and time-delta" ts dt)
      ) ;unless
      (let* ((tz (%timestamp-timezone ts)) (ts-utc (timestamp-in-utc ts)))
        (let-values (((d3 clk2 clk) (adjust-date-and-time ts-utc dt)))
          (let* ((t (%make-timestamp d3 clk2 (utc-timezone) 0))
                 (ts2
                   (validate-leapsecond t
                     (lambda () t)
                     (lambda ()
                       (%make-timestamp d3
                         (make-clock-time (clock-time-hour clk2)
                           (clock-time-minute clk2)
                           (- (clock-time-second clk) 1)
                         ) ;make-clock-time
                         (utc-timezone)
                         0
                       ) ;%make-timestamp
                     ) ;lambda
                   ) ;validate-leapsecond
                 ) ;ts2
                 (ts3
                   (if (zero? (time-delta-seconds dt))
                     ts2
                     (moment->timestamp (moment+seconds (timestamp->moment ts2) (time-delta-seconds dt))
                       (utc-timezone)
                     ) ;moment->timestamp
                   ) ;if
                 ) ;ts3
                ) ;
            (timestamp-in-timezone ts3 tz)
          ) ;let*
        ) ;let-values
      ) ;let*
    ) ;define

    (define (timestamp- ts dt)
      (timestamp+ ts (time-delta-negate dt))
    ) ;define

  ) ;begin
) ;define-library
