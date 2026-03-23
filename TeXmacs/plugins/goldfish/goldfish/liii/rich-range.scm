;
; Copyright (C) 2024 The Goldfish Scheme Authors
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;

(define-library (liii rich-range)
  (import (liii oop)
          (liii rich-list)
          (liii error)
  ) ;import
  (export rich-range)
  (begin

    (define-case-class rich-range
      ((start integer?) (end integer?) (step integer? 1) (inclusive? boolean? #f))

      (define* (@inclusive start end (step 1))
        (rich-range start end step #t)
      ) ;define*

      (define (check-step)
        (when (zero? step)
          (value-error "step can't be zero")
        ) ;when
      ) ;define

      (define (in-range? x)
        (or (and (> step 0) (if inclusive? (and (<= x end) (>= x start)) (and (< x end) (>= x start))))
            (and (< step 0) (if inclusive? (and (>= x end) (<= x start)) (and (> x end) (<= x start))))
        ) ;or
      ) ;define

      (define (not-in-range? x)
        (or (and (> step 0) (or (> x end) (< x start)))
            (and (< step 0) (or (< x end) (> x start)))
            (and (= x end) (not inclusive?))
        ) ;or
      ) ;define

      (define (%empty?)
        (check-step)
        (or (and (> start end) (> step 0))
            (and (< start end) (< step 0))
            (and (= start end) (not inclusive?))
        ) ;or
      ) ;define

      (define (%map map-func)
        (if (%empty?)
            (rich-list :empty)
            (let loop ((current start) (result '()))
              (if (not-in-range? current)
                  (rich-list (reverse result))
                  (loop (+ current step)
                        (cons (map-func current) result)
                  ) ;loop
              ) ;if
            ) ;let
        ) ;if
      ) ;define

      (define (%for-each proc)
        (when (not (%empty?))
          (let loop ((current start))
               (when (in-range? current)
                     (proc current)
                     (loop (+ current step))
               ) ;when
          ) ;let
        ) ;when
      ) ;define

      (define (%filter f)
        (if (%empty?)
            (rich-list :empty)
            (let loop ((i start) (return '()))
              (if (not-in-range? i)
                  (rich-list (reverse return))
                  (loop (+ i step)
                        (if (f i)
                            (cons i return)
                            return
                        ) ;if
                  ) ;loop
              ) ;if
            ) ;let
        ) ;if
      ) ;define

      (define (%contains elem)
        (check-step)
        (if (%empty?)
            #f
            (if (in-range? elem) ;判断是否在范围内
                (zero? (modulo (- elem start) (abs step)))
                #f
            ) ;if
        ) ;if
      ) ;define




    ) ;define-case-class
  ) ;begin
) ;define-library

