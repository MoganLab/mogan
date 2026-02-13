;
; Copyright (C) 2026 The Goldfish Scheme Authors
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
                  (get-time-of-day      glue:get-time-of-day)
                  (monotonic-nanosecond glue:monotonic-nanosecond))
          (only (srfi srfi-13) string-pad string-tokenize string-trim-right)
          (only (srfi srfi-8) receive)
          (only (scheme base) open-output-string open-input-string get-output-string
                              floor/)
          (liii error))
  (export
    ;; Constants
    TIME-DURATION TIME-MONOTONIC TIME-PROCESS
    TIME-TAI      TIME-THREAD    TIME-UTC
    ;; Time object and accessors
    make-time time?
    time-type      time-nanosecond      time-second
    set-time-type! set-time-nanosecond! set-time-second!
    copy-time
    ;; Time comparison procedures
    time<=? time<? time=? time>=? time>?
    ;; Time arithmetic procedures
    time-difference
    ;; Current time and clock resolution
    current-date current-julian-day current-time time-resolution
    ;; Date object and accessors
    make-date date?
    date-nanosecond date-second date-minute date-hour
    date-day        date-month  date-year   date-zone-offset
    date-year-day
    date-week-day date-week-number
    ;; Time/Date/Julian Day/Modified Julian Day Converters
    time-utc->date date->time-utc
    ;; Date to String/String to Date Converters
    date->string)
  (begin

    ;; ====================
    ;; Constants
    ;; ====================

    (define TIME-DURATION  'time-duration)
    (define TIME-MONOTONIC 'time-monotonic)
    (define TIME-PROCESS   'time-process)
    (define TIME-TAI       'time-tai)
    (define TIME-THREAD    'time-thread)
    (define TIME-UTC       'time-utc)

    (define priv:LOCALE-DECIMAL-POINT ".")

    (define priv:LOCALE-ABBR-WEEKDAY-VECTOR (vector "Sun" "Mon" "Tue" "Wed"
                                                    "Thu" "Fri" "Sat"))
    (define priv:LOCALE-LONG-WEEKDAY-VECTOR (vector "Sunday" "Monday"
                                                    "Tuesday" "Wednesday"
                                                    "Thursday" "Friday"
                                                    "Saturday"))
    ;; note empty string in 0th place.
    (define priv:LOCALE-ABBR-MONTH-VECTOR   (vector "" "Jan" "Feb" "Mar"
                                                    "Apr" "May" "Jun" "Jul"
                                                    "Aug" "Sep" "Oct" "Nov"
                                                    "Dec"))
    (define priv:LOCALE-LONG-MONTH-VECTOR   (vector "" "January" "February"
                                                    "March" "April" "May"
                                                    "June" "July" "August"
                                                    "September" "October"
                                                    "November" "December"))

    (define priv:LOCALE-PM "PM")
    (define priv:LOCALE-AM "AM")

    ;; See `date->string` below
    (define priv:LOCALE-DATE-TIME-FORMAT   "~a ~b ~d ~H:~M:~S~z ~Y")
    (define priv:LOCALE-SHORT-DATE-FORMAT  "~m/~d/~y")
    (define priv:LOCALE-TIME-FORMAT        "~H:~M:~S")
    (define priv:ISO-8601-DATE-TIME-FORMAT "~Y-~m-~dT~H:~M:~S~z")

    (define priv:NANO            (expt 10 9))
    (define priv:SID             86400)     ; seconds in a day
    (define priv:SIHD            43200)     ; seconds in a half day
    (define priv:TAI-EPOCH-IN-JD 4881175/2) ; julian day number for 'the epoch'

    ;; ====================
    ;; Time object and accessors
    ;; ====================

    (define-record-type <time>
      (%make-time type nanosecond second)
      time?
      (type       time-type       set-time-type!)
      (nanosecond time-nanosecond set-time-nanosecond!)
      (second     time-second     set-time-second!))

    (define (make-time type nanosecond second)
      (unless (and (integer? nanosecond)
                   (integer? second))
        (error 'wrong-type-arg "nanosecond and second should be integer"))
      (unless (member type (map car priv:TIME-DISPATCH))
        (value-error "unsupported time type" type))
      (%make-time type nanosecond second))

    (define (copy-time time)
      (make-time (time-type       time)
                 (time-nanosecond time)
                 (time-second     time)))

    ;; ====================
    ;; Time comparison procedures
    ;; ====================

    (define (priv:check-same-time-type time1 time2)
      (unless (and (time? time1) (time? time2))
        (error 'wrong-type-arg "time comparison: time1 and time2 must be time objects"
               (list time1 time2)))
      (unless (eq? (time-type time1) (time-type time2))
        (error 'wrong-type-arg "time comparison: time types must match"
               (list (time-type time1) (time-type time2)))))

    (define (priv:time-compare time1 time2)
      (priv:check-same-time-type time1 time2)
      (let ((delta (- (priv:time->nanoseconds time1)
                      (priv:time->nanoseconds time2))))
        (cond
          ((< delta 0) -1)
          ((> delta 0) 1)
          (else 0))))

    (define (time<? time1 time2)
      (< (priv:time-compare time1 time2) 0))

    (define (time<=? time1 time2)
      (<= (priv:time-compare time1 time2) 0))

    (define (time=? time1 time2)
      (= (priv:time-compare time1 time2) 0))

    (define (time>=? time1 time2)
      (>= (priv:time-compare time1 time2) 0))

    (define (time>? time1 time2)
      (> (priv:time-compare time1 time2) 0))

    ;; ====================
    ;; Time arithmetic procedures
    ;; ====================

    (define (priv:time->nanoseconds time)
      (+ (* (time-second time) priv:NANO)
         (time-nanosecond time)))

    (define (priv:time-difference time1 time2 time3)
      (unless (and (time? time1) (time? time2))
        (error 'wrong-type-arg "time-difference: time1 and time2 must be time objects" (list time1 time2)))
      (unless (eq? (time-type time1) (time-type time2))
        (error 'wrong-type-arg "time-difference: time types must match"
               (list (time-type time1) (time-type time2))))
      (receive (secs nanos)
               (floor/ (- (priv:time->nanoseconds time1)
                          (priv:time->nanoseconds time2))
                       priv:NANO)
        (set-time-second! time3 secs)
        (set-time-nanosecond! time3 nanos))
      time3)

    (define (time-difference time1 time2)
      (priv:time-difference time1 time2 (%make-time TIME-DURATION 0 0)))

    ;; ====================
    ;; Current time and clock resolution
    ;; ====================

    (define (priv:us->ns us) (* us 1000))

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
      '((1483228800 . 37)   ; 2017-01-01 00:00:00 UTC 起，TAI-UTC = 37 秒
        (1435708800 . 36)   ; 2015-07-01 00:00:00 UTC 起，TAI-UTC = 36 秒
        (1341100800 . 35)   ; 2012-07-01 00:00:00 UTC 起，TAI-UTC = 35 秒
        (1230768000 . 34)   ; 2009-01-01 00:00:00 UTC 起，TAI-UTC = 34 秒
        (1136073600 . 33)   ; 2006-01-01 00:00:00 UTC 起，TAI-UTC = 33 秒
        (915148800  . 32)   ; 1999-01-01 00:00:00 UTC 起，TAI-UTC = 32 秒
        (867715200  . 31)   ; 1997-07-01 00:00:00 UTC 起，TAI-UTC = 31 秒
        (820454400  . 30)   ; 1996-01-01 00:00:00 UTC 起，TAI-UTC = 30 秒
        (773020800  . 29)   ; 1994-07-01 00:00:00 UTC 起，TAI-UTC = 29 秒
        (741484800  . 28)   ; 1993-07-01 00:00:00 UTC 起，TAI-UTC = 28 秒
        (709948800  . 27)   ; 1992-07-01 00:00:00 UTC 起，TAI-UTC = 27 秒
        (662688000  . 26)   ; 1991-01-01 00:00:00 UTC 起，TAI-UTC = 26 秒
        (631152000  . 25)   ; 1990-01-01 00:00:00 UTC 起，TAI-UTC = 25 秒
        (567993600  . 24)   ; 1988-01-01 00:00:00 UTC 起，TAI-UTC = 24 秒
        (489024000  . 23)   ; 1985-07-01 00:00:00 UTC 起，TAI-UTC = 23 秒
        (425865600  . 22)   ; 1983-07-01 00:00:00 UTC 起，TAI-UTC = 22 秒
        (394329600  . 21)   ; 1982-07-01 00:00:00 UTC 起，TAI-UTC = 21 秒
        (362793600  . 20)   ; 1981-07-01 00:00:00 UTC 起，TAI-UTC = 20 秒
        (315532800  . 19)   ; 1980-01-01 00:00:00 UTC 起，TAI-UTC = 19 秒
        (283996800  . 18)   ; 1979-01-01 00:00:00 UTC 起，TAI-UTC = 18 秒
        (252460800  . 17)   ; 1978-01-01 00:00:00 UTC 起，TAI-UTC = 17 秒
        (220924800  . 16)   ; 1977-01-01 00:00:00 UTC 起，TAI-UTC = 16 秒
        (189302400  . 15)   ; 1976-01-01 00:00:00 UTC 起，TAI-UTC = 15 秒
        (157766400  . 14)   ; 1975-01-01 00:00:00 UTC 起，TAI-UTC = 14 秒
        (126230400  . 13)   ; 1974-01-01 00:00:00 UTC 起，TAI-UTC = 13 秒
        (94694400   . 12)   ; 1973-01-01 00:00:00 UTC 起，TAI-UTC = 12 秒
        (78796800   . 11)   ; 1972-07-01 00:00:00 UTC 起，TAI-UTC = 11 秒
        (63072000   . 10))) ; 1972-01-01 00:00:00 UTC 起，TAI-UTC = 10 秒
                            ; 注：1972年之前，UTC与TAI的理论差也为10秒，但当时无正式闰秒机制。
    (define (priv:leap-second-delta s)
      (let lp ((table priv:leap-second-table))
        (cond
          ((null? table)       10) ; 理论值
          ((>= s (caar table)) (cdar table))
          (else                (lp (cdr table))))))

    (define (priv:current-time-monotonic)
      (receive (s ns) (floor/ (glue:monotonic-nanosecond) 1000000000) ; 1e9
       (make-time TIME-MONOTONIC ns s)))

    (define (priv:current-time-process)
      (error "unimplemented time"))

    (define (priv:current-time-tai)
      (receive (s us) (glue:get-time-of-day)
        (make-time TIME-TAI
                   (priv:us->ns us)
                   (+ s (priv:leap-second-delta s)))))

    (define (priv:current-time-thread)
      (error "unimplemented time"))

    (define (priv:current-time-utc)
      (receive (s us) (glue:get-time-of-day)
        (make-time TIME-UTC (priv:us->ns us) s)))

    (define priv:TIME-DISPATCH
       `((,TIME-MONOTONIC . (,priv:current-time-monotonic . ,steady-clock-resolution))
         ; (,TIME-PROCESS   . (,priv:current-time-process   . ,0))
         (,TIME-TAI       . (,priv:current-time-tai       . ,system-clock-resolution))
         ; (,TIME-THREAD    . (,priv:current-time-thread    . ,0))
         (,TIME-UTC       . (,priv:current-time-utc       . ,system-clock-resolution))))
    (define (priv:query-time-dispatch clock-type querier)
      (let ((entry (assq clock-type priv:TIME-DISPATCH)))
        (if entry
          (querier entry)
          (error 'wrong-type-arg "unsupported time type" clock-type))))

    ;; ====================

    (define* (current-date (tz-offset 'TODO-GET-LOCAL-TZ-FROM-OS))
      (error 'todo "TODO"))

    (define (current-julian-day)
      (error 'todo "TODO"))

    (define* (current-time (clock-type TIME-UTC))
      ((priv:query-time-dispatch clock-type cadr)))

    (define* (time-resolution (clock-type TIME-UTC))
      (priv:query-time-dispatch clock-type cddr))

    ;; ====================
    ;; Date object and accessors
    ;; ====================

    ;; Date objects are immutable once created
    (define-record-type <date>
      (%make-date nanosecond second minute hour day month year zone-offset)
      date?
      (nanosecond  date-nanosecond)
      (second      date-second)
      (minute      date-minute)
      (hour        date-hour)
      (day         date-day)
      (month       date-month)
      (year        date-year)
      (zone-offset date-zone-offset))

    (define (make-date nanosecond second minute hour day month year zone-offset)
      ;; TODO: more guards maybe
      (unless (and (integer? nanosecond) (integer? second)
                   (integer? minute)     (integer? hour)
                   (integer? day)        (integer? month)
                   (integer? year)       (integer? zone-offset))
        (error 'wrong-type-arg "The date fields need to be integer"))
      (%make-date nanosecond second minute hour day month year zone-offset))

    ;; ====================

    (define (priv:leap-year? year)
      (cond
        ((zero? (modulo year 400)) #t)
        ((zero? (modulo year 100)) #f)
        ((zero? (modulo year 4))   #t)
        (else                      #f)))

    (define priv:MONTH-ASSOC '((0 . 0)   (1 . 31)  (2 . 59)   (3 . 90)
                               (4 . 120) (5 . 151) (6 . 181)  (7 . 212)
                               (8 . 243) (9 . 273) (10 . 304) (11 . 334)))

    (define (priv:year-day day month year)
      (let ((days-pr (assoc (- month 1) priv:MONTH-ASSOC)))
        (unless days-pr
          (value-error "invalid month specified" month))
        ;; 闰2月，所以2月之后，且当年是闰年的，要多一天
        (if (and (priv:leap-year? year) (> month 2))
          (+ day (cdr days-pr) 1)
          (+ day (cdr days-pr)))))

    ;; ====================

    (define (priv:week-day day month year)
      (let* ((yy (if (negative? year) (+ year 1) year))
             (a (quotient (- 14 month) 12))
             (y (- yy a))
             (m (+ month (* 12 a) -2)))
        (modulo (+ day
                   y
                   (floor-quotient y 4)
                   (- (floor-quotient y 100))
                   (floor-quotient y 400)
                   (floor-quotient (* 31 m) 12))
                7)))

    (define (priv:days-before-first-week date day-of-week-starting-week)
      (let* ((first-day (make-date 0 0 0 0
                                   1
                                   1
                                   (date-year date)
                                   0))
             (fdweek-day (date-week-day first-day)))
        (modulo (- day-of-week-starting-week fdweek-day)
                7)))

    (define (date-year-day date)
      (priv:year-day (date-day   date)
                     (date-month date)
                     (date-year  date)))

    (define (date-week-day date)
      (priv:week-day (date-day   date)
                     (date-month date)
                     (date-year  date)))

    (define (date-week-number date day-of-week-starting-week)
      (floor-quotient
        (- (date-year-day date)
           1
           (priv:days-before-first-week date day-of-week-starting-week))
        7))

    ;; ====================
    ;; Time/Date/Julian Day/Modified Julian Day Converters
    ;; ====================

    (define (priv:days-since-epoch year month day)
      ;; Howard Hinnant's days_from_civil algorithm, inverse of civil-from-days
      (let* ((y (- year (if (<= month 2) 1 0)))
             (era (if (>= y 0)
                    (floor-quotient y 400)
                    (floor-quotient (- y 399) 400)))
             (yoe (- y (* era 400)))
             (m (+ month (if (> month 2) -3 9))) ; March=0, ..., February=11
             (doy (+ (floor-quotient (+ (* 153 m) 2) 5) (- day 1)))
             (doe (+ (* yoe 365)
                     (floor-quotient yoe 4)
                     (- (floor-quotient yoe 100))
                     doy)))
        (- (+ (* era 146097) doe) 719468)))

    (define (priv:civil-from-days days)
      ;; Howard Hinnant's algorithm, adapted for proleptic Gregorian calendar
      (let* ((z (+ days 719468))
             (era (if (>= z 0)
                    (floor-quotient z 146097)
                    (floor-quotient (- z 146096) 146097)))
             (doe (- z (* era 146097))) ; [0, 146096]
             (yoe (floor-quotient (- doe (floor-quotient doe 1460)
                                    (- (floor-quotient doe 36524))
                                    (floor-quotient doe 146096))
                                  365))
             (y (+ yoe (* era 400)))
             (doy (- doe (+ (* 365 yoe)
                            (floor-quotient yoe 4)
                            (- (floor-quotient yoe 100)))))
             (mp (floor-quotient (+ (* 5 doy) 2) 153))
             (d (+ (- doy (floor-quotient (+ (* 153 mp) 2) 5)) 1))
             (m (+ mp (if (< mp 10) 3 -9)))
             (y (if (<= m 2) (+ y 1) y)))
        (values y m d)))

    ;; TODO: spec says default tz-offset should be local time zone.
    ;; We don't have a local tz interface yet, so default is 0 (UTC).
    (define* (time-utc->date time-utc (tz-offset 0))
      (unless (and (time? time-utc) (eq? (time-type time-utc) TIME-UTC))
        (error 'wrong-type-arg "time-utc->date: time-utc must be a TIME-UTC object" time-utc))
      (unless (integer? tz-offset)
        (error 'wrong-type-arg "time-utc->date: tz-offset must be an integer" tz-offset))
      (let* ((sec (+ (time-second time-utc) tz-offset))
             (nsec (time-nanosecond time-utc)))
        (receive (days day-sec) (floor/ sec priv:SID)
          (receive (year month day) (priv:civil-from-days days)
            (receive (hour rem1) (floor/ day-sec 3600)
              (receive (minute second) (floor/ rem1 60)
                (make-date nsec second minute hour day month year tz-offset)))))))

    (define (date->time-utc date)
      (unless (date? date)
        (error 'wrong-type-arg "date->time-utc: date must be a date object" date))
      (let* ((days (priv:days-since-epoch (date-year date)
                                          (date-month date)
                                          (date-day date)))
             (local-sec (+ (* days priv:SID)
                           (* (date-hour date) 3600)
                           (* (date-minute date) 60)
                           (date-second date)))
             (utc-sec (- local-sec (date-zone-offset date))))
        (make-time TIME-UTC (date-nanosecond date) utc-sec)))

    ;; ====================
    ;; Date to String/String to Date Converters
    ;; ====================

    (define (priv:locale-abbr-weekday n)
      (vector-ref priv:LOCALE-ABBR-WEEKDAY-VECTOR n))
    (define (priv:locale-long-weekday n)
      (vector-ref priv:LOCALE-LONG-WEEKDAY-VECTOR n))
    (define (priv:locale-abbr-month n)
      (vector-ref priv:LOCALE-ABBR-MONTH-VECTOR n))
    (define (priv:locale-long-month n)
      (vector-ref priv:LOCALE-LONG-MONTH-VECTOR n))

    ;; Only handles positive integers `n`. Internal use only.
    (define (priv:padding n pad-with len)
      (let* ((str     (number->string n))
             (str-len (string-length str)))
        (cond
          ((or (> str-len len) (not pad-with)) str)
          (else (string-pad str len pad-with)))))

    (define (priv:locale-am/pm hr)
      (if (> hr 11) priv:LOCALE-PM priv:LOCALE-AM))

    (define (priv:date-week-number-iso date)
      (let* ((year (date-year date))
             (jan1-wday (priv:week-day 1 1 year))
             (offset (if (> jan1-wday 4) 0 1))
             ;; 调整值：补偿 1-based 和 周日归属
             (adjusted (+ (date-year-day date) jan1-wday -2))
             (raw-week (+ (floor-quotient adjusted 7) offset)))
        (cond ((zero? raw-week)
               (priv:date-week-number-iso 
                (make-date 0 0 0 0 31 12 (- year 1) 0)))

              ((and (= raw-week 53)
                    (<= (priv:week-day 1 1 (+ year 1)) 4))
               1)

              (else raw-week))))

    (define (priv:last-n-digits i n)
      (modulo i (expt 10 n)))

    (define (priv:tz-printer offset port)
      (cond
        ((zero? offset)     (display "Z" port))
        ((negative? offset) (display "-" port))
        (else               (display "+" port)))
      (unless (zero? offset)
        (let ((hours   (abs (quotient offset (* 60 60))))
              (minutes (abs (quotient (remainder offset (* 60 60)) 60))))
          (display (priv:padding hours   #\0 2) port)
          (display (priv:padding minutes #\0 2) port))))

    (define (priv:directives/formatter char format-string)
      (lambda (date pad-with port)
        (case char
          ((#\~) (display #\~ port))
          ((#\a) (display (priv:locale-abbr-weekday (date-week-day date))
                          port))
          ((#\A) (display (priv:locale-long-weekday (date-week-day date))
                          port))
          ((#\b #\h) (display (priv:locale-abbr-month (date-month date))
                              port))
          ((#\B) (display (priv:locale-long-month (date-month date))
                          port))
          ((#\c) (display (date->string date priv:LOCALE-DATE-TIME-FORMAT)
                          port))
          ((#\d) (display (priv:padding (date-day date) #\0 2)
                          port))
          ((#\D) (display (date->string date "~m/~d/~y")
                          port))
          ((#\e) (display (priv:padding (date-day date) #\space 2)
                          port))
          ;; ref Guile
          ((#\f) (receive (s ns)
                          (floor/ (+ (* (date-second date) priv:NANO)
                                     (date-nanosecond date))
                                  priv:NANO)
                   (display (number->string s) port)
                   (display priv:LOCALE-DECIMAL-POINT port)
                   (let ((str (priv:padding ns #\0 9)))
                     (display (substring str 0 1) port)
                     (display (string-trim-right str #\0 1) port))))
          ((#\H) (display (priv:padding (date-hour date)
                                        pad-with 2)
                          port))
          ((#\I) (display (priv:padding (+ 1 (modulo (- (date-hour date) 1) 12))
                                        pad-with
                                        2)
                          port))
          ((#\j) (display (priv:padding (date-year-day date)
                                        pad-with 3)
                          port))
          ((#\k) (display (priv:padding (date-hour date)
                                        #\space 2)
                          port))
          ((#\l) (display (priv:padding (+ 1 (modulo (- (date-hour date) 1) 12))
                                        #\space
                                        2)
                          port))
          ((#\m) (display (priv:padding (date-month date)
                                        pad-with 2)
                          port))
          ((#\M) (display (priv:padding (date-minute date)
                                        pad-with 2)
                          port))
          ((#\n) (newline port))
          ((#\N) (display (priv:padding (date-nanosecond date)
                                        pad-with 9)
                          port))
          ((#\p) (display (priv:locale-am/pm (date-hour date)) port))
          ((#\r) (display (date->string date "~I:~M:~S ~p") port))

          ((#\s) (error "Not Implement"))
          ((#\S) (let ((sec-delta (if (> (date-nanosecond date) priv:NANO)
                                    1 0)))
                   (display (priv:padding (+ (date-second date) sec-delta)
                                          pad-with 2)
                            port)))

          ((#\t) (display #\tab port))
          ((#\T) (display (date->string date "~H:~M:~S") port))
          ((#\U) (let* ((week>0?   (> (priv:days-before-first-week date 0) 0))
                        (week-num  (date-week-number date 0))
                        (week-num* (if week>0? (+ week-num 1) week-num)))
                   (display (priv:padding week-num* #\0 2) port)))

          ((#\V) (display (priv:padding (priv:date-week-number-iso date)
                                        #\0 2) port))
          ((#\w) (display (date-week-day date) port))
          ((#\W) (let* ((week>1?   (> (priv:days-before-first-week date 1) 0))
                        (week-num  (date-week-number date 1))
                        (week-num* (if week>1? (+ week-num 1) week-num)))
                   (display (priv:padding week-num* #\0 2) port)))
          ((#\x) (display (date->string date priv:LOCALE-SHORT-DATE-FORMAT) port))
          ((#\X) (display (date->string date priv:LOCALE-TIME-FORMAT) port))
          ((#\y) (display (priv:padding (priv:last-n-digits (date-year date) 2)
                                        pad-with 2)
                          port))
          ((#\Y) (display (priv:padding (date-year date) pad-with 4)
                          port))
          ((#\z) (priv:tz-printer (date-zone-offset date) port))
          ((#\Z) (error "Not Implement"))
          ((#\1) (display (date->string date "~Y-~m-~d") port))
          ((#\2) (display (date->string date "~H:~M:~S~z") port))
          ((#\3) (display (date->string date "~H:~M:~S") port))
          ((#\4) (display (date->string date "~Y-~m-~dT~H:~M:~S~z") port))
          ((#\5) (display (date->string date "~Y-~m-~dT~H:~M:~S") port))
          (else (priv:bad-format-error format-string)))))

    (define (priv:bad-format-error format-string)
      (value-error "bad date format string" format-string))

    (define (priv:date-printer date format-string format-string-port port)
      (let ((current-char (read-char format-string-port))
            (pad-char     (peek-char format-string-port)))
        (cond
          ((eof-object? current-char) (values))

          ((and (eof-object? pad-char)
                ;; unfinished directives
                (char=? current-char #\~))
           (priv:bad-format-error format-string))

          ((and (char=? current-char #\~)
                (or (char=? pad-char #\-) (char=? pad-char #\_)))
           (let ((pad-pad-char (begin (read-char format-string-port)
                                      (peek-char format-string-port))))
             (if (eof-object? pad-pad-char)
               (priv:bad-format-error format-string)
               (let ((formatter (priv:directives/formatter pad-pad-char format-string))
                     (pad-with  (if (char=? pad-char #\-) #f #\space)))
                 (begin (formatter date pad-with port)
                        (priv:date-printer date format-string
                                           format-string-port port))))))

          ((char=? current-char #\~)
           (let ((formatter (priv:directives/formatter pad-char format-string)))
             (begin (formatter date #\0 port)
                    (read-char format-string-port) ; consume
                    (priv:date-printer date format-string
                                       format-string-port port))))

          (else
            (display current-char port)
            (priv:date-printer date format-string
                               format-string-port port)))))

    (define* (date->string date (format-string "~c"))
      (let ((str-port (open-output-string)))
        (priv:date-printer date format-string
                   (open-input-string format-string)
                   str-port)
        (get-output-string str-port)))))
