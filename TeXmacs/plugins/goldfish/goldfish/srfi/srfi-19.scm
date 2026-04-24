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

;;
;; SRFI-19 Implementation for Goldfish Scheme
;;
;; This is a heavily modified implementation of SRFI-19 "Time Data Types
;; and Procedures". While based on the original reference implementation,
;; nearly every function has been rewritten for performance, clarity, or
;; to adapt to Goldfish Scheme's idioms.
;;
;; ======================================================================
;; SRFI-19: Time Data Types and Procedures.
;;
;; Copyright (C) I/NET, Inc. (2000, 2002, 2003). All Rights Reserved.
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

(define-library (srfi srfi-19)
  (import (rename (scheme time)
            (get-time-of-day glue:get-time-of-day)
            (monotonic-nanosecond glue:monotonic-nanosecond
            ) ;monotonic-nanosecond
          ) ;rename
    (only (srfi srfi-13)
      string-pad
      string-tokenize
      string-trim-right
    ) ;only
    (only (srfi srfi-8) receive)
    (only (scheme base)
      open-output-string
      open-input-string
      get-output-string
      floor/
    ) ;only
    (liii error)
  ) ;import
  (export
    ;; Constants
    TIME-DURATION
    TIME-MONOTONIC
    TIME-PROCESS
    TIME-TAI
    TIME-THREAD
    TIME-UTC
    ;; Time object and accessors
    make-time
    time?
    time-type
    time-nanosecond
    time-second
    set-time-type!
    set-time-nanosecond!
    set-time-second!
    copy-time
    ;; Time comparison procedures
    time<=?
    time<?
    time=?
    time>=?
    time>?
    ;; Time arithmetic procedures
    add-duration
    subtract-duration
    time-difference
    ;; Current time and clock resolution
    current-date
    current-julian-day
    current-time
    time-resolution
    local-tz-offset
    ;; Date object and accessors
    make-date
    date?
    date-nanosecond
    date-second
    date-minute
    date-hour
    date-day
    date-month
    date-year
    date-zone-offset
    date-year-day
    date-week-day
    date-week-number
    ;; Time/Date/Julian Day/Modified Julian Day Converters
    time-utc->time-tai
    time-tai->time-utc
    time-utc->time-monotonic
    time-monotonic->time-utc
    time-tai->time-monotonic
    time-monotonic->time-tai
    time-utc->date
    date->time-utc
    time-tai->date
    date->time-tai
    time-monotonic->date
    date->time-monotonic
    date->julian-day
    date->modified-julian-day
    ;; Date to String/String to Date Converters
    date->string
    string->date
  ) ;export
  (begin

    ;; ====================
    ;; Constants
    ;; ====================

    (define TIME-DURATION 'time-duration)
    (define TIME-MONOTONIC 'time-monotonic)
    (define TIME-PROCESS 'time-process)
    (define TIME-TAI 'time-tai)
    (define TIME-THREAD 'time-thread)
    (define TIME-UTC 'time-utc)

    (define priv:LOCALE-DECIMAL-POINT ".")

    (define priv:LOCALE-ABBR-WEEKDAY-VECTOR
      (vector "Sun"
        "Mon"
        "Tue"
        "Wed"
        "Thu"
        "Fri"
        "Sat"
      ) ;vector
    ) ;define
    (define priv:LOCALE-LONG-WEEKDAY-VECTOR
      (vector "Sunday"
        "Monday"
        "Tuesday"
        "Wednesday"
        "Thursday"
        "Friday"
        "Saturday"
      ) ;vector
    ) ;define
    ;; note empty string in 0th place.
    (define priv:LOCALE-ABBR-MONTH-VECTOR
      (vector ""
        "Jan"
        "Feb"
        "Mar"
        "Apr"
        "May"
        "Jun"
        "Jul"
        "Aug"
        "Sep"
        "Oct"
        "Nov"
        "Dec"
      ) ;vector
    ) ;define
    (define priv:LOCALE-LONG-MONTH-VECTOR
      (vector ""
        "January"
        "February"
        "March"
        "April"
        "May"
        "June"
        "July"
        "August"
        "September"
        "October"
        "November"
        "December"
      ) ;vector
    ) ;define

    (define priv:LOCALE-PM "PM")
    (define priv:LOCALE-AM "AM")

    ;; See `date->string` below
    (define priv:LOCALE-DATE-TIME-FORMAT
      "~a ~b ~d ~H:~M:~S~z ~Y"
    ) ;define
    (define priv:LOCALE-SHORT-DATE-FORMAT
      "~m/~d/~y"
    ) ;define
    (define priv:LOCALE-TIME-FORMAT
      "~H:~M:~S"
    ) ;define
    (define priv:ISO-8601-DATE-TIME-FORMAT
      "~Y-~m-~dT~H:~M:~S~z"
    ) ;define

    (define priv:NANO (expt 10 9))
    (define priv:SID 86400)
    (define priv:SIHD 43200)
    (define priv:TAI-EPOCH-IN-JD 4881175/2)

    ;; ====================
    ;; Time object and accessors
    ;; ====================

    (define-record-type <time>
      (%make-time type nanosecond second)
      time?
      (type time-type set-time-type!)
      (nanosecond time-nanosecond
        set-time-nanosecond!
      ) ;nanosecond
      (second time-second set-time-second!)
    ) ;define-record-type

    (define (make-time type nanosecond second)
      (unless (and (integer? nanosecond)
                (integer? second)
              ) ;and
        (error 'wrong-type-arg
          "nanosecond and second should be integer"
        ) ;error
      ) ;unless
      (unless (member type
                (map car priv:TIME-DISPATCH)
              ) ;member
        (value-error "unsupported time type"
          type
        ) ;value-error
      ) ;unless
      (%make-time type nanosecond second)
    ) ;define

    (define (copy-time time)
      (make-time (time-type time)
        (time-nanosecond time)
        (time-second time)
      ) ;make-time
    ) ;define

    ;; ====================
    ;; Time comparison procedures
    ;; ====================

    (define (priv:check-same-time-type time1 time2)
      (unless (and (time? time1) (time? time2))
        (error 'wrong-type-arg
          "time comparison: time1 and time2 must be time objects"
          (list time1 time2)
        ) ;error
      ) ;unless
      (unless (eq? (time-type time1)
                (time-type time2)
              ) ;eq?
        (error 'wrong-type-arg
          "time comparison: time types must match"
          (list (time-type time1)
            (time-type time2)
          ) ;list
        ) ;error
      ) ;unless
    ) ;define

    (define (priv:time-compare time1 time2)
      (priv:check-same-time-type time1 time2)
      (let ((delta (- (priv:time->nanoseconds time1)
                     (priv:time->nanoseconds time2)
                   ) ;-
            ) ;delta
           ) ;
        (cond ((< delta 0) -1)
              ((> delta 0) 1)
              (else 0)
        ) ;cond
      ) ;let
    ) ;define

    (define (time<? time1 time2)
      (< (priv:time-compare time1 time2) 0)
    ) ;define

    (define (time<=? time1 time2)
      (<= (priv:time-compare time1 time2) 0)
    ) ;define

    (define (time=? time1 time2)
      (= (priv:time-compare time1 time2) 0)
    ) ;define

    (define (time>=? time1 time2)
      (>= (priv:time-compare time1 time2) 0)
    ) ;define

    (define (time>? time1 time2)
      (> (priv:time-compare time1 time2) 0)
    ) ;define

    ;; ====================
    ;; Time arithmetic procedures
    ;; ====================

    (define (priv:time->nanoseconds time)
      (+ (* (time-second time) priv:NANO)
        (time-nanosecond time)
      ) ;+
    ) ;define

    (define (priv:time-difference time1 time2 time3)
      (unless (and (time? time1) (time? time2))
        (error 'wrong-type-arg
          "time-difference: time1 and time2 must be time objects"
          (list time1 time2)
        ) ;error
      ) ;unless
      (unless (eq? (time-type time1)
                (time-type time2)
              ) ;eq?
        (error 'wrong-type-arg
          "time-difference: time types must match"
          (list (time-type time1)
            (time-type time2)
          ) ;list
        ) ;error
      ) ;unless
      (receive (secs nanos)
        (floor/ (- (priv:time->nanoseconds time1)
                  (priv:time->nanoseconds time2)
                ) ;-
          priv:NANO
        ) ;floor/
        (set-time-second! time3 secs)
        (set-time-nanosecond! time3 nanos)
      ) ;receive
      time3
    ) ;define

    (define (time-difference time1 time2)
      (priv:time-difference time1
        time2
        (%make-time TIME-DURATION 0 0)
      ) ;priv:time-difference
    ) ;define

    (define (priv:time-arithmetic time1
              time-duration
              op
            ) ;priv:time-arithmetic
      (unless (time? time1)
        (error 'wrong-type-arg
          "time arithmetic: time1 must be a time object"
          (list time1 time-duration)
        ) ;error
      ) ;unless
      (unless (and (time? time-duration)
                (eq? (time-type time-duration)
                  TIME-DURATION
                ) ;eq?
              ) ;and
        (error 'wrong-type-arg
          "time arithmetic: time-duration must be a TIME-DURATION object"
          (list time1 time-duration)
        ) ;error
      ) ;unless
      (receive (secs nanos)
        (floor/ (op (priv:time->nanoseconds time1)
                  (priv:time->nanoseconds time-duration)
                ) ;op
          priv:NANO
        ) ;floor/
        (let ((type (time-type time1)))
          (if (eq? type TIME-DURATION)
            (%make-time type nanos secs)
            (make-time type nanos secs)
          ) ;if
        ) ;let
      ) ;receive
    ) ;define

    (define (add-duration time1 time-duration)
      (priv:time-arithmetic time1
        time-duration
        +
      ) ;priv:time-arithmetic
    ) ;define

    (define (subtract-duration time1 time-duration)
      (priv:time-arithmetic time1
        time-duration
        -
      ) ;priv:time-arithmetic
    ) ;define

    ;; ====================
    ;; Current time and clock resolution
    ;; ====================

    (define (priv:us->ns us)
      (* us 1000)
    ) ;define

    ;; NOTE:
    ;; 此函数使用静态的闰秒偏移表 `priv:leap-second-table`
    ;; 来计算 TAI（国际原子时）与 UTC 之间的差值。
    ;; 该表包含了自1972年UTC系统引入闰秒机制以来，所有闰秒生效的时刻（Unix时间戳）
    ;; 以及从该时刻起，TAI 领先 UTC 的总秒数。
    ;;
    ;; 表格结构：((生效时间戳1 . 总偏移量1) (生效时间戳2 . 总偏移量2) ...)
    ;; 数据排列：最新的条目（时间戳最大）在前。
    ;; 数据来源：基于巴黎天文台等机构维护的闰秒历史记录生成。
    ;;
    ;; 工作原理：对于给定的 UTC 时间戳 `s`，函数从新到旧遍历此表，
    ;; 找到第一个 `生效时间戳 <= s` 的条目，并使用其对应的总偏移量。
    ;; 如果 `s` 早于表格中的最早记录（1972年之前），则返回初始偏移量 10 秒。
    ;;
    ;; 当前限制：
    ;; 1. 此表为静态数据。未来若有新的闰秒引入（如 IANA 文件所示，有效期至 2026-06-28），
    ;;    需要手动更新此表，在列表最前面添加新条目。
    ;; 2. 更动态的实现应考虑从外部权威源
    ;;   （如 https://hpiers.obspm.fr/iers/bul/bulc/ntp/leap-seconds.list）
    ;;    在程序启动时或定期加载并解析闰秒数据。
    ;;
    ;; 参考资料：
    ;; - IANA 闰秒数据文件: https://data.iana.org/time-zones/tzdb/leapseconds
    ;; - 巴黎天文台闰秒文件：https://hpiers.obspm.fr/iers/bul/bulc/ntp/leap-seconds.list
    (define priv:leap-second-table
      '((1483228800 . 37) (1435708800 . 36) (1341100800 . 35) (1230768000 . 34) (1136073600 . 33) (915148800 . 32) (867715200 . 31) (820454400 . 30) (773020800 . 29) (741484800 . 28) (709948800 . 27) (662688000 . 26) (631152000 . 25) (567993600 . 24) (489024000 . 23) (425865600 . 22) (394329600 . 21) (362793600 . 20) (315532800 . 19) (283996800 . 18) (252460800 . 17) (220924800 . 16) (189302400 . 15) (157766400 . 14) (126230400 . 13) (94694400 . 12) (78796800 . 11) (63072000 . 10))
    ) ;define
    (define (priv:leap-second-delta s)
      (let lp
        ((table priv:leap-second-table))
        (cond ((null? table) 10)
              ((>= s (caar table)) (cdar table))
              (else (lp (cdr table)))
        ) ;cond
      ) ;let
    ) ;define

    (define (priv:current-time-monotonic)
      (receive (s ns)
        (floor/ (glue:monotonic-nanosecond)
          1000000000
        ) ;floor/
        (make-time TIME-MONOTONIC ns s)
      ) ;receive
    ) ;define

    (define (priv:current-time-process)
      (error "unimplemented time")
    ) ;define

    (define (priv:current-time-tai)
      (receive (s us)
        (glue:get-time-of-day)
        (make-time TIME-TAI
          (priv:us->ns us)
          (+ s (priv:leap-second-delta s))
        ) ;make-time
      ) ;receive
    ) ;define

    (define (priv:current-time-thread)
      (error "unimplemented time")
    ) ;define

    (define (priv:current-time-utc)
      (receive (s us)
        (glue:get-time-of-day)
        (make-time TIME-UTC (priv:us->ns us) s)
      ) ;receive
    ) ;define

    (define priv:TIME-DISPATCH
      `((,TIME-MONOTONIC ,priv:current-time-monotonic unquote steady-clock-resolution) (,TIME-TAI ,priv:current-time-tai unquote system-clock-resolution) (,TIME-UTC ,priv:current-time-utc unquote system-clock-resolution))
    ) ;define
    (define (priv:query-time-dispatch clock-type
              querier
            ) ;priv:query-time-dispatch
      (let ((entry (assq clock-type priv:TIME-DISPATCH)
            ) ;entry
           ) ;
        (if entry
          (querier entry)
          (error 'wrong-type-arg
            "unsupported time type"
            clock-type
          ) ;error
        ) ;if
      ) ;let
    ) ;define

    ;; ====================

    (define (priv:round-offset-to-minute offset)
      (let* ((sign (if (negative? offset) -1 1))
             (abs-off (abs offset))
             (q (quotient abs-off 60))
             (r (remainder abs-off 60))
            ) ;
        (* sign (if (>= r 30) (+ q 1) q) 60)
      ) ;let*
    ) ;define

    (define (local-tz-offset)
      (let ((time-vec (g_datetime-now)))
        (if (and (vector? time-vec)
              (= (vector-length time-vec) 7)
            ) ;and
          (let* ((year (vector-ref time-vec 0))
                 (month (vector-ref time-vec 1))
                 (day (vector-ref time-vec 2))
                 (hour (vector-ref time-vec 3))
                 (minute (vector-ref time-vec 4))
                 (second (vector-ref time-vec 5))
                ) ;
            (receive (utc-sec utc-usec)
              (glue:get-time-of-day)
              (let* ((days (priv:days-since-epoch year month day)
                     ) ;days
                     (local-sec (+ (* days priv:SID)
                                  (* hour 3600)
                                  (* minute 60)
                                  second
                                ) ;+
                     ) ;local-sec
                     (offset (- local-sec utc-sec))
                    ) ;
                (priv:round-offset-to-minute offset)
              ) ;let*
            ) ;receive
          ) ;let*
          0
        ) ;if
      ) ;let
    ) ;define

    (define* (current-date (tz-offset (local-tz-offset))
             ) ;current-date
      (time-utc->date (current-time TIME-UTC)
        tz-offset
      ) ;time-utc->date
    ) ;define*

    (define (current-julian-day)
      (error 'todo "TODO")
    ) ;define

    (define* (current-time (clock-type TIME-UTC))
     ((priv:query-time-dispatch clock-type
        cadr
      ) ;priv:query-time-dispatch
     ) ;
    ) ;define*

    (define* (time-resolution (clock-type TIME-UTC))
      (priv:query-time-dispatch clock-type
        cddr
      ) ;priv:query-time-dispatch
    ) ;define*

    ;; ====================
    ;; Date object and accessors
    ;; ====================

    ;; Date objects are immutable once created
    (define-record-type <date>
      (%make-date nanosecond
        second
        minute
        hour
        day
        month
        year
        zone-offset
      ) ;%make-date
      date?
      (nanosecond date-nanosecond)
      (second date-second)
      (minute date-minute)
      (hour date-hour)
      (day date-day)
      (month date-month)
      (year date-year)
      (zone-offset date-zone-offset)
    ) ;define-record-type

    (define (make-date nanosecond
              second
              minute
              hour
              day
              month
              year
              zone-offset
            ) ;make-date
      ;; TODO: more guards maybe
      (unless (and (integer? nanosecond)
                (integer? second)
                (integer? minute)
                (integer? hour)
                (integer? day)
                (integer? month)
                (integer? year)
                (integer? zone-offset)
              ) ;and
        (error 'wrong-type-arg
          "The date fields need to be integer"
        ) ;error
      ) ;unless
      (%make-date nanosecond
        second
        minute
        hour
        day
        month
        year
        zone-offset
      ) ;%make-date
    ) ;define

    ;; ====================

    (define (priv:leap-year? year)
      (cond ((zero? (modulo year 400)) #t)
            ((zero? (modulo year 100)) #f)
            ((zero? (modulo year 4)) #t)
            (else #f)
      ) ;cond
    ) ;define

    (define priv:MONTH-ASSOC
      '((0 . 0) (1 . 31) (2 . 59) (3 . 90) (4 . 120) (5 . 151) (6 . 181) (7 . 212) (8 . 243) (9 . 273) (10 . 304) (11 . 334))
    ) ;define

    (define (priv:year-day day month year)
      (let ((days-pr (assoc (- month 1) priv:MONTH-ASSOC)
            ) ;days-pr
           ) ;
        (unless days-pr
          (value-error "invalid month specified"
            month
          ) ;value-error
        ) ;unless
        ;; 闰2月，所以2月之后，且当年是闰年的，要多一天
        (if (and (priv:leap-year? year) (> month 2))
          (+ day (cdr days-pr) 1)
          (+ day (cdr days-pr))
        ) ;if
      ) ;let
    ) ;define

    ;; ====================

    (define (priv:week-day day month year)
      (let* ((yy (if (negative? year) (+ year 1) year)
             ) ;yy
             (a (quotient (- 14 month) 12))
             (y (- yy a))
             (m (+ month (* 12 a) -2))
            ) ;
        (modulo (+ day
                  y
                  (floor-quotient y 4)
                  (- (floor-quotient y 100))
                  (floor-quotient y 400)
                  (floor-quotient (* 31 m) 12)
                ) ;+
          7
        ) ;modulo
      ) ;let*
    ) ;define

    (define (priv:days-before-first-week date
              day-of-week-starting-week
            ) ;priv:days-before-first-week
      (let* ((first-day (make-date 0
                          0
                          0
                          0
                          1
                          1
                          (date-year date)
                          0
                        ) ;make-date
             ) ;first-day
             (fdweek-day (date-week-day first-day))
            ) ;
        (modulo (- day-of-week-starting-week fdweek-day)
          7
        ) ;modulo
      ) ;let*
    ) ;define

    (define (date-year-day date)
      (priv:year-day (date-day date)
        (date-month date)
        (date-year date)
      ) ;priv:year-day
    ) ;define

    (define (date-week-day date)
      (priv:week-day (date-day date)
        (date-month date)
        (date-year date)
      ) ;priv:week-day
    ) ;define

    (define (date-week-number date
              day-of-week-starting-week
            ) ;date-week-number
      (floor-quotient (- (date-year-day date)
                        1
                        (priv:days-before-first-week date
                          day-of-week-starting-week
                        ) ;priv:days-before-first-week
                      ) ;-
        7
      ) ;floor-quotient
    ) ;define

    ;; ====================
    ;; Time/Date/Julian Day/Modified Julian Day Converters
    ;; ====================

    (define (priv:tai->utc-seconds tai-sec)
      (let lp
        ((table priv:leap-second-table))
        (cond ((null? table) (- tai-sec 10))
              (else (let* ((utc-start (caar table))
                           (delta (cdar table))
                           (tai-start (+ utc-start delta))
                          ) ;
                      (if (>= tai-sec tai-start)
                        (- tai-sec delta)
                        (lp (cdr table))
                      ) ;if
                    ) ;let*
              ) ;else
        ) ;cond
      ) ;let
    ) ;define

    (define (time-utc->time-tai time-utc)
      (unless (and (time? time-utc)
                (eq? (time-type time-utc) TIME-UTC)
              ) ;and
        (error 'wrong-type-arg
          "time-utc->time-tai: time-utc must be a TIME-UTC object"
          time-utc
        ) ;error
      ) ;unless
      (make-time TIME-TAI
        (time-nanosecond time-utc)
        (+ (time-second time-utc)
          (priv:leap-second-delta (time-second time-utc)
          ) ;priv:leap-second-delta
        ) ;+
      ) ;make-time
    ) ;define

    (define (time-tai->time-utc time-tai)
      (unless (and (time? time-tai)
                (eq? (time-type time-tai) TIME-TAI)
              ) ;and
        (error 'wrong-type-arg
          "time-tai->time-utc: time-tai must be a TIME-TAI object"
          time-tai
        ) ;error
      ) ;unless
      (make-time TIME-UTC
        (time-nanosecond time-tai)
        (priv:tai->utc-seconds (time-second time-tai)
        ) ;priv:tai->utc-seconds
      ) ;make-time
    ) ;define

    (define (time-utc->time-monotonic time-utc)
      (unless (and (time? time-utc)
                (eq? (time-type time-utc) TIME-UTC)
              ) ;and
        (error 'wrong-type-arg
          "time-utc->time-monotonic: time-utc must be a TIME-UTC object"
          time-utc
        ) ;error
      ) ;unless
      (make-time TIME-MONOTONIC
        (time-nanosecond time-utc)
        (time-second time-utc)
      ) ;make-time
    ) ;define

    (define (time-monotonic->time-utc time-monotonic
            ) ;time-monotonic->time-utc
      (unless (and (time? time-monotonic)
                (eq? (time-type time-monotonic)
                  TIME-MONOTONIC
                ) ;eq?
              ) ;and
        (error 'wrong-type-arg
          "time-monotonic->time-utc: time-monotonic must be a TIME-MONOTONIC object"
          time-monotonic
        ) ;error
      ) ;unless
      (make-time TIME-UTC
        (time-nanosecond time-monotonic)
        (time-second time-monotonic)
      ) ;make-time
    ) ;define

    (define (time-tai->time-monotonic time-tai)
      (unless (and (time? time-tai)
                (eq? (time-type time-tai) TIME-TAI)
              ) ;and
        (error 'wrong-type-arg
          "time-tai->time-monotonic: time-tai must be a TIME-TAI object"
          time-tai
        ) ;error
      ) ;unless
      (time-utc->time-monotonic (time-tai->time-utc time-tai)
      ) ;time-utc->time-monotonic
    ) ;define

    (define (time-monotonic->time-tai time-monotonic
            ) ;time-monotonic->time-tai
      (unless (and (time? time-monotonic)
                (eq? (time-type time-monotonic)
                  TIME-MONOTONIC
                ) ;eq?
              ) ;and
        (error 'wrong-type-arg
          "time-monotonic->time-tai: time-monotonic must be a TIME-MONOTONIC object"
          time-monotonic
        ) ;error
      ) ;unless
      (time-utc->time-tai (time-monotonic->time-utc time-monotonic
                          ) ;time-monotonic->time-utc
      ) ;time-utc->time-tai
    ) ;define

    (define (priv:days-since-epoch year month day)
      ;; Howard Hinnant's days_from_civil algorithm, inverse of civil-from-days
      (let* ((y (- year (if (<= month 2) 1 0)))
             (era (if (>= y 0)
                    (floor-quotient y 400)
                    (floor-quotient (- y 399) 400)
                  ) ;if
             ) ;era
             (yoe (- y (* era 400)))
             (m (+ month (if (> month 2) -3 9)))
             (doy (+ (floor-quotient (+ (* 153 m) 2) 5)
                    (- day 1)
                  ) ;+
             ) ;doy
             (doe (+ (* yoe 365)
                    (floor-quotient yoe 4)
                    (- (floor-quotient yoe 100))
                    doy
                  ) ;+
             ) ;doe
            ) ;
        (- (+ (* era 146097) doe) 719468)
      ) ;let*
    ) ;define

    (define (priv:civil-from-days days)
      ;; Howard Hinnant's algorithm, adapted for proleptic Gregorian calendar
      (let* ((z (+ days 719468))
             (era (if (>= z 0)
                    (floor-quotient z 146097)
                    (floor-quotient (- z 146096) 146097)
                  ) ;if
             ) ;era
             (doe (- z (* era 146097)))
             (yoe (floor-quotient (- doe
                                    (floor-quotient doe 1460)
                                    (- (floor-quotient doe 36524))
                                    (floor-quotient doe 146096)
                                  ) ;-
                    365
                  ) ;floor-quotient
             ) ;yoe
             (y (+ yoe (* era 400)))
             (doy (- doe
                    (+ (* 365 yoe)
                      (floor-quotient yoe 4)
                      (- (floor-quotient yoe 100))
                    ) ;+
                  ) ;-
             ) ;doy
             (mp (floor-quotient (+ (* 5 doy) 2) 153)
             ) ;mp
             (d (+ (- doy
                     (floor-quotient (+ (* 153 mp) 2) 5)
                   ) ;-
                  1
                ) ;+
             ) ;d
             (m (+ mp (if (< mp 10) 3 -9)))
             (y (if (<= m 2) (+ y 1) y))
            ) ;
        (values y m d)
      ) ;let*
    ) ;define

    ;; Default tz-offset uses local time zone from OS.
    (define* (time-utc->date time-utc
               (tz-offset (local-tz-offset))
             ) ;time-utc->date
      (unless (and (time? time-utc)
                (eq? (time-type time-utc) TIME-UTC)
              ) ;and
        (error 'wrong-type-arg
          "time-utc->date: time-utc must be a TIME-UTC object"
          time-utc
        ) ;error
      ) ;unless
      (unless (integer? tz-offset)
        (error 'wrong-type-arg
          "time-utc->date: tz-offset must be an integer"
          tz-offset
        ) ;error
      ) ;unless
      (let* ((sec (+ (time-second time-utc) tz-offset)
             ) ;sec
             (nsec (time-nanosecond time-utc))
            ) ;
        (receive (days day-sec)
          (floor/ sec priv:SID)
          (receive (year month day)
            (priv:civil-from-days days)
            (receive (hour rem1)
              (floor/ day-sec 3600)
              (receive (minute second)
                (floor/ rem1 60)
                (make-date nsec
                  second
                  minute
                  hour
                  day
                  month
                  year
                  tz-offset
                ) ;make-date
              ) ;receive
            ) ;receive
          ) ;receive
        ) ;receive
      ) ;let*
    ) ;define*

    (define (date->time-utc date)
      (unless (date? date)
        (error 'wrong-type-arg
          "date->time-utc: date must be a date object"
          date
        ) ;error
      ) ;unless
      (let* ((days (priv:days-since-epoch (date-year date)
                     (date-month date)
                     (date-day date)
                   ) ;priv:days-since-epoch
             ) ;days
             (local-sec (+ (* days priv:SID)
                          (* (date-hour date) 3600)
                          (* (date-minute date) 60)
                          (date-second date)
                        ) ;+
             ) ;local-sec
             (utc-sec (- local-sec (date-zone-offset date))
             ) ;utc-sec
            ) ;
        (make-time TIME-UTC
          (date-nanosecond date)
          utc-sec
        ) ;make-time
      ) ;let*
    ) ;define

    ;; Default tz-offset uses local time zone from OS.
    (define* (time-tai->date time-tai
               (tz-offset (local-tz-offset))
             ) ;time-tai->date
      (unless (and (time? time-tai)
                (eq? (time-type time-tai) TIME-TAI)
              ) ;and
        (error 'wrong-type-arg
          "time-tai->date: time-tai must be a TIME-TAI object"
          time-tai
        ) ;error
      ) ;unless
      (unless (integer? tz-offset)
        (error 'wrong-type-arg
          "time-tai->date: tz-offset must be an integer"
          tz-offset
        ) ;error
      ) ;unless
      (time-utc->date (time-tai->time-utc time-tai)
        tz-offset
      ) ;time-utc->date
    ) ;define*

    (define (date->time-tai date)
      (time-utc->time-tai (date->time-utc date)
      ) ;time-utc->time-tai
    ) ;define

    ;; Default tz-offset uses local time zone from OS.
    (define* (time-monotonic->date time-monotonic
               (tz-offset (local-tz-offset))
             ) ;time-monotonic->date
      (unless (and (time? time-monotonic)
                (eq? (time-type time-monotonic)
                  TIME-MONOTONIC
                ) ;eq?
              ) ;and
        (error 'wrong-type-arg
          "time-monotonic->date: time-monotonic must be a TIME-MONOTONIC object"
          time-monotonic
        ) ;error
      ) ;unless
      (unless (integer? tz-offset)
        (error 'wrong-type-arg
          "time-monotonic->date: tz-offset must be an integer"
          tz-offset
        ) ;error
      ) ;unless
      (time-utc->date (time-monotonic->time-utc time-monotonic
                      ) ;time-monotonic->time-utc
        tz-offset
      ) ;time-utc->date
    ) ;define*

    (define (date->time-monotonic date)
      (unless (date? date)
        (error 'wrong-type-arg
          "date->time-monotonic: date must be a date object"
          date
        ) ;error
      ) ;unless
      (time-utc->time-monotonic (date->time-utc date)
      ) ;time-utc->time-monotonic
    ) ;define

    (define (date->julian-day date)
      (unless (date? date)
        (error 'wrong-type-arg
          "date->julian-day: date must be a date object"
          date
        ) ;error
      ) ;unless
      (let* ((t (time-utc->time-monotonic (date->time-utc date)
                ) ;time-utc->time-monotonic
             ) ;t
             (secs (time-second t))
             (nsecs (time-nanosecond t))
             (total-secs (+ secs (/ nsecs priv:NANO))
             ) ;total-secs
            ) ;
        (+ priv:TAI-EPOCH-IN-JD
          (/ total-secs priv:SID)
        ) ;+
      ) ;let*
    ) ;define

    (define (date->modified-julian-day date)
      (- (date->julian-day date) 4800001/2)
    ) ;define

    ;; ====================
    ;; Date to String/String to Date Converters
    ;; ====================

    (define (priv:locale-abbr-weekday n)
      (vector-ref priv:LOCALE-ABBR-WEEKDAY-VECTOR
        n
      ) ;vector-ref
    ) ;define
    (define (priv:locale-long-weekday n)
      (vector-ref priv:LOCALE-LONG-WEEKDAY-VECTOR
        n
      ) ;vector-ref
    ) ;define
    (define (priv:locale-abbr-month n)
      (vector-ref priv:LOCALE-ABBR-MONTH-VECTOR
        n
      ) ;vector-ref
    ) ;define
    (define (priv:locale-long-month n)
      (vector-ref priv:LOCALE-LONG-MONTH-VECTOR
        n
      ) ;vector-ref
    ) ;define

    ;; Only handles positive integers `n`. Internal use only.
    (define (priv:padding n pad-with len)
      (let* ((str (number->string n))
             (str-len (string-length str))
            ) ;
        (cond ((or (> str-len len) (not pad-with))
               str
              ) ;
              (else (string-pad str len pad-with))
        ) ;cond
      ) ;let*
    ) ;define

    (define (priv:locale-am/pm hr)
      (if (> hr 11)
        priv:LOCALE-PM
        priv:LOCALE-AM
      ) ;if
    ) ;define

    (define (priv:date-week-number-iso date)
      (let* ((year (date-year date))
             (jan1-wday (priv:week-day 1 1 year))
             (offset (if (> jan1-wday 4) 0 1))
             ;; 调整值：补偿 1-based 和 周日归属
             (adjusted (+ (date-year-day date) jan1-wday -2)
             ) ;adjusted
             (raw-week (+ (floor-quotient adjusted 7) offset)
             ) ;raw-week
            ) ;
        (cond ((zero? raw-week)
               (priv:date-week-number-iso (make-date 0 0 0 0 31 12 (- year 1) 0)
               ) ;priv:date-week-number-iso
              ) ;

              ((and (= raw-week 53)
                 (<= (priv:week-day 1 1 (+ year 1)) 4)
               ) ;and
               1
              ) ;

              (else raw-week)
        ) ;cond
      ) ;let*
    ) ;define

    (define (priv:last-n-digits i n)
      (modulo i (expt 10 n))
    ) ;define

    (define (priv:tz-printer offset port)
      (cond ((zero? offset) (display "Z" port))
            ((negative? offset) (display "-" port))
            (else (display "+" port))
      ) ;cond
      (unless (zero? offset)
        (let ((hours (abs (quotient offset (* 60 60)))
              ) ;hours
              (minutes (abs (quotient (remainder offset (* 60 60))
                              60
                            ) ;quotient
                       ) ;abs
              ) ;minutes
             ) ;
          (display (priv:padding hours #\0 2)
            port
          ) ;display
          (display (priv:padding minutes #\0 2)
            port
          ) ;display
        ) ;let
      ) ;unless
    ) ;define

    (define (priv:directives/formatter char
              format-string
            ) ;priv:directives/formatter
      (lambda (date pad-with port)
        (case char
         ((#\~) (display #\~ port))
         ((#\a)
          (display (priv:locale-abbr-weekday (date-week-day date)
                   ) ;priv:locale-abbr-weekday
            port
          ) ;display
         ) ;
         ((#\A)
          (display (priv:locale-long-weekday (date-week-day date)
                   ) ;priv:locale-long-weekday
            port
          ) ;display
         ) ;
         ((#\b #\h)
          (display (priv:locale-abbr-month (date-month date)
                   ) ;priv:locale-abbr-month
            port
          ) ;display
         ) ;
         ((#\B)
          (display (priv:locale-long-month (date-month date)
                   ) ;priv:locale-long-month
            port
          ) ;display
         ) ;
         ((#\c)
          (display (date->string date
                     priv:LOCALE-DATE-TIME-FORMAT
                   ) ;date->string
            port
          ) ;display
         ) ;
         ((#\d)
          (display (priv:padding (date-day date) #\0 2)
            port
          ) ;display
         ) ;
         ((#\D)
          (display (date->string date "~m/~d/~y")
            port
          ) ;display
         ) ;
         ((#\e)
          (display (priv:padding (date-day date) #\space 2)
            port
          ) ;display
         ) ;
         ;; ref Guile
         ((#\f)
          (receive (s ns)
            (floor/ (+ (* (date-second date) priv:NANO)
                      (date-nanosecond date)
                    ) ;+
              priv:NANO
            ) ;floor/
            (display (number->string s) port)
            (display priv:LOCALE-DECIMAL-POINT port)
            (let ((str (priv:padding ns #\0 9)))
              (display (substring str 0 1) port)
              (display (string-trim-right str #\0 1)
                port
              ) ;display
            ) ;let
          ) ;receive
         ) ;
         ((#\H)
          (display (priv:padding (date-hour date)
                     pad-with
                     2
                   ) ;priv:padding
            port
          ) ;display
         ) ;
         ((#\I)
          (display (priv:padding (+ 1 (modulo (- (date-hour date) 1) 12))
                     pad-with
                     2
                   ) ;priv:padding
            port
          ) ;display
         ) ;
         ((#\j)
          (display (priv:padding (date-year-day date)
                     pad-with
                     3
                   ) ;priv:padding
            port
          ) ;display
         ) ;
         ((#\k)
          (display (priv:padding (date-hour date)
                     #\space
                     2
                   ) ;priv:padding
            port
          ) ;display
         ) ;
         ((#\l)
          (display (priv:padding (+ 1 (modulo (- (date-hour date) 1) 12))
                     #\space
                     2
                   ) ;priv:padding
            port
          ) ;display
         ) ;
         ((#\m)
          (display (priv:padding (date-month date)
                     pad-with
                     2
                   ) ;priv:padding
            port
          ) ;display
         ) ;
         ((#\M)
          (display (priv:padding (date-minute date)
                     pad-with
                     2
                   ) ;priv:padding
            port
          ) ;display
         ) ;
         ((#\n) (newline port))
         ((#\N)
          (display (priv:padding (date-nanosecond date)
                     pad-with
                     9
                   ) ;priv:padding
            port
          ) ;display
         ) ;
         ((#\p)
          (display (priv:locale-am/pm (date-hour date))
            port
          ) ;display
         ) ;
         ((#\r)
          (display (date->string date "~I:~M:~S ~p")
            port
          ) ;display
         ) ;

         ((#\s) (error "Not Implement"))
         ((#\S)
          (let ((sec-delta (if (> (date-nanosecond date) priv:NANO)
                             1
                             0
                           ) ;if
                ) ;sec-delta
               ) ;
            (display (priv:padding (+ (date-second date) sec-delta)
                       pad-with
                       2
                     ) ;priv:padding
              port
            ) ;display
          ) ;let
         ) ;

         ((#\t) (display #\tab port))
         ((#\T)
          (display (date->string date "~H:~M:~S")
            port
          ) ;display
         ) ;
         ((#\U)
          (let* ((week>0? (> (priv:days-before-first-week date 0)
                            0
                          ) ;>
                 ) ;week>0?
                 (week-num (date-week-number date 0))
                 (week-num* (if week>0? (+ week-num 1) week-num)
                 ) ;week-num*
                ) ;
            (display (priv:padding week-num* #\0 2)
              port
            ) ;display
          ) ;let*
         ) ;

         ((#\V)
          (display (priv:padding (priv:date-week-number-iso date)
                     #\0
                     2
                   ) ;priv:padding
            port
          ) ;display
         ) ;
         ((#\w)
          (display (date-week-day date) port)
         ) ;
         ((#\W)
          (let* ((week>1? (> (priv:days-before-first-week date 1)
                            0
                          ) ;>
                 ) ;week>1?
                 (week-num (date-week-number date 1))
                 (week-num* (if week>1? (+ week-num 1) week-num)
                 ) ;week-num*
                ) ;
            (display (priv:padding week-num* #\0 2)
              port
            ) ;display
          ) ;let*
         ) ;
         ((#\x)
          (display (date->string date
                     priv:LOCALE-SHORT-DATE-FORMAT
                   ) ;date->string
            port
          ) ;display
         ) ;
         ((#\X)
          (display (date->string date
                     priv:LOCALE-TIME-FORMAT
                   ) ;date->string
            port
          ) ;display
         ) ;
         ((#\y)
          (display (priv:padding (priv:last-n-digits (date-year date) 2)
                     pad-with
                     2
                   ) ;priv:padding
            port
          ) ;display
         ) ;
         ((#\Y)
          (display (priv:padding (date-year date)
                     pad-with
                     4
                   ) ;priv:padding
            port
          ) ;display
         ) ;
         ((#\z)
          (priv:tz-printer (date-zone-offset date)
            port
          ) ;priv:tz-printer
         ) ;
         ((#\Z) (error "Not Implement"))
         ((#\1)
          (display (date->string date "~Y-~m-~d")
            port
          ) ;display
         ) ;
         ((#\2)
          (display (date->string date "~H:~M:~S~z")
            port
          ) ;display
         ) ;
         ((#\3)
          (display (date->string date "~H:~M:~S")
            port
          ) ;display
         ) ;
         ((#\4)
          (display (date->string date
                     "~Y-~m-~dT~H:~M:~S~z"
                   ) ;date->string
            port
          ) ;display
         ) ;
         ((#\5)
          (display (date->string date "~Y-~m-~dT~H:~M:~S")
            port
          ) ;display
         ) ;
         (else (priv:bad-format-error format-string)
         ) ;else
        ) ;case
      ) ;lambda
    ) ;define

    (define (priv:bad-format-error format-string)
      (value-error "bad date format string"
        format-string
      ) ;value-error
    ) ;define

    (define (priv:date-printer date
              format-string
              format-string-port
              port
            ) ;priv:date-printer
      (let ((current-char (read-char format-string-port)
            ) ;current-char
            (pad-char (peek-char format-string-port)
            ) ;pad-char
           ) ;
        (cond ((eof-object? current-char) (values))

              ((and (eof-object? pad-char)
                 ;; unfinished directives
                 (char=? current-char #\~)
               ) ;and
               (priv:bad-format-error format-string)
              ) ;

              ((and (char=? current-char #\~)
                 (or (char=? pad-char #\-)
                   (char=? pad-char #\_)
                 ) ;or
               ) ;and
               (let ((pad-pad-char (begin
                                     (read-char format-string-port)
                                     (peek-char format-string-port)
                                   ) ;begin
                     ) ;pad-pad-char
                    ) ;
                 (if (eof-object? pad-pad-char)
                   (priv:bad-format-error format-string)
                   (let ((formatter (priv:directives/formatter pad-pad-char
                                      format-string
                                    ) ;priv:directives/formatter
                         ) ;formatter
                         (pad-with (if (char=? pad-char #\-) #f #\space)
                         ) ;pad-with
                        ) ;
                     (begin
                       (formatter date pad-with port)
                       (priv:date-printer date
                         format-string
                         format-string-port
                         port
                       ) ;priv:date-printer
                     ) ;begin
                   ) ;let
                 ) ;if
               ) ;let
              ) ;

              ((char=? current-char #\~)
               (let ((formatter (priv:directives/formatter pad-char
                                  format-string
                                ) ;priv:directives/formatter
                     ) ;formatter
                    ) ;
                 (begin
                   (formatter date #\0 port)
                   (read-char format-string-port)
                   (priv:date-printer date
                     format-string
                     format-string-port
                     port
                   ) ;priv:date-printer
                 ) ;begin
               ) ;let
              ) ;

              (else (display current-char port)
                (priv:date-printer date
                  format-string
                  format-string-port
                  port
                ) ;priv:date-printer
              ) ;else
        ) ;cond
      ) ;let
    ) ;define

    (define* (date->string date (format-string "~c"))
      (let ((str-port (open-output-string)))
        (priv:date-printer date
          format-string
          (open-input-string format-string)
          str-port
        ) ;priv:date-printer
        (get-output-string str-port)
      ) ;let
    ) ;define*

    (define (priv:string-ci-prefix? str pos prefix)
      (let* ((plen (string-length prefix))
             (slen (string-length str))
            ) ;
        (and (<= (+ pos plen) slen)
          (let loop
            ((i 0))
            (cond ((= i plen) #t)
                  ((char=? (char-downcase (string-ref str (+ pos i))
                           ) ;char-downcase
                     (char-downcase (string-ref prefix i))
                   ) ;char=?
                   (loop (+ i 1))
                  ) ;
                  (else #f)
            ) ;cond
          ) ;let
        ) ;and
      ) ;let*
    ) ;define

    (define (priv:skip-to pred str pos)
      (let ((len (string-length str)))
        (let loop
          ((i pos))
          (cond ((>= i len)
                 (value-error "string->date: input does not match template"
                   str
                 ) ;value-error
                ) ;
                ((pred (string-ref str i)) i)
                (else (loop (+ i 1)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (priv:read-digits str pos)
      (let ((len (string-length str)))
        (let loop
          ((i pos))
          (if (and (< i len)
                (char-numeric? (string-ref str i))
              ) ;and
            (loop (+ i 1))
            (if (= i pos)
              (value-error "string->date: expected digits"
                str
              ) ;value-error
              (values (substring str pos i) i)
            ) ;if
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (priv:read-fixed-digits str pos n)
      (let ((end (+ pos n))
            (len (string-length str))
           ) ;
        (when (> end len)
          (value-error "string->date: expected digits"
            str
          ) ;value-error
        ) ;when
        (let loop
          ((i pos))
          (if (= i end)
            (values (substring str pos end) end)
            (if (char-numeric? (string-ref str i))
              (loop (+ i 1))
              (value-error "string->date: expected digits"
                str
              ) ;value-error
            ) ;if
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (priv:match-locale str pos vec)
      (let ((len (vector-length vec)))
        (let loop
          ((i 0) (best-index #f) (best-len 0))
          (if (= i len)
            (if best-index
              (values best-index (+ pos best-len))
              (value-error "string->date: invalid locale name"
                str
              ) ;value-error
            ) ;if
            (let ((name (vector-ref vec i)))
              (if (and (string? name)
                    (> (string-length name) 0)
                    (priv:string-ci-prefix? str pos name)
                  ) ;and
                (let ((nlen (string-length name)))
                  (if (> nlen best-len)
                    (loop (+ i 1) i nlen)
                    (loop (+ i 1) best-index best-len)
                  ) ;if
                ) ;let
                (loop (+ i 1) best-index best-len)
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (string->date input-string
              template-string
            ) ;string->date
      (unless (and (string? input-string)
                (string? template-string)
              ) ;and
        (error 'wrong-type-arg
          "string->date: input-string and template-string must be strings"
          (list input-string template-string)
        ) ;error
      ) ;unless
      (let* ((input input-string)
             (len (string-length input))
             (year #f)
             (month #f)
             (day #f)
             (hour #f)
             (minute #f)
             (second #f)
             (nanosecond #f)
             (zone-offset #f)
             (year-day #f)
             (week-day #f)
             (week-number #f)
             (week-start #f)
             (week-number-iso #f)
             (hour12 #f)
             (ampm #f)
            ) ;

        (define (priv:expect-char pos ch)
          (if (and (< pos len)
                (char=? (string-ref input pos) ch)
              ) ;and
            (+ pos 1)
            (value-error "string->date: input does not match template"
              (list input template-string)
            ) ;value-error
          ) ;if
        ) ;define

        (define (priv:read-number pos
                  skip-pred
                  allow-space?
                  allow-sign?
                ) ;priv:read-number
          (let* ((pos (if skip-pred
                        (priv:skip-to skip-pred input pos)
                        pos
                      ) ;if
                 ) ;pos
                 (pos (if (and allow-space?
                            (< pos len)
                            (char=? (string-ref input pos) #\space)
                          ) ;and
                        (+ pos 1)
                        pos
                      ) ;if
                 ) ;pos
                 (start pos)
                 (pos (if (and allow-sign?
                            (< pos len)
                            (or (char=? (string-ref input pos) #\+)
                              (char=? (string-ref input pos) #\-)
                            ) ;or
                          ) ;and
                        (+ pos 1)
                        pos
                      ) ;if
                 ) ;pos
                ) ;
            (receive (digits end)
              (priv:read-digits input pos)
              (values (string->number (substring input start end)
                      ) ;string->number
                end
              ) ;values
            ) ;receive
          ) ;let*
        ) ;define

        (define (priv:parse-tz-offset pos)
          (cond ((and (< pos len)
                   (char=? (string-ref input pos) #\Z)
                 ) ;and
                 (values 0 (+ pos 1))
                ) ;
                ((and (< pos len)
                   (or (char=? (string-ref input pos) #\+)
                     (char=? (string-ref input pos) #\-)
                   ) ;or
                 ) ;and
                 (let* ((sign (if (char=? (string-ref input pos) #\-)
                                -1
                                1
                              ) ;if
                        ) ;sign
                        (pos (+ pos 1))
                       ) ;
                   (receive (hh pos1)
                     (priv:read-fixed-digits input pos 2)
                     (let ((pos2 pos1))
                       (if (and (< pos2 len)
                             (char=? (string-ref input pos2) #\:)
                           ) ;and
                         (set! pos2 (+ pos2 1))
                       ) ;if
                       (receive (mm pos3)
                         (priv:read-fixed-digits input pos2 2)
                         (values (* sign
                                   (+ (* (string->number hh) 3600)
                                     (* (string->number mm) 60)
                                   ) ;+
                                 ) ;*
                           pos3
                         ) ;values
                       ) ;receive
                     ) ;let
                   ) ;receive
                 ) ;let*
                ) ;
                (else (value-error "string->date: invalid time zone offset"
                        input
                      ) ;value-error
                ) ;else
          ) ;cond
        ) ;define

        (define (priv:resolve-two-digit-year y)
          (let* ((y2 (modulo y 100))
                 (cur-year (date-year (current-date 0)))
                 (century (* (quotient cur-year 100) 100)
                 ) ;century
                 (candidate (+ century y2))
                ) ;
            (cond ((< candidate (- cur-year 50))
                   (+ candidate 100)
                  ) ;
                  ((> candidate (+ cur-year 49))
                   (- candidate 100)
                  ) ;
                  (else candidate)
            ) ;cond
          ) ;let*
        ) ;define

        (define (priv:parse-directive dir pos)
          (case dir
           ((#\~) (priv:expect-char pos #\~))
           ((#\n) (priv:expect-char pos #\newline))
           ((#\t) (priv:expect-char pos #\tab))
           ((#\a)
            (let ((pos (priv:skip-to char-alphabetic?
                         input
                         pos
                       ) ;priv:skip-to
                  ) ;pos
                 ) ;
              (receive (idx pos2)
                (priv:match-locale input
                  pos
                  priv:LOCALE-ABBR-WEEKDAY-VECTOR
                ) ;priv:match-locale
                pos2
              ) ;receive
            ) ;let
           ) ;
           ((#\A)
            (let ((pos (priv:skip-to char-alphabetic?
                         input
                         pos
                       ) ;priv:skip-to
                  ) ;pos
                 ) ;
              (receive (idx pos2)
                (priv:match-locale input
                  pos
                  priv:LOCALE-LONG-WEEKDAY-VECTOR
                ) ;priv:match-locale
                pos2
              ) ;receive
            ) ;let
           ) ;
           ((#\b #\h)
            (let ((pos (priv:skip-to char-alphabetic?
                         input
                         pos
                       ) ;priv:skip-to
                  ) ;pos
                 ) ;
              (receive (idx pos2)
                (priv:match-locale input
                  pos
                  priv:LOCALE-ABBR-MONTH-VECTOR
                ) ;priv:match-locale
                (set! month idx)
                pos2
              ) ;receive
            ) ;let
           ) ;
           ((#\B)
            (let ((pos (priv:skip-to char-alphabetic?
                         input
                         pos
                       ) ;priv:skip-to
                  ) ;pos
                 ) ;
              (receive (idx pos2)
                (priv:match-locale input
                  pos
                  priv:LOCALE-LONG-MONTH-VECTOR
                ) ;priv:match-locale
                (set! month idx)
                pos2
              ) ;receive
            ) ;let
           ) ;
           ((#\c)
            (priv:parse-template priv:LOCALE-DATE-TIME-FORMAT
              pos
            ) ;priv:parse-template
           ) ;
           ((#\D)
            (priv:parse-template "~m/~d/~y" pos)
           ) ;
           ((#\d)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! day v)
              pos2
            ) ;receive
           ) ;
           ((#\e)
            (receive (v pos2)
              (priv:read-number pos #f #t #f)
              (set! day v)
              pos2
            ) ;receive
           ) ;
           ((#\f)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! second v)
              (let ((pos3 pos2))
                (if (and (< pos3 len)
                      (char=? (string-ref input pos3)
                        (string-ref priv:LOCALE-DECIMAL-POINT 0)
                      ) ;char=?
                    ) ;and
                  (begin
                    (set! pos3 (+ pos3 1))
                    (receive (frac pos4)
                      (priv:read-digits input pos3)
                      (let ((flen (string-length frac)))
                        (when (> flen 9)
                          (value-error "string->date: invalid fractional seconds"
                            input
                          ) ;value-error
                        ) ;when
                        (set! nanosecond
                          (* (string->number frac)
                            (expt 10 (- 9 flen))
                          ) ;*
                        ) ;set!
                      ) ;let
                      pos4
                    ) ;receive
                  ) ;begin
                  (begin
                    (set! nanosecond 0)
                    pos3
                  ) ;begin
                ) ;if
              ) ;let
            ) ;receive
           ) ;
           ((#\H)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! hour v)
              pos2
            ) ;receive
           ) ;
           ((#\I)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! hour12 v)
              pos2
            ) ;receive
           ) ;
           ((#\j)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! year-day v)
              pos2
            ) ;receive
           ) ;
           ((#\k)
            (receive (v pos2)
              (priv:read-number pos #f #t #f)
              (set! hour v)
              pos2
            ) ;receive
           ) ;
           ((#\l)
            (receive (v pos2)
              (priv:read-number pos #f #t #f)
              (set! hour12 v)
              pos2
            ) ;receive
           ) ;
           ((#\m)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! month v)
              pos2
            ) ;receive
           ) ;
           ((#\M)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! minute v)
              pos2
            ) ;receive
           ) ;
           ((#\N)
            (let ((pos (priv:skip-to char-numeric? input pos)
                  ) ;pos
                 ) ;
              (receive (digits pos2)
                (priv:read-digits input pos)
                (when (> (string-length digits) 9)
                  (value-error "string->date: invalid nanosecond"
                    input
                  ) ;value-error
                ) ;when
                (set! nanosecond
                  (string->number digits)
                ) ;set!
                pos2
              ) ;receive
            ) ;let
           ) ;
           ((#\p)
            (let ((pos (priv:skip-to char-alphabetic?
                         input
                         pos
                       ) ;priv:skip-to
                  ) ;pos
                 ) ;
              (cond ((priv:string-ci-prefix? input
                       pos
                       priv:LOCALE-AM
                     ) ;priv:string-ci-prefix?
                     (set! ampm 'am)
                     (+ pos (string-length priv:LOCALE-AM))
                    ) ;
                    ((priv:string-ci-prefix? input
                       pos
                       priv:LOCALE-PM
                     ) ;priv:string-ci-prefix?
                     (set! ampm 'pm)
                     (+ pos (string-length priv:LOCALE-PM))
                    ) ;
                    (else (value-error "string->date: invalid am/pm"
                            input
                          ) ;value-error
                    ) ;else
              ) ;cond
            ) ;let
           ) ;
           ((#\r)
            (priv:parse-template "~I:~M:~S ~p" pos)
           ) ;
           ((#\s) (error "Not Implement"))
           ((#\S)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! second v)
              pos2
            ) ;receive
           ) ;
           ((#\T)
            (priv:parse-template "~H:~M:~S" pos)
           ) ;
           ((#\U)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! week-number v)
              (set! week-start 0)
              pos2
            ) ;receive
           ) ;
           ((#\V)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! week-number-iso v)
              pos2
            ) ;receive
           ) ;
           ((#\w)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! week-day v)
              pos2
            ) ;receive
           ) ;
           ((#\W)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! week-number v)
              (set! week-start 1)
              pos2
            ) ;receive
           ) ;
           ((#\x)
            (priv:parse-template priv:LOCALE-SHORT-DATE-FORMAT
              pos
            ) ;priv:parse-template
           ) ;
           ((#\X)
            (priv:parse-template priv:LOCALE-TIME-FORMAT
              pos
            ) ;priv:parse-template
           ) ;
           ((#\y)
            (receive (v pos2)
              (priv:read-number pos #f #f #f)
              (set! year
                (priv:resolve-two-digit-year v)
              ) ;set!
              pos2
            ) ;receive
           ) ;
           ((#\Y)
            (receive (v pos2)
              (priv:read-number pos
                char-numeric?
                #f
                #f
              ) ;priv:read-number
              (set! year v)
              pos2
            ) ;receive
           ) ;
           ((#\z)
            (receive (v pos2)
              (priv:parse-tz-offset pos)
              (set! zone-offset v)
              pos2
            ) ;receive
           ) ;
           ((#\Z) (error "Not Implement"))
           ((#\1)
            (priv:parse-template "~Y-~m-~d" pos)
           ) ;
           ((#\2)
            (priv:parse-template "~H:~M:~S~z" pos)
           ) ;
           ((#\3)
            (priv:parse-template "~H:~M:~S" pos)
           ) ;
           ((#\4)
            (priv:parse-template "~Y-~m-~dT~H:~M:~S~z"
              pos
            ) ;priv:parse-template
           ) ;
           ((#\5)
            (priv:parse-template "~Y-~m-~dT~H:~M:~S"
              pos
            ) ;priv:parse-template
           ) ;
           (else (priv:bad-format-error template-string)
           ) ;else
          ) ;case
        ) ;define

        (define (priv:parse-template tmpl pos)
          (let ((tlen (string-length tmpl)))
            (let loop
              ((ti 0) (pos pos))
              (if (>= ti tlen)
                pos
                (let ((ch (string-ref tmpl ti)))
                  (if (char=? ch #\~)
                    (let* ((ti1 (+ ti 1)))
                      (when (>= ti1 tlen)
                        (priv:bad-format-error tmpl)
                      ) ;when
                      (let* ((next (string-ref tmpl ti1))
                             (pad? (or (char=? next #\-) (char=? next #\_))
                             ) ;pad?
                             (dir (if pad?
                                    (begin
                                      (when (>= (+ ti1 1) tlen)
                                        (priv:bad-format-error tmpl)
                                      ) ;when
                                      (string-ref tmpl (+ ti1 1))
                                    ) ;begin
                                    next
                                  ) ;if
                             ) ;dir
                             (pos2 (priv:parse-directive dir pos))
                             (ti2 (if pad? (+ ti 3) (+ ti 2)))
                            ) ;
                        (loop ti2 pos2)
                      ) ;let*
                    ) ;let*
                    (begin
                      (when (or (>= pos len)
                              (not (char=? (string-ref input pos) ch))
                            ) ;or
                        (value-error "string->date: input does not match template"
                          (list input template-string)
                        ) ;value-error
                      ) ;when
                      (loop (+ ti 1) (+ pos 1))
                    ) ;begin
                  ) ;if
                ) ;let
              ) ;if
            ) ;let
          ) ;let
        ) ;define

        (let ((pos (priv:parse-template template-string 0)
              ) ;pos
             ) ;
          (when (< pos len)
            (value-error "string->date: input does not match template"
              (list input template-string)
            ) ;value-error
          ) ;when

          (let* ((year* (or year 0))
                 (month* (or month 0))
                 (day* (or day 0))
                 (hour* (or hour 0))
                 (minute* (or minute 0))
                 (second* (or second 0))
                 (nanosecond* (or nanosecond 0))
                 (zone-offset* (or zone-offset 0))
                ) ;

            (when (and (not hour) hour12)
              (let ((h (modulo hour12 12)))
                (set! hour*
                  (if (eq? ampm 'pm) (+ h 12) h)
                ) ;set!
              ) ;let
            ) ;when

            (when (and year-day
                    (or (not month) (not day))
                  ) ;and
              (let ((days-in-year (if (priv:leap-year? year*) 366 365)
                    ) ;days-in-year
                   ) ;
                (unless (and (integer? year-day)
                          (<= 1 year-day days-in-year)
                        ) ;and
                  (value-error "string->date: invalid day-of-year"
                    input
                  ) ;value-error
                ) ;unless
                (receive (yy mm dd)
                  (priv:civil-from-days (+ (priv:days-since-epoch year* 1 1)
                                          (- year-day 1)
                                        ) ;+
                  ) ;priv:civil-from-days
                  (set! month* mm)
                  (set! day* dd)
                ) ;receive
              ) ;let
            ) ;when

            (when (and (or (not month) (not day))
                    week-number
                    (integer? week-start)
                    (integer? week-day)
                  ) ;and
              (let* ((wday-jan1 (priv:week-day 1 1 year*))
                     (offset (modulo (- week-start wday-jan1) 7)
                     ) ;offset
                     (wday (modulo (- week-day week-start) 7)
                     ) ;wday
                     (yday (if (= week-number 0)
                             (+ 1 (modulo (- week-day wday-jan1) 7))
                             (+ 1
                               offset
                               (* (- week-number 1) 7)
                               wday
                             ) ;+
                           ) ;if
                     ) ;yday
                     (days-in-year (if (priv:leap-year? year*) 366 365)
                     ) ;days-in-year
                    ) ;
                (unless (and (integer? yday)
                          (<= 1 yday days-in-year)
                        ) ;and
                  (value-error "string->date: invalid week number"
                    input
                  ) ;value-error
                ) ;unless
                (receive (yy mm dd)
                  (priv:civil-from-days (+ (priv:days-since-epoch year* 1 1)
                                          (- yday 1)
                                        ) ;+
                  ) ;priv:civil-from-days
                  (set! month* mm)
                  (set! day* dd)
                ) ;receive
              ) ;let*
            ) ;when

            (when (and (or (not month) (not day))
                    week-number-iso
                    (integer? week-day)
                  ) ;and
              (let* ((iso-wday (if (= week-day 0) 7 week-day)
                     ) ;iso-wday
                     (jan4-days (priv:days-since-epoch year* 1 4)
                     ) ;jan4-days
                     (jan4-wday (priv:week-day 4 1 year*))
                     (jan4-iso (if (= jan4-wday 0) 7 jan4-wday)
                     ) ;jan4-iso
                     (week1-monday (- jan4-days (- jan4-iso 1))
                     ) ;week1-monday
                     (target-days (+ week1-monday
                                    (* (- week-number-iso 1) 7)
                                    (- iso-wday 1)
                                  ) ;+
                     ) ;target-days
                    ) ;
                (receive (yy mm dd)
                  (priv:civil-from-days target-days)
                  (set! month* mm)
                  (set! day* dd)
                ) ;receive
              ) ;let*
            ) ;when

            (make-date nanosecond*
              second*
              minute*
              hour*
              day*
              month*
              year*
              zone-offset*
            ) ;make-date
          ) ;let*
        ) ;let
      ) ;let*
    ) ;define
  ) ;begin
) ;define-library
