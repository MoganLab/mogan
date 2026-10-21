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

(define-library (liii date-time internal database)
  (import (scheme base)
    (scheme process-context)
    (liii date-time date)
    (liii date-time moment)
    (liii date-time time-delta)
    (liii date-time internal util)
    (liii date-time internal tz)
    (liii date-time internal tzif)
    (liii date-time internal dst)
    (liii date-time internal leap-seconds)
    (liii date-time internal date-rd)
    (liii date-time internal database-types)
    (liii date-time internal database-ranges)
    (liii date-time internal database-discover)
  ) ;import
  (export timezone? time-range time-range? time-range-start time-range-end
    time-range-offset time-range-dst? time-range-designation timezone-first
    timezone-utc-ranges timezone-local-ranges timezone-rule timezone-earliest
    timezone-latest find-offset/wall find-offset/utc utc-timezone
    utc-offset-timezone system-timezone tz-timezone tz-timezones
    reload-timezone-data!
  ) ;export
  (begin

    (define *cached-timezones* (make-vector 101 '()))
    (define *timezone-names* #f)
    (define *system-timezone-cached* #f)

    (define (convert-tz-offset offset)
      (and offset
        (let ((sign (if (tz-offset-sign-negative? offset) 1 -1)))
          (time-delta+ (hours-delta (* sign (tz-offset-hour offset)))
            (minutes-delta (* sign (tz-offset-minute offset)))
            (seconds-delta (* sign (tz-offset-second offset)))
          ) ;time-delta+
        ) ;let
      ) ;and
    ) ;define

    (define (convert-footer-rule footer)
      (and footer
        (tzrule (tz-string-stdname footer)
          (convert-tz-offset (tz-string-stdoffset footer))
          (tz-string-change-to-dst footer)
          (tz-string-dstname footer)
          (convert-tz-offset (tz-string-dstoffset footer))
          (tz-string-change-to-std footer)
        ) ;tzrule
      ) ;and
    ) ;define

    (define (load-timezone-from-tzif-data first transitions footer)
      (let-values (((utc-ranges loc-ranges) (build-ranges first transitions)))
        (let ((rule (convert-footer-rule footer)))
          (make-timezone first utc-ranges loc-ranges rule)
        ) ;let
      ) ;let-values
    ) ;define

    (define (load-timezone-from-file filepath)
      (let-values (((first transitions footer) (read-tzif-file filepath)))
        (load-timezone-from-tzif-data first transitions footer)
      ) ;let-values
    ) ;define

    (define (load-timezone-from-bytevector bv)
      (let-values (((first transitions footer) (read-tzif-bytevector bv)))
        (load-timezone-from-tzif-data first transitions footer)
      ) ;let-values
    ) ;define

    (define (hash-str str)
      (let ((len (string-length str)))
        (let loop
          ((i 0) (h 0))
          (if (= i len)
            (modulo h 101)
            (loop (+ i 1) (+ (* h 31) (char->integer (string-ref str i))))
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (get-cached-timezone name)
      (let* ((idx (hash-str name)) (bucket (vector-ref *cached-timezones* idx)))
        (cond ((assoc name bucket) => cdr)
              (else #f)
        ) ;cond
      ) ;let*
    ) ;define

    (define (cache-timezone! name tz)
      (let* ((idx (hash-str name)) (bucket (vector-ref *cached-timezones* idx)))
        (vector-set! *cached-timezones* idx (cons (cons name tz) bucket))
        tz
      ) ;let*
    ) ;define

    (define (tz-timezone name)
      (unless (string? name)
        (error 'type-error "tz-timezone: expected a string timezone name" name)
      ) ;unless
      (let ((cached (get-cached-timezone name)))
        (if cached
          cached
          (let ((bv (g_get_zoneinfo name)))
            (if bv
              (cache-timezone! name (load-timezone-from-bytevector bv))
              (let ((path (string-append "/usr/share/zoneinfo/" name)))
                (if (file-exists? path)
                  (cache-timezone! name (load-timezone-from-file path))
                  (error 'type-error "tz-timezone: unknown timezone" name)
                ) ;if
              ) ;let
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (tz-timezones)
      (unless *timezone-names*
        (set! *timezone-names* (discover-zoneinfo-names))
      ) ;unless
      *timezone-names*
    ) ;define

    (define (reload-timezone-data!)
      (set! *cached-timezones* (make-vector 101 '()))
      (set! *timezone-names* #f)
      (set! *system-timezone-cached* #f)
    ) ;define

    (define (utc-timezone)
      (let ((r (vector (time-range #f #f (seconds-delta 0) #f "UTC"))))
        (make-timezone #f r r #f)
      ) ;let
    ) ;define

    (define (utc-offset-timezone offset)
      (unless (time-delta? offset)
        (error 'type-error "utc-offset-timezone: expected a time-delta" offset)
      ) ;unless
      (unless (and (zero? (time-delta-years offset))
                (zero? (time-delta-months offset))
                (zero? (time-delta-weeks offset))
                (zero? (time-delta-days offset))
              ) ;and
        (error 'type-error
          "utc-offset-timezone: offset cannot have year/month/week/day"
          offset
        ) ;error
      ) ;unless
      (let* ((h (time-delta-hours offset))
             (m (time-delta-minutes offset))
             (s (time-delta-seconds offset))
             (neg? (or (negative? h) (negative? m) (negative? s)))
             (sign-str (if neg? "-" "+"))
             (desig
               (string-append "UTC"
                 sign-str
                 (left-pad (number->string (abs h)) #\0 2)
                 ":"
                 (left-pad (number->string (abs m)) #\0 2)
               ) ;string-append
             ) ;desig
             (r (vector (time-range #f #f offset #f desig)))
            ) ;
        (make-timezone #f r r #f)
      ) ;let*
    ) ;define

    (define (system-timezone)
      (unless *system-timezone-cached*
        (let ((env-tz (get-environment-variable "TZ")))
          (cond
           ((and env-tz (not (string=? env-tz "")))
            (set! *system-timezone-cached*
              (if (char=? (string-ref env-tz 0) #\:)
                (tz-timezone (substring env-tz 1))
                (tz-timezone env-tz)
              ) ;if
            ) ;set!
           ) ;
           ((file-exists? "/etc/localtime")
            (set! *system-timezone-cached* (load-timezone-from-file "/etc/localtime"))
           ) ;
           (else
             (let ((mins (g_system_timezone_offset_minutes)))
               (set! *system-timezone-cached* (utc-offset-timezone (minutes-delta mins)))
             ) ;let
           ) ;else
          ) ;cond
        ) ;let
      ) ;unless
      *system-timezone-cached*
    ) ;define

    (define (timezone-earliest tz)
      (let* ((first (vector-ref (timezone-utc-ranges tz) 0)) (utc (time-range-end first)))
        (let-values (((_ tai-utc) (leapsecond-info/utc (or utc 0))))
          (let-values (((days sec) (floor/ (+ (or utc 0) tai-utc) 86400)))
            (make-moment (rata-die->date (+ days unix-epoch-rd)) sec)
          ) ;let-values
        ) ;let-values
      ) ;let*
    ) ;define

    (define (timezone-latest tz)
      (let* ((ranges (timezone-utc-ranges tz))
             (last (vector-ref ranges (- (vector-length ranges) 1)))
             (utc (time-range-start last))
            ) ;
        (let-values (((_ tai-utc) (leapsecond-info/utc (or utc 0))))
          (let-values (((days sec) (floor/ (+ (or utc 0) tai-utc) 86400)))
            (make-moment (rata-die->date (+ days unix-epoch-rd)) sec)
          ) ;let-values
        ) ;let-values
      ) ;let*
    ) ;define

  ) ;begin
) ;define-library
