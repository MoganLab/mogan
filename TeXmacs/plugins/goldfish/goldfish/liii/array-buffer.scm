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

(define-library (liii array-buffer)
  (import (liii lang) (liii error))
  (export array-buffer)
  (begin

    (define-case-class array-buffer
      ((data vector?)
       (size integer?)
       (capacity integer?)
      ) ;

      (chained-define (@from-vector vec)
        (let ((len (vector-length vec)))
          (array-buffer (copy vec) len len)
        ) ;let
      ) ;chained-define

      (chained-define (@from-list lst)
        (let ((len (length lst)))
          (array-buffer (copy lst (make-vector len)) len len)
        ) ;let
      ) ;chained-define

      (typed-define (check-bound (n integer?))
        (when (or (< n 0) (>= n size))
          (index-error
            ($ "access No." :+ n :+ " of array-buffer [0:" :+ size :+ ")" :get)
          ) ;index-error
        ) ;when
      ) ;typed-define

      (define (%collect)
        (copy data (make-vector size))
      ) ;define

      (define (%length) size)

      (define (%apply n)
        (check-bound n)
        (vector-ref data n)
      ) ;define

      (chained-define (%set! n v)
        (check-bound n)
        (vector-set! data n v)
        (%this)
      ) ;chained-define

      (define (%update! . args)
        (apply %set! args)
      ) ;define

      (chained-define (%extend! n)
        (when (< capacity n)
          (if (= capacity 0)
            (set! capacity n)
            (let loop ()
              (when (< capacity n)
                (set! capacity (* 2 capacity))
                (loop)
              ) ;when
            ) ;let
          ) ;if
          (set! data (copy data (make-vector capacity) 0 size))
        ) ;when
        (%this)
      ) ;chained-define

      (define (%size-hint! . args) (apply %extend! args))

      (chained-define (%resize! n)
        (%extend! n)
        (set! size n)
        (%this)
      ) ;chained-define

      (chained-define (%trim-to-size! n)
        (%extend! n)
        (set! size n)
        (when (> capacity (* 2 size))
          (set! data (copy data (make-vector size)))
          (set! capacity size)
        ) ;when
        (%this)
      ) ;chained-define

      (chained-define (%add-one! x)
        (%extend! (+ size 1))
        (vector-set! data size x)
        (set! size (+ size 1))
        (%this)
      ) ;chained-define

      (chained-define (%clear!)
        (set! size 0)
        (%this)
      ) ;chained-define

      (chained-define (%clear/shrink!)
        (set! size 0)
        (set! capacity 1)
        (set! data (make-vector 1))
        (%this)
      ) ;chained-define

      (chained-define (%insert! index elem)
        (%extend! (+ size 1))
        (set! size (+ size 1))
        (check-bound index)
        (let loop ((p (- size 1)))
          (when (> p index)
            (vector-set! data p (vector-ref data (- p 1)))
            (loop (- p 1))
          ) ;when
        ) ;let
        (vector-set! data index elem)
        (%this)
      ) ;chained-define

      (typed-define (%equals (that case-class?))
        (and (that :is-instance-of 'array-buffer)
             ((%to-vector) :equals (that :to-vector))
        ) ;and
      ) ;typed-define

      (define (%to-vector)
        (rich-vector (copy data (make-vector size)))
      ) ;define

      (define (%to-list)
        (vector->list data 0 size)
      ) ;define

      (define (%to-rich-list)
        (box (%to-list))
      ) ;define

    ) ;define-case-class

  ) ;begin
) ;define-library

