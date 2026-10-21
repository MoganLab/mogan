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

(define-library (liii date-time timestamp)
  (import (scheme base)
    (liii date-time internal timestamp-types)
    (liii date-time internal timestamp-construct)
    (liii date-time internal timestamp-conv)
    (liii date-time internal timestamp-iso)
    (liii date-time internal timestamp-cmp)
  ) ;import
  (export make-timestamp date+clock-time->timestamp timestamp? timestamp-date
    timestamp-clock-time timestamp-timezone timestamp-fold timestamp-year
    timestamp-month timestamp-day timestamp-ymd timestamp-hour timestamp-minute
    timestamp-second timestamp-hms moment->timestamp timestamp->moment
    posix-time->utc-timestamp timestamp->utc-posix-time timestamp-in-timezone
    timestamp-timezone-offset timestamp-timezone-dst?
    timestamp-timezone-abbreviation timestamp->iso-8601 iso-8601->timestamp
    timestamp+ timestamp- timestamp=? timestamp<? timestamp<=? timestamp>?
    timestamp>=?
  ) ;export
  (begin

    (define timestamp=? (make-timestamp-comparator zero?))
    (define timestamp<? (make-timestamp-comparator negative?))
    (define timestamp<=? (make-timestamp-comparator (lambda (c) (<= c 0))))
    (define timestamp>? (make-timestamp-comparator positive?))
    (define timestamp>=? (make-timestamp-comparator (lambda (c) (>= c 0))))

  ) ;begin
) ;define-library
