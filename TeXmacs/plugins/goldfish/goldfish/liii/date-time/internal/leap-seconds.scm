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

(define-library (liii date-time internal leap-seconds)
  (import (scheme base) (liii date-time internal util))
  (export leapsecond-info/tai leapsecond-info/utc default-leap-seconds)
  (begin

    (define default-raw-leap-seconds
      '((2272060800 . 10)
        (2287785600 . 11)
        (2303683200 . 12)
        (2335219200 . 13)
        (2366755200 . 14)
        (2398291200 . 15)
        (2429913600 . 16)
        (2461449600 . 17)
        (2492985600 . 18)
        (2524521600 . 19)
        (2571782400 . 20)
        (2603318400 . 21)
        (2634854400 . 22)
        (2698012800 . 23)
        (2776982400 . 24)
        (2840140800 . 25)
        (2871676800 . 26)
        (2918937600 . 27)
        (2950473600 . 28)
        (2982009600 . 29)
        (3029443200 . 30)
        (3076704000 . 31)
        (3124137600 . 32)
        (3345062400 . 33)
        (3439756800 . 34)
        (3550089600 . 35)
        (3644697600 . 36)
        (3692217600 . 37))
    ) ;define

    (define default-leap-seconds
      (list->vector
        (map
          (lambda (pair) (cons (ntp-timestamp->unix-timestamp (car pair)) (cdr pair)))
          default-raw-leap-seconds
        ) ;map
      ) ;list->vector
    ) ;define

    ;; returns (values is-leap? tai-offset)
    ;; posix-utc is seconds since 1970-01-01 in UTC
    (define (leapsecond-info/utc posix-utc)
      (let ((v default-leap-seconds) (len (vector-length default-leap-seconds)))
        (let loop
          ((i 0) (last-offset 10))
          (if (= i len)
            (values #f last-offset)
            (let* ((entry (vector-ref v i)) (t (car entry)) (offset (cdr entry)))
              (cond ((= (floor posix-utc) t) (values #t last-offset))
                    ((< posix-utc t) (values #f last-offset))
                    (else (loop (+ i 1) offset))
              ) ;cond
            ) ;let*
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (leapsecond-info/tai posix-tai)
      (let ((v default-leap-seconds) (len (vector-length default-leap-seconds)))
        (let loop
          ((i 0) (last-offset 10))
          (if (= i len)
            (values #f last-offset)
            (let* ((entry (vector-ref v i))
                   (t (car entry))
                   (offset (cdr entry))
                   (tai-t (+ t offset))
                  ) ;
              (cond ((< posix-tai tai-t) (values #f last-offset))
                    (else (loop (+ i 1) offset))
              ) ;cond
            ) ;let*
          ) ;if
        ) ;let
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
