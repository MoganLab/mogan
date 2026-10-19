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

(define-library (liii date-time internal util)
  (import (scheme base))
  (export left-pad floor/ truncate/ unix-timestamp->ntp-timestamp
    ntp-timestamp->unix-timestamp make-chain-comparator
  ) ;export
  (begin

    (define (make-chain-comparator pred type-check msg compare-impl)
      (lambda (t1 t2 . rest)
        (let ((check (lambda (t) (unless (type-check t) (error 'type-error msg t)))))
          (check t1)
          (check t2)
          (let loop
            ((prev t1) (curr t2) (rem rest))
            (and (pred (compare-impl prev curr))
              (if (null? rem)
                #t
                (let ((nxt (car rem)))
                  (check nxt)
                  (loop curr nxt (cdr rem))
                ) ;let
              ) ;if
            ) ;and
          ) ;let
        ) ;let
      ) ;lambda
    ) ;define

    (define (left-pad str c min-length)
      (let ((pad-size (max 0 (- min-length (string-length str)))))
        (if (> pad-size 0) (string-append (make-string pad-size c) str) str)
      ) ;let
    ) ;define

    (define (floor/ n d)
      (values (floor-quotient n d) (floor-remainder n d))
    ) ;define

    (define (truncate/ n d)
      (values (quotient n d) (remainder n d))
    ) ;define

    (define (unix-timestamp->ntp-timestamp unix)
      (+ unix 2208988800)
    ) ;define

    (define (ntp-timestamp->unix-timestamp ntp)
      (- ntp 2208988800)
    ) ;define

  ) ;begin
) ;define-library
