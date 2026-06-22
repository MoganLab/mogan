(define-library (liii time)
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
    ;; Base
    sleep
  ) ;export
  (import (liii base) (scheme time) (srfi srfi-19))
  (begin

    (define (sleep seconds)
      (if (not (number? seconds))
        (error 'type-error "(sleep seconds): seconds must be a number")
        (g_sleep seconds)
      ) ;if
    ) ;define

  ) ;begin
) ;define-library
