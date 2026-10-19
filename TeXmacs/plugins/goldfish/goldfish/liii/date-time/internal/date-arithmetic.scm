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

(define-library (liii date-time internal date-arithmetic)
  (import (scheme base) (liii date-time date))
  (export date-add-components)
  (begin

    (define (add-months year month month+)
      (let* ((m (+ (- month 1) month+))
             (y+ (floor-quotient m 12))
             (rem-m (floor-remainder m 12))
            ) ;
        (values (+ year y+) (+ 1 rem-m))
      ) ;let*
    ) ;define

    (define (add-days year month day day+)
      (if (zero? day+)
        (make-date year month day)
        (let* ((base (make-date year month day)) (rd (+ (date->rata-die base) day+)))
          (rata-die->date rd)
        ) ;let*
      ) ;if
    ) ;define

    (define (date-add-components date y-plus m-plus w-plus d-plus)
      (let-values (((y m d) (date-ymd date)))
        (let* ((y (+ y y-plus)))
          (let-values (((y2 m2) (add-months y m m-plus)))
            (let* ((max-d (days-in-month-raw (leap-year? y2) m2)) (d2 (min d max-d)))
              (add-days y2 m2 d2 (+ (* 7 w-plus) d-plus))
            ) ;let*
          ) ;let-values
        ) ;let*
      ) ;let-values
    ) ;define

  ) ;begin
) ;define-library
