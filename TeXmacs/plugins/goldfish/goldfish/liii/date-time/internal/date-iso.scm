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

(define-library (liii date-time internal date-iso)
  (import (scheme base)
    (liii date-time internal util)
    (liii date-time internal date-types)
  ) ;import
  (export format-date-iso-8601 parse-date-iso-8601)
  (begin

    (define (format-date-iso-8601 d)
      (unless (date? d)
        (error 'type-error "date->iso-8601: expected a date" d)
      ) ;unless
      (let-values (((y m day) (date-ymd d)))
        (string-append (if (< y 0) "-" "")
          (left-pad (number->string (abs y)) #\0 4)
          "-"
          (left-pad (number->string m) #\0 2)
          "-"
          (left-pad (number->string day) #\0 2)
        ) ;string-append
      ) ;let-values
    ) ;define

    (define (split-hyphen str)
      (let loop
        ((i 0) (start 0) (acc '()))
        (cond
         ((= i (string-length str)) (reverse (cons (substring str start i) acc)))
         ((char=? (string-ref str i) #\-)
          (loop (+ i 1) (+ i 1) (cons (substring str start i) acc))
         ) ;
         (else (loop (+ i 1) start acc))
        ) ;cond
      ) ;let
    ) ;define

    (define (parse-date-iso-8601 str)
      (unless (string? str)
        (error 'type-error "iso-8601->date: expected a string" str)
      ) ;unless
      (let* ((len (string-length str))
             (is-neg (and (> len 0) (char=? (string-ref str 0) #\-)))
             (rem-str (if is-neg (substring str 1 len) str))
             (parts (split-hyphen rem-str))
            ) ;
        (unless (= (length parts) 3)
          (error 'type-error "iso-8601->date: invalid format" str)
        ) ;unless
        (let ((raw-y (string->number (car parts)))
              (m (string->number (cadr parts)))
              (d (string->number (caddr parts)))
             ) ;
          (unless (and raw-y
                    m
                    d
                    (integer? raw-y)
                    (exact? raw-y)
                    (integer? m)
                    (exact? m)
                    (integer? d)
                    (exact? d)
                  ) ;and
            (error 'type-error "iso-8601->date: invalid numbers" str)
          ) ;unless
          (make-date (if is-neg (- raw-y) raw-y) m d)
        ) ;let
      ) ;let*
    ) ;define

  ) ;begin
) ;define-library
