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

(define-library (scheme time)
  (import (only (scheme base) let-values s7-round))
  (export current-second current-jiffy jiffies-per-second
          get-time-of-day monotonic-nanosecond
          system-clock-resolution steady-clock-resolution)
  (begin

    (define (jiffies-per-second) 1000000)

    (define get-time-of-day g_get-time-of-day)
    (define monotonic-nanosecond g_monotonic-nanosecond)
    (define system-clock-resolution g_system-clock-resolution)
    (define steady-clock-resolution g_steady-clock-resolution)

    (define (current-second)
      (let-values (((sec usec) (get-time-of-day)))
        (+ sec (exact->inexact (/ usec 1000000)))))

    (define (current-jiffy)
      ;; NOTE: use the s7-round to ensure that a natural number is returned.
      (s7-round (* (current-second) (jiffies-per-second))))

    ) ; end of begin
  ) ; end of define-library
