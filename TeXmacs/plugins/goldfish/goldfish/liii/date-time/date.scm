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

(define-library (liii date-time date)
  (import (scheme base)
    (liii date-time internal date-cmp)
    (liii date-time internal date-iso)
    (liii date-time internal date-rd)
    (liii date-time internal date-types)
    (liii date-time internal date-week)
  ) ;import
  (export make-date date? date-year date-month date-day date-ymd date->iso-8601
    iso-8601->date date-iso-weekday date-iso-week date-iso-week-year
    date->rata-die rata-die->date date->mjd mjd->date date=? date<? date<=?
    date>? date>=? leap-year? days-in-month-raw
  ) ;export
  (begin

    (define (date->iso-8601 d)
      (format-date-iso-8601 d)
    ) ;define

    (define (iso-8601->date str)
      (parse-date-iso-8601 str)
    ) ;define

    (define (date-iso-weekday d)
      (calc-date-iso-weekday d)
    ) ;define

    (define (date-iso-week d)
      (calc-date-iso-week d)
    ) ;define

    (define (date-iso-week-year d)
      (calc-date-iso-week-year d)
    ) ;define

    (define (date->rata-die d)
      (calc-date->rata-die d)
    ) ;define

    (define (rata-die->date rd)
      (calc-rata-die->date rd)
    ) ;define

    (define (date->mjd d)
      (calc-date->mjd d)
    ) ;define

    (define (mjd->date mjd)
      (calc-mjd->date mjd)
    ) ;define

    (define date=? (make-date-comparator zero?))
    (define date<? (make-date-comparator negative?))
    (define date<=? (make-date-comparator (lambda (c) (<= c 0))))
    (define date>? (make-date-comparator positive?))
    (define date>=? (make-date-comparator (lambda (c) (>= c 0))))

  ) ;begin
) ;define-library
