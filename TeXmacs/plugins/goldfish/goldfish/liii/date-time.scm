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

(define-library (liii date-time)
  (import (scheme base)
    (liii date-time date)
    (liii date-time gregorian)
    (liii date-time clock-time)
    (liii date-time moment)
    (liii date-time time-delta)
    (liii date-time timezone)
    (liii date-time tzdb)
    (liii date-time timestamp)
    (liii date-time current-time)
  ) ;import
  (export
    ;; date
    make-date
    date?
    date-year
    date-month
    date-day
    date-ymd
    date->iso-8601
    iso-8601->date
    date-iso-weekday
    date-iso-week
    date-iso-week-year
    date->rata-die
    rata-die->date
    date->mjd
    mjd->date
    date=?
    date<?
    date<=?
    date>?
    date>=?

    ;; gregorian
    leap-year?
    days-in-month
    day-name->weekday
    first-in-month
    last-in-month
    nth-in-month

    ;; clock-time
    make-clock-time
    clock-time?
    clock-time-hour
    clock-time-minute
    clock-time-second
    clock-time-hms
    clock-time->iso-8601
    iso-8601->clock-time
    clock-time=?
    clock-time<?
    clock-time<=?
    clock-time>?
    clock-time>=?

    ;; moment
    make-moment
    moment?
    moment-date
    moment-second-of-day
    moment=?
    moment<?
    moment<=?
    moment>?
    moment>=?

    ;; time-delta
    make-time-delta
    time-delta?
    time-delta-years
    time-delta-months
    time-delta-weeks
    time-delta-days
    time-delta-hours
    time-delta-minutes
    time-delta-seconds
    years-delta
    months-delta
    weeks-delta
    days-delta
    hours-delta
    minutes-delta
    seconds-delta
    time-delta+
    time-delta-
    time-delta-negate
    date+
    date-

    ;; timezone
    timezone?
    utc-timezone
    utc-offset-timezone
    system-timezone
    tz-timezone
    timezone-earliest
    timezone-latest

    ;; tzdb
    tz-timezones
    reload-timezone-data!

    ;; timestamp
    make-timestamp
    date+clock-time->timestamp
    timestamp?
    timestamp-date
    timestamp-clock-time
    timestamp-timezone
    timestamp-fold
    timestamp-year
    timestamp-month
    timestamp-day
    timestamp-ymd
    timestamp-hour
    timestamp-minute
    timestamp-second
    timestamp-hms
    moment->timestamp
    timestamp->moment
    posix-time->utc-timestamp
    timestamp->utc-posix-time
    timestamp-in-timezone
    timestamp-timezone-offset
    timestamp-timezone-dst?
    timestamp-timezone-abbreviation
    timestamp->iso-8601
    iso-8601->timestamp
    timestamp+
    timestamp-
    timestamp=?
    timestamp<?
    timestamp<=?
    timestamp>?
    timestamp>=?

    ;; current-time
    current-moment
    current-utc-timestamp
    current-system-timestamp
  ) ;export
  (begin
  ) ;begin
) ;define-library
