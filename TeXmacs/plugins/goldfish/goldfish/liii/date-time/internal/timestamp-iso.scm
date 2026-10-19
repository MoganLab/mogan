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

(define-library (liii date-time internal timestamp-iso)
  (import (scheme base)
    (liii date-time date)
    (liii date-time clock-time)
    (liii date-time time-delta)
    (liii date-time timezone)
    (liii date-time internal util)
    (liii date-time internal timestamp-types)
    (liii date-time internal timestamp-conv)
  ) ;import
  (export timestamp->iso-8601 iso-8601->timestamp)
  (begin

    (define (format-second-iso s)
      (let* ((int-part (floor s))
             (frac-part (- s int-part))
             (base-str (left-pad (number->string (exact int-part)) #\0 2))
            ) ;
        (if (zero? frac-part)
          base-str
          (let* ((frac-flt (exact->inexact frac-part)) (s-str (number->string frac-flt)))
            (string-append base-str (substring s-str 1))
          ) ;let*
        ) ;if
      ) ;let*
    ) ;define

    (define (timestamp->iso-8601 ts)
      (unless (timestamp? ts)
        (error 'type-error "timestamp->iso-8601: expected a timestamp" ts)
      ) ;unless
      (let* ((off (timestamp-timezone-offset ts))
             (h (time-delta-hours off))
             (m (time-delta-minutes off))
             (tz-str
               (if (and (zero? h) (zero? m))
                 "Z"
                 (let ((sign (if (negative? h) "-" "+")))
                   (string-append sign
                     (left-pad (number->string (abs h)) #\0 2)
                     ":"
                     (left-pad (number->string (abs m)) #\0 2)
                   ) ;string-append
                 ) ;let
               ) ;if
             ) ;tz-str
            ) ;
        (string-append (date->iso-8601 (%timestamp-date ts))
          "T"
          (left-pad (number->string (timestamp-hour ts)) #\0 2)
          ":"
          (left-pad (number->string (timestamp-minute ts)) #\0 2)
          ":"
          (format-second-iso (timestamp-second ts))
          tz-str
        ) ;string-append
      ) ;let*
    ) ;define

    (define (string-index str pred)
      (let loop
        ((i 0))
        (cond ((= i (string-length str)) #f)
              ((pred (string-ref str i)) i)
              (else (loop (+ i 1)))
        ) ;cond
      ) ;let
    ) ;define

    (define (string-index-char str ch)
      (string-index str (lambda (c) (char=? c ch)))
    ) ;define

    (define (find-tz-sign str)
      (let loop
        ((i (- (string-length str) 1)))
        (cond ((< i 0) 0)
              ((or (char=? (string-ref str i) #\+) (char=? (string-ref str i) #\-)) i)
              (else (loop (- i 1)))
        ) ;cond
      ) ;let
    ) ;define

    (define (parse-time-and-tz time-rest)
      (cond
       ((string=? (substring time-rest (- (string-length time-rest) 1)) "Z")
        (values (substring time-rest 0 (- (string-length time-rest) 1)) (utc-timezone))
       ) ;
       ((or (string-index-char time-rest #\+) (string-index-char time-rest #\-))
        (let* ((sign-idx (find-tz-sign time-rest))
               (t-str (substring time-rest 0 sign-idx))
               (tz-str (substring time-rest sign-idx))
               (sign (if (char=? (string-ref tz-str 0) #\-) -1 1))
               (tz-body (substring tz-str 1))
               (colon (string-index-char tz-body #\:))
              ) ;
          (if colon
            (let ((zh (string->number (substring tz-body 0 colon)))
                  (zm (string->number (substring tz-body (+ colon 1))))
                 ) ;
              (values t-str
                (utc-offset-timezone (time-delta+ (hours-delta (* sign zh)) (minutes-delta (* sign zm)))
                ) ;utc-offset-timezone
              ) ;values
            ) ;let
            (values t-str (utc-timezone))
          ) ;if
        ) ;let*
       ) ;
       (else (values time-rest (utc-timezone)))
      ) ;cond
    ) ;define

    (define (iso-8601->timestamp str)
      (unless (string? str)
        (error 'type-error "iso-8601->timestamp: expected a string" str)
      ) ;unless
      (let ((t-pos (string-index str (lambda (c) (or (char=? c #\T) (char=? c #\t))))))
        (unless t-pos
          (error 'type-error "iso-8601->timestamp: invalid format (missing T)" str)
        ) ;unless
        (let* ((date-part (substring str 0 t-pos))
               (time-rest (substring str (+ t-pos 1)))
               (d (iso-8601->date date-part))
              ) ;
          (let-values (((time-str tz) (parse-time-and-tz time-rest)))
            (let ((clk (iso-8601->clock-time time-str)))
              (date+clock-time->timestamp d clk tz)
            ) ;let
          ) ;let-values
        ) ;let*
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
