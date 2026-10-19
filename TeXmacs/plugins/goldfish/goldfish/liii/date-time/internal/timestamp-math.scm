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

(define-library (liii date-time internal timestamp-math)
  (import (scheme base)
    (liii date-time date)
    (liii date-time clock-time)
    (liii date-time moment)
    (liii date-time time-delta)
    (liii date-time internal timestamp-types)
  ) ;import
  (export time-delta->seconds moment+seconds timestamp+/no-rules
    adjust-date-and-time
  ) ;export
  (begin

    (define (time-delta->seconds dt)
      (+ (* (time-delta-days dt) 86400)
        (* (time-delta-hours dt) 3600)
        (* (time-delta-minutes dt) 60)
        (time-delta-seconds dt)
      ) ;+
    ) ;define

    (define (moment+seconds m s+)
      (let* ((d (moment-date m))
             (sec (+ (moment-second-of-day m) s+))
             (sec-full (floor sec))
             (sec-frac (- sec sec-full))
             (sec-int sec-full)
             (days+ (floor-quotient sec-int 86400))
             (d2 (if (zero? days+) d (date+ d (days-delta days+))))
             (rem-sec (+ (floor-remainder sec-int 86400) sec-frac))
            ) ;
        (make-moment d2 rem-sec)
      ) ;let*
    ) ;define

    (define (timestamp+/no-rules ts dt)
      (let* ((clk (%timestamp-clock-time ts))
             (leap? (>= (clock-time-second clk) 60))
             (d (%timestamp-date ts))
             (secs
               (+ (* 3600 (+ (clock-time-hour clk) (time-delta-hours dt)))
                 (* 60 (+ (clock-time-minute clk) (time-delta-minutes dt)))
                 (clock-time-second clk)
                 (time-delta-seconds dt)
               ) ;+
             ) ;secs
             (secs (if leap? (- secs 1) secs))
             (secs-full (floor secs))
             (secs-frac (- secs secs-full))
             (secs secs-full)
             (days-diff (floor-quotient secs 86400))
             (d2 (if (zero? days-diff) d (date+ d (days-delta days-diff))))
             (rem-sec (floor-remainder secs 86400))
             (h (floor-quotient rem-sec 3600))
             (rem-sec2 (floor-remainder rem-sec 3600))
             (m (floor-quotient rem-sec2 60))
             (s (+ (floor-remainder rem-sec2 60) secs-frac))
             (s (if leap? (+ s 1) s))
             (clk2 (make-clock-time h m s))
            ) ;
        (%make-timestamp d2 clk2 (%timestamp-timezone ts) 0)
      ) ;let*
    ) ;define

    (define (adjust-date-and-time ts-utc dt)
      (let* ((d (%timestamp-date ts-utc))
             (clk (%timestamp-clock-time ts-utc))
             (date-dt (time-delta+ (years-delta (time-delta-years dt))
                        (months-delta (time-delta-months dt))
                        (weeks-delta (time-delta-weeks dt))
                        (days-delta (time-delta-days dt))
                      ) ;time-delta+
             ) ;date-dt
             (d2 (date+ d date-dt))
             (old-mins (+ (* 60 (clock-time-hour clk)) (clock-time-minute clk)))
             (diff-mins (+ (* 60 (time-delta-hours dt)) (time-delta-minutes dt)))
             (total-mins (+ old-mins diff-mins))
             (days-diff (floor-quotient total-mins 1440))
             (d3 (if (zero? days-diff) d2 (date+ d2 (days-delta days-diff))))
             (rem-mins (floor-remainder total-mins 1440))
             (h (floor-quotient rem-mins 60))
             (m (floor-remainder rem-mins 60))
             (clk2 (make-clock-time h m (clock-time-second clk)))
            ) ;
        (values d3 clk2 clk)
      ) ;let*
    ) ;define

  ) ;begin
) ;define-library
