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

(define-library (liii date-time internal timestamp-construct)
  (import (scheme base)
    (scheme case-lambda)
    (liii date-time date)
    (liii date-time clock-time)
    (liii date-time time-delta)
    (liii date-time timezone)
    (liii date-time internal leap-seconds)
    (liii date-time internal database)
    (liii date-time internal timestamp-types)
    (liii date-time internal timestamp-math)
  ) ;import
  (export make-timestamp date+clock-time->timestamp timestamp-in-utc ts-no-rules
    validate-leapsecond
  ) ;export
  (begin

    (define (ts-no-rules ts dt)
      (timestamp+/no-rules ts dt)
    ) ;define

    (define (validate-time-in-tz ts)
      (let* ((local-tp (timestamp->local-timepoint ts))
             (tz (%timestamp-timezone ts))
             (offset (find-offset/wall tz local-tp))
             (fold (%timestamp-fold ts))
            ) ;
        (cond ((= 0 (vector-length offset))
               (error 'type-error
                 "date+clock-time->timestamp: invalid local time for timezone"
               ) ;error
              ) ;
              ((and (= 1 fold) (< (vector-length offset) 2))
               (error 'type-error "date+clock-time->timestamp: invalid fold for timestamp")
              ) ;
              (else #t)
        ) ;cond
      ) ;let*
    ) ;define

    (define (validate-leapsecond ts ok err)
      (if (< (timestamp-second ts) 60)
        (ok)
        (let* ((ts-utc (timestamp-in-utc ts))
               (tp (floor (timestamp->local-timepoint ts-utc)))
              ) ;
          (let-values (((leap? off) (leapsecond-info/utc tp)))
            (if leap? (ok) (err))
          ) ;let-values
        ) ;let*
      ) ;if
    ) ;define

    (define date+clock-time->timestamp
      (case-lambda
       ((date time timezone) (date+clock-time->timestamp date time timezone 0))
       ((date time timezone fold)
        (unless (and (date? date)
                  (clock-time? time)
                  (timezone? timezone)
                  (integer? fold)
                  (or (= fold 0) (= fold 1))
                ) ;and
          (error 'type-error
            "date+clock-time->timestamp: invalid parameters"
            date
            time
            timezone
            fold
          ) ;error
        ) ;unless
        (let ((ts (%make-timestamp date time timezone fold)))
          (validate-time-in-tz ts)
          (validate-leapsecond ts
            (lambda () #t)
            (lambda ()
              (error 'type-error
                "date+clock-time->timestamp: invalid leap second"
                date
                time
                timezone
                fold
              ) ;error
            ) ;lambda
          ) ;validate-leapsecond
          ts
        ) ;let
       ) ;
      ) ;case-lambda
    ) ;define

    (define make-timestamp
      (case-lambda
       ((year month day hour minute second timezone)
        (date+clock-time->timestamp (make-date year month day)
          (make-clock-time hour minute second)
          timezone
          0
        ) ;date+clock-time->timestamp
       ) ;
       ((year month day hour minute second timezone fold)
        (date+clock-time->timestamp (make-date year month day)
          (make-clock-time hour minute second)
          timezone
          fold
        ) ;date+clock-time->timestamp
       ) ;
      ) ;case-lambda
    ) ;define

    (define (timestamp-in-utc ts)
      (let* ((dt (time-delta-negate (timestamp-timezone-offset ts)))
             (ts-adj (ts-no-rules ts dt))
            ) ;
        (%make-timestamp (%timestamp-date ts-adj)
          (%timestamp-clock-time ts-adj)
          (utc-timezone)
          0
        ) ;%make-timestamp
      ) ;let*
    ) ;define

  ) ;begin
) ;define-library
