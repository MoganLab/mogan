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
; SRFI-158: Generators and Accumulators
;
; Based on SRFI-158 reference implementation:
; Copyright (C) Shiro Kawai, John Cowan, Thomas Gilray (2015). All Rights Reserved.
;
; Permission is hereby granted, free of charge, to any person obtaining a copy of 
; this software and associated documentation files (the "Software"), to deal in 
; the Software without restriction, including without limitation the rights to 
; use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
; of the Software, and to permit persons to whom the Software is furnished to do 
; so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all 
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
; SOFTWARE.

(define-library (srfi srfi-158)
  (import (srfi srfi-16))
  (export generator
          circular-generator
          make-iota-generator
          make-range-generator
          make-coroutine-generator
          make-for-each-generator
          make-unfold-generator

          list->generator
          vector->generator
          reverse-vector->generator
          string->generator
          bytevector->generator

          generator->list
          generator->reverse-list
          generator->vector
          generator->vector!
          generator->string
          generator-map->list

          gcons*
          gappend
          gflatten
          ggroup
          gmerge
          gmap
          gcombine
          gfilter
          gremove
          gstate-filter
          gtake
          gdrop
          gtake-while
          gdrop-while
          gdelete
          gdelete-neighbor-dups
          gindex
          gselect
          generator-fold
          generator-for-each
          generator-find
          generator-count
          generator-any
          generator-every
          generator-unfold

          make-accumulator
          list-accumulator
          reverse-list-accumulator
          vector-accumulator
          reverse-vector-accumulator
          vector-accumulator!
          string-accumulator
          bytevector-accumulator
          bytevector-accumulator!
          sum-accumulator
          product-accumulator
  ) ;export

  (begin
    ;; Chibi Scheme version of any
    (define (any pred ls)
      (if (null? (cdr ls))
        (pred (car ls))
        ((lambda (x) (if x x (any pred (cdr ls))))
         (pred (car ls))
        ) ;
      ) ;if
    ) ;define

    ;; list->bytevector
    (define (list->bytevector list)
      (let ((vec (make-bytevector (length list) 0)))
        (let loop ((i 0) (list list))
          (if (null? list)
            vec
            (begin
              (bytevector-u8-set! vec i (car list))
              (loop (+ i 1) (cdr list))
            ) ;begin
          ) ;if
        ) ;let
      ) ;let
    ) ;define


    ;; generator
    (define (generator . args)
      (lambda () (if (null? args)
                   (eof-object)
                   (let ((next (car args)))
                     (set! args (cdr args))
                     next)
                   ) ;let
      ) ;lambda
    ) ;define

    ;; circular-generator
    (define (circular-generator . args)
      (let ((base-args args))
        (lambda ()
          (when (null? args)
            (set! args base-args)
          ) ;when
          (let ((next (car args)))
            (set! args (cdr args))
            next
          ) ;let
        ) ;lambda
      ) ;let
    ) ;define


    ;; make-iota-generator
    (define make-iota-generator
      (case-lambda ((count) (make-iota-generator count 0 1))
                   ((count start) (make-iota-generator count start 1))
                   ((count start step) (make-iota count start step))
      ) ;case-lambda
    ) ;define

    ;; make-iota
    (define (make-iota count start step)
      (lambda ()
        (cond
          ((<= count 0)
           (eof-object)
          ) ;
          (else
            (let ((result start))
             (set! count (- count 1))
             (set! start (+ start step))
             result
            ) ;let
          ) ;else
        ) ;cond
      ) ;lambda
    ) ;define


    ;; make-range-generator
    (define make-range-generator
      (case-lambda ((start end) (make-range-generator start end 1))
                   ((start) (make-infinite-range-generator start))
                   ((start end step)
                    (set! start (- (+ start step) step))
                    (lambda () (if (< start end)
                                 (let ((v start))
                                  (set! start (+ start step))
                                  v
                                 ) ;let
                                 (eof-object))
                    ) ;lambda
                   ) ;
      ) ;case-lambda
    ) ;define

    (define (make-infinite-range-generator start)
      (lambda ()
        (let ((result start))
         (set! start (+ start 1))
         result
        ) ;let
      ) ;lambda
    ) ;define



    ;; make-coroutine-generator
    (define (make-coroutine-generator proc)
      (define return #f)
      (define resume #f)
      (define yield (lambda (v) (call/cc (lambda (r) (set! resume r) (return v)))))
      (lambda () (call/cc (lambda (cc) (set! return cc)
                            (if resume
                              (resume (if #f #f))  ; void? or yield again?
                              (begin (proc yield)
                                     (set! resume (lambda (v) (return (eof-object))))
                                     (return (eof-object))))
                              ) ;begin
                            ) ;if
      ) ;lambda
    ) ;define


    ;; list->generator
    (define (list->generator lst)
      (lambda () (if (null? lst)
                   (eof-object)
                   (let ((next (car lst)))
                     (set! lst (cdr lst))
                     next)
                   ) ;let
      ) ;lambda
    ) ;define


    ;; vector->generator
    (define vector->generator
      (case-lambda ((vec) (vector->generator vec 0 (vector-length vec)))
                   ((vec start) (vector->generator vec start (vector-length vec)))
                   ((vec start end)
                    (lambda () (if (>= start end)
                                 (eof-object)
                                 (let ((next (vector-ref vec start)))
                                   (set! start (+ start 1))
                                   next)
                                 ) ;let
                    ) ;lambda
                   ) ;
      ) ;case-lambda
    ) ;define


    ;; reverse-vector->generator
    (define reverse-vector->generator
      (case-lambda ((vec) (reverse-vector->generator vec 0 (vector-length vec)))
                   ((vec start) (reverse-vector->generator vec start (vector-length vec)))
                   ((vec start end)
                    (lambda () (if (>= start end)
                                 (eof-object)
                                 (let ((next (vector-ref vec (- end 1))))
                                   (set! end (- end 1))
                                   next)
                                 ) ;let
                    ) ;lambda
                   ) ;
      ) ;case-lambda
    ) ;define


    ;; string->generator
    (define string->generator
      (case-lambda ((str) (string->generator str 0 (string-length str)))
                   ((str start) (string->generator str start (string-length str)))
                   ((str start end)
                    (lambda () (if (>= start end)
                                 (eof-object)
                                 (let ((next (string-ref str start)))
                                   (set! start (+ start 1))
                                   next)
                                 ) ;let
                    ) ;lambda
                   ) ;
      ) ;case-lambda
    ) ;define


    ;; bytevector->generator
    (define bytevector->generator
      (case-lambda ((str) (bytevector->generator str 0 (bytevector-length str)))
                   ((str start) (bytevector->generator str start (bytevector-length str)))
                   ((str start end)
                    (lambda () (if (>= start end)
                                 (eof-object)
                                 (let ((next (bytevector-u8-ref str start)))
                                   (set! start (+ start 1))
                                   next)
                                 ) ;let
                    ) ;lambda
                   ) ;
      ) ;case-lambda
    ) ;define


    ;; make-for-each-generator
    ;FIXME: seems to fail test
    (define (make-for-each-generator for-each obj)
      (make-coroutine-generator (lambda (yield) (for-each yield obj)))
    ) ;define


    ;; make-unfold-generator
    (define (make-unfold-generator stop? mapper successor seed)
      (make-coroutine-generator (lambda (yield)
                                  (let loop ((s seed))
                                   (if (stop? s)
                                     (if #f #f)
                                     (begin (yield (mapper s))
                                            (loop (successor s)))
                                     ) ;begin
                                   ) ;if
                                  ) ;let
      ) ;make-coroutine-generator
    ) ;define


    ;; gcons*
    (define (gcons* . args)
      (lambda () (if (null? args)
                   (eof-object)
                   (if (= (length args) 1)
                     ((car args))
                     (let ((v (car args)))
                      (set! args (cdr args))
                      v)
                     ) ;let
                   ) ;if
      ) ;lambda
    ) ;define


    ;; gappend
    (define (gappend . args)
      (lambda () (if (null? args)
                   (eof-object)
                   (let loop ((v ((car args))))
                    (if (eof-object? v)
                      (begin (set! args (cdr args))
                             (if (null? args)
                               (eof-object)
                               (loop ((car args)))
                             ) ;if
                      ) ;begin
                      v)
                    ) ;if
                   ) ;let
      ) ;lambda
    ) ;define

    ;; gflatten
    (define (gflatten gen)
      (let ((state '()))
        (lambda ()
          (if (null? state) (set! state (gen)))
          (if (eof-object? state)
            state
            (let ((obj (car state)))
              (set! state (cdr state))
              obj
            ) ;let
          ) ;if
        ) ;lambda
      ) ;let
    ) ;define

    ;; ggroup
    (define ggroup
      (case-lambda
        ((gen k)
         (simple-ggroup gen k)
        ) ;
        ((gen k padding)
         (padded-ggroup (simple-ggroup gen k) k padding)
        ) ;
      ) ;case-lambda
    ) ;define

    (define (simple-ggroup gen k)
      (lambda ()
        (let loop ((item (gen)) (result '()) (count (- k 1)))
          (if (eof-object? item)
            (if (null? result) item (reverse result))
            (if (= count 0)
              (reverse (cons item result))
              (loop (gen) (cons item result) (- count 1))
            ) ;if
          ) ;if
        ) ;let
      ) ;lambda
    ) ;define

    (define (padded-ggroup gen k padding)
      (lambda ()
        (let ((item (gen)))
          (if (eof-object? item)
            item
            (let ((len (length item)))
              (if (= len k)
                  item
                  (append item (make-list (- k len) padding))
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;lambda
    ) ;define

    ;; gmerge
    (define gmerge
      (case-lambda
        ((<) (error "wrong number of arguments for gmerge"))
        ((< gen) gen)
        ((< genleft genright)
         (let ((left (genleft))
               (right (genright)))
           (lambda ()
             (cond
              ((and (eof-object? left) (eof-object? right))
               left
              ) ;
              ((eof-object? left)
               (let ((obj right)) (set! right (genright)) obj)
              ) ;
              ((eof-object? right)
               (let ((obj left))  (set! left (genleft)) obj)
              ) ;
              ((< right left)
               (let ((obj right)) (set! right (genright)) obj)
              ) ;
              (else
               (let ((obj left)) (set! left (genleft)) obj)
              ) ;else
             ) ;cond
           ) ;lambda
         ) ;let
        ) ;
        ((< . gens)
         (apply gmerge <
                (let loop ((gens gens) (gs '()))
                  (cond ((null? gens) (reverse gs))
                        ((null? (cdr gens)) (reverse (cons (car gens) gs)))
                        (else (loop (cddr gens)
                                    (cons (gmerge < (car gens) (cadr gens)) gs))
                        ) ;else
                  ) ;cond
                ) ;let
         ) ;apply
        ) ;
      ) ;case-lambda
    ) ;define

    ;; gmap
    (define gmap
      (case-lambda
        ((proc) (error "wrong number of arguments for gmap"))
        ((proc gen)
         (lambda ()
           (let ((item (gen)))
             (if (eof-object? item) item (proc item))
           ) ;let
         ) ;lambda
        ) ;
        ((proc . gens)
         (lambda ()
           (let ((items (map (lambda (x) (x)) gens)))
             (if (any eof-object? items) (eof-object) (apply proc items))
           ) ;let
         ) ;lambda
        ) ;
      ) ;case-lambda
    ) ;define

    ;; gcombine
    (define (gcombine proc seed . gens)
      (lambda ()
        (define items (map (lambda (x) (x)) gens))
        (if (any eof-object? items)
          (eof-object)
          (let ()
           (define-values (value newseed) (apply proc (append items (list seed))))
           (set! seed newseed)
           value
          ) ;let
        ) ;if
      ) ;lambda
    ) ;define

    ;; gfilter
    (define (gfilter pred gen)
      (lambda () (let loop ()
                  (let ((next (gen)))
                    (if (or (eof-object? next)
                            (pred next))
                      next
                      (loop))
                    ) ;if
                  ) ;let
      ) ;lambda
    ) ;define

    ;; gstate-filter
    (define (gstate-filter proc seed gen)
      (let ((state seed))
        (lambda ()
          (let loop ((item (gen)))
            (if (eof-object? item)
              item
              (let-values (((yes newstate) (proc item state)))
                (set! state newstate)
                (if yes
                   item
                   (loop (gen))
                ) ;if
              ) ;let-values
            ) ;if
          ) ;let
        ) ;lambda
      ) ;let
    ) ;define



    ;; gremove
    (define (gremove pred gen)
      (gfilter (lambda (v) (not (pred v))) gen)
    ) ;define



    ;; gtake
    (define gtake
      (case-lambda ((gen k) (gtake gen k (eof-object)))
                   ((gen k padding)
                    (make-coroutine-generator (lambda (yield)
                                                (if (> k 0)
                                                  (let loop ((i 0) (v (gen)))
                                                   (begin (if (eof-object? v) (yield padding) (yield v))
                                                          (if (< (+ 1 i) k)
                                                            (loop (+ 1 i) (gen))
                                                            (eof-object)
                                                          ) ;if
                                                   ) ;begin
                                                  ) ;let
                                                  (eof-object))
                                                ) ;if
                    ) ;make-coroutine-generator
                   ) ;
      ) ;case-lambda
    ) ;define



    ;; gdrop
    (define (gdrop gen k)
      (lambda () (do () ((<= k 0)) (set! k (- k 1)) (gen))
        (gen)
      ) ;lambda
    ) ;define



    ;; gdrop-while
    (define (gdrop-while pred gen)
      (define found #f)
      (lambda ()
        (let loop ()
         (let ((val (gen)))
          (cond (found val)
                ((and (not (eof-object? val)) (pred val)) (loop))
                (else (set! found #t) val)
          ) ;cond
         ) ;let
        ) ;let
      ) ;lambda
    ) ;define


    ;; gtake-while
    (define (gtake-while pred gen)
      (lambda () (let ((next (gen)))
                   (if (eof-object? next)
                     next
                     (if (pred next)
                       next
                       (begin (set! gen (generator))
                              (gen))
                       ) ;begin
                     ) ;if
                   ) ;if
      ) ;lambda
    ) ;define



    ;; gdelete
    (define gdelete
      (case-lambda ((item gen) (gdelete item gen equal?))
                   ((item gen ==)
                    (lambda () (let loop ((v (gen)))
                                (cond
                                  ((eof-object? v) (eof-object))
                                  ((== item v) (loop (gen)))
                                  (else v))
                                ) ;cond
                    ) ;lambda
                   ) ;
      ) ;case-lambda
    ) ;define



    ;; gdelete-neighbor-dups
    (define gdelete-neighbor-dups
      (case-lambda ((gen)
                    (gdelete-neighbor-dups gen equal?))
                   ((gen ==)
                    (define firsttime #t)
                    (define prev #f)
                    (lambda () (if firsttime
                                 (begin (set! firsttime #f)
                                        (set! prev (gen))
                                        prev
                                 ) ;begin
                                 (let loop ((v (gen)))
                                  (cond
                                    ((eof-object? v)
                                     v
                                    ) ;
                                    ((== prev v)
                                     (loop (gen))
                                    ) ;
                                    (else
                                      (set! prev v)
                                      v)
                                    ) ;else
                                  ) ;cond
                                 ) ;let
                    ) ;lambda
                   ) ;
      ) ;case-lambda
    ) ;define


    ;; gindex
    (define (gindex value-gen index-gen)
      (let ((done? #f) (count 0))
       (lambda ()
         (if done?
           (eof-object)
           (let loop ((value (value-gen)) (index (index-gen)))
            (cond
              ((or (eof-object? value) (eof-object? index))
               (set! done? #t)
               (eof-object)
              ) ;
              ((= index count)
               (set! count (+ count 1))
               value
              ) ;
              (else
                (set! count (+ count 1))
                (loop (value-gen) index)
              ) ;else
            ) ;cond
           ) ;let
         ) ;if
       ) ;lambda
      ) ;let
    ) ;define


    ;; gselect
    (define (gselect value-gen truth-gen)
      (let ((done? #f))
       (lambda ()
         (if done?
           (eof-object)
           (let loop ((value (value-gen)) (truth (truth-gen)))
            (cond
              ((or (eof-object? value) (eof-object? truth))
               (set! done? #t)
               (eof-object)
              ) ;
              (truth value)
              (else (loop (value-gen) (truth-gen)))
            ) ;cond
           ) ;let
         ) ;if
       ) ;lambda
      ) ;let
    ) ;define

    ;; generator->list
    (define generator->list
      (case-lambda
        ((gen n)
         (generator->list (gtake gen n))
        ) ;
        ((gen)
         (reverse (generator->reverse-list gen))
        ) ;
      ) ;case-lambda
    ) ;define

    ;; generator->reverse-list
    (define generator->reverse-list
      (case-lambda
        ((gen n)
         (generator->reverse-list (gtake gen n))
        ) ;
        ((gen)
         (generator-fold cons '() gen)
        ) ;
      ) ;case-lambda
    ) ;define

    ;; generator->vector
    (define generator->vector
      (case-lambda ((gen) (list->vector (generator->list gen)))
                   ((gen n) (list->vector (generator->list gen n)))
      ) ;case-lambda
    ) ;define


    ;; generator->vector!
    (define (generator->vector! vector at gen)
      (let loop ((value (gen)) (count 0) (at at))
       (cond
         ((eof-object? value) count)
         ((>= at (vector-length vector)) count)
         (else (begin
                 (vector-set! vector at value)
                 (loop (gen) (+ count 1) (+ at 1)))
         ) ;else
       ) ;cond
      ) ;let
    ) ;define


    ;; generator->string
    (define generator->string
      (case-lambda ((gen) (list->string (generator->list gen)))
                   ((gen n) (list->string (generator->list gen n)))
      ) ;case-lambda
    ) ;define




    ;; generator-fold
    (define (generator-fold f seed . gs)
      (define (inner-fold seed)
        (let ((vs (map (lambda (g) (g)) gs)))
         (if (any eof-object? vs)
           seed
           (inner-fold (apply f (append vs (list seed))))
         ) ;if
        ) ;let
      ) ;define
      (inner-fold seed)
    ) ;define



    ;; generator-for-each
    (define (generator-for-each f . gs)
      (let loop ()
       (let ((vs (map (lambda (g) (g)) gs)))
        (if (any eof-object? vs)
          (if #f #f)
          (begin (apply f vs)
                 (loop)
          ) ;begin
        ) ;if
       ) ;let
      ) ;let
    ) ;define

    ;; generator-map->list
    (define (generator-map->list f . gs)
      (let loop ((result '()))
       (let ((vs (map (lambda (g) (g)) gs)))
        (if (any eof-object? vs)
          (reverse result)
          (loop (cons (apply f vs) result))
        ) ;if
       ) ;let
      ) ;let
    ) ;define


    ;; generator-find
    (define (generator-find pred g)
      (let loop ((v (g)))
        (cond ((eof-object? v) #f)
              ((pred v) v)
              (else (loop (g)))
        ) ;cond
      ) ;let
    ) ;define

    ;; generator-count
    (define (generator-count pred g)
      (generator-fold (lambda (v n) (if (pred v) (+ 1 n) n)) 0 g)
    ) ;define


    ;; generator-any
    (define (generator-any pred gen)
      (let loop ((item (gen)))
        (cond ((eof-object? item) #f)
              ((pred item))
              (else (loop (gen)))
        ) ;cond
      ) ;let
    ) ;define


    ;; generator-every
    (define (generator-every pred gen)
      (let loop ((item (gen)) (last #t))
        (if (eof-object? item)
          last
          (let ((r (pred item)))
            (if r
              (loop (gen) r)
              #f
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define


    ;; generator-unfold
    (define (generator-unfold g unfold . args)
      (apply unfold eof-object? (lambda (x) x) (lambda (x) (g)) (g) args)
    ) ;define


    ;; make-accumulator
    (define (make-accumulator kons knil finalize)
      (let ((state knil))
        (lambda (obj)
          (if (eof-object? obj)
            (finalize state)
            (set! state (kons obj state))
          ) ;if
        ) ;lambda
      ) ;let
    ) ;define


    ;; count-accumulator
    (define (count-accumulator) (make-accumulator)
                                (lambda (obj state) (+ 1 state)) 0 (lambda (x) x)
    ) ;define

    ;; list-accumulator
    (define (list-accumulator) (make-accumulator cons '() reverse))

    ;; reverse-list-accumulator
    (define (reverse-list-accumulator) (make-accumulator cons '() (lambda (x) x)))

    ;; vector-accumulator
    (define (vector-accumulator)
      (make-accumulator cons '() (lambda (x) (list->vector (reverse x))))
    ) ;define

    ;; reverse-vector-accumulator
    (define (reverse-vector-accumulator)
      (make-accumulator cons '() list->vector)
    ) ;define

    ;; vector-accumulator!
    (define (vector-accumulator! vec at)
      (lambda (obj)
        (if (eof-object? obj)
          vec
          (begin
            (vector-set! vec at obj)
            (set! at (+ at 1))
          ) ;begin
        ) ;if
      ) ;lambda
    ) ;define

    ;; bytevector-accumulator
    (define (bytevector-accumulator)
      (make-accumulator cons '() (lambda (x) (list->bytevector (reverse x))))
    ) ;define

    ;; bytevector-accumulator!
    (define (bytevector-accumulator! bytevec at)
      (lambda (obj)
        (if (eof-object? obj)
          bytevec
          (begin
            (bytevector-u8-set! bytevec at obj)
            (set! at (+ at 1))
          ) ;begin
        ) ;if
      ) ;lambda
    ) ;define

    ;; string-accumulator
    (define (string-accumulator)
      (make-accumulator cons '()
            (lambda (lst) (list->string (reverse lst)))
      ) ;make-accumulator
    ) ;define

    ;; sum-accumulator
    (define (sum-accumulator) (make-accumulator + 0 (lambda (x) x)))

    ;; product-accumulator
    (define (product-accumulator) (make-accumulator * 1 (lambda (x) x)))
  ) ;begin
) ;define-library
