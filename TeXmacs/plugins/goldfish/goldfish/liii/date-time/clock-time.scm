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
;; Copyright (C) 2024-2026 Arvydas Silanskas
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

(define-library (liii date-time clock-time)
  (import (scheme base))
  (export make-clock-time clock-time? clock-time-hour clock-time-minute
    clock-time-second clock-time-hms clock-time->iso-8601 iso-8601->clock-time
    clock-time=? clock-time<? clock-time<=? clock-time>? clock-time>=?
  ) ;export
  (begin

    (define-record-type <clock-time>
      (%make-clock-time hour minute second)
      clock-time?
      (hour %clock-time-hour)
      (minute %clock-time-minute)
      (second %clock-time-second)
    ) ;define-record-type

    (define (valid-clock-time? h m s)
      (and (integer? h)
        (exact? h)
        (<= 0 h 23)
        (integer? m)
        (exact? m)
        (<= 0 m 59)
        (rational? s)
        (exact? s)
        (<= 0 s)
        (< s 61)
      ) ;and
    ) ;define

    (define (make-clock-time hour minute second)
      (unless (valid-clock-time? hour minute second)
        (error 'type-error "make-clock-time: invalid parameters" hour minute second)
      ) ;unless
      (%make-clock-time hour minute second)
    ) ;define

    (define (clock-time-hour ct)
      (unless (clock-time? ct)
        (error 'type-error "clock-time-hour: expected a clock-time" ct)
      ) ;unless
      (%clock-time-hour ct)
    ) ;define

    (define (clock-time-minute ct)
      (unless (clock-time? ct)
        (error 'type-error "clock-time-minute: expected a clock-time" ct)
      ) ;unless
      (%clock-time-minute ct)
    ) ;define

    (define (clock-time-second ct)
      (unless (clock-time? ct)
        (error 'type-error "clock-time-second: expected a clock-time" ct)
      ) ;unless
      (%clock-time-second ct)
    ) ;define

    (define (clock-time-hms ct)
      (unless (clock-time? ct)
        (error 'type-error "clock-time-hms: expected a clock-time" ct)
      ) ;unless
      (values (%clock-time-hour ct) (%clock-time-minute ct) (%clock-time-second ct))
    ) ;define

    (define (left-pad str c min-length)
      (let ((pad-size (max 0 (- min-length (string-length str)))))
        (if (> pad-size 0) (string-append (make-string pad-size c) str) str)
      ) ;let
    ) ;define

    (define (clock-time->iso-8601 ct)
      (unless (clock-time? ct)
        (error 'type-error "clock-time->iso-8601: expected a clock-time" ct)
      ) ;unless
      (let-values (((h m s) (clock-time-hms ct)))
        (let ((sec-str
                (if (integer? s)
                  (left-pad (number->string s) #\0 2)
                  (let* ((int-part (floor s))
                         (frac-part (- s int-part))
                         (frac-val (exact->inexact frac-part))
                         (frac-str (substring (number->string frac-val) 1))
                        ) ;
                    (string-append (left-pad (number->string (exact int-part)) #\0 2) frac-str)
                  ) ;let*
                ) ;if
              ) ;sec-str
             ) ;
          (string-append (left-pad (number->string h) #\0 2)
            ":"
            (left-pad (number->string m) #\0 2)
            ":"
            sec-str
          ) ;string-append
        ) ;let
      ) ;let-values
    ) ;define

    (define (iso-8601->clock-time str)
      (unless (string? str)
        (error 'type-error "iso-8601->clock-time: expected a string" str)
      ) ;unless
      (let ((parts
              (let loop
                ((i 0) (start 0) (acc '()))
                (cond
                 ((= i (string-length str)) (reverse (cons (substring str start i) acc)))
                 ((char=? (string-ref str i) #\:)
                  (loop (+ i 1) (+ i 1) (cons (substring str start i) acc))
                 ) ;
                 (else (loop (+ i 1) start acc))
                ) ;cond
              ) ;let
            ) ;parts
           ) ;
        (unless (= (length parts) 3)
          (error 'type-error "iso-8601->clock-time: invalid format" str)
        ) ;unless
        (let ((h-str (car parts)) (m-str (cadr parts)) (s-str (caddr parts)))
          (let ((h (string->number h-str))
                (m (string->number m-str))
                (s (string->number s-str))
               ) ;
            (unless (and h m s (integer? h) (exact? h) (integer? m) (exact? m) (real? s))
              (error 'type-error "iso-8601->clock-time: invalid numbers" str)
            ) ;unless
            (make-clock-time h m (inexact->exact s))
          ) ;let
        ) ;let
      ) ;let
    ) ;define

    (define (%clock-time-compare-impl ct1 ct2)
      (let-values (((h1 m1 s1) (clock-time-hms ct1)) ((h2 m2 s2) (clock-time-hms ct2)))
        (cond ((< h1 h2) -1)
              ((> h1 h2) 1)
              ((< m1 m2) -1)
              ((> m1 m2) 1)
              ((< s1 s2) -1)
              ((> s1 s2) 1)
              (else 0)
        ) ;cond
      ) ;let-values
    ) ;define

    (define (make-transitive-comparator pred)
      (lambda (ct1 ct2 . rest)
        (unless (clock-time? ct1)
          (error 'type-error "clock-time comparison: expected a clock-time" ct1)
        ) ;unless
        (unless (clock-time? ct2)
          (error 'type-error "clock-time comparison: expected a clock-time" ct2)
        ) ;unless
        (let loop
          ((prev ct1) (curr ct2) (rem rest))
          (and (pred (%clock-time-compare-impl prev curr))
            (if (null? rem)
              #t
              (let ((nxt (car rem)))
                (unless (clock-time? nxt)
                  (error 'type-error "clock-time comparison: expected a clock-time" nxt)
                ) ;unless
                (loop curr nxt (cdr rem))
              ) ;let
            ) ;if
          ) ;and
        ) ;let
      ) ;lambda
    ) ;define

    (define clock-time=? (make-transitive-comparator zero?))
    (define clock-time<? (make-transitive-comparator negative?))
    (define clock-time<=? (make-transitive-comparator (lambda (c) (<= c 0))))
    (define clock-time>? (make-transitive-comparator positive?))
    (define clock-time>=? (make-transitive-comparator (lambda (c) (>= c 0))))

  ) ;begin
) ;define-library
