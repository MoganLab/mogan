;; SRFI-27 Implementation for Goldfish Scheme
;;
;; This is an implementation of SRFI-27 "Sources of Random Bits".
;; It is based on s7.c's built-in random functions.
;;
;; Copyright (C) Sebastian Egner (2002). All Rights Reserved.
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

(define-library (srfi srfi-27)
  (import (scheme base)
    (srfi srfi-19)
    (liii error)
  ) ;import
  (export random-integer
    random-real
    default-random-source
    make-random-source
    random-source?
    random-source-state-ref
    random-source-state-set!
    random-source-randomize!
    random-source-pseudo-randomize!
    random-source-make-integers
    random-source-make-reals
  ) ;export
  (begin

    ;; ====================
    ;; Random Source Record Type
    ;; ====================
    ;; A random-source is a record containing:
    ;; - state: the underlying s7 random-state object
    ;; - state-ref: thunk to get the state as external representation
    ;; - state-set!: procedure to set state from external representation
    ;; - randomize!: procedure to randomize state
    ;; - pseudo-randomize!: procedure to pseudo-randomize with indices
    ;; - make-integers: procedure returning a random-integer generator
    ;; - make-reals: procedure returning a random-real generator

    (define-record-type <random-source>
      (%make-random-source state
        state-ref
        state-set!
        randomize!
        pseudo-randomize!
        make-integers
        make-reals
      ) ;%make-random-source
      random-source?
      (state random-source-internal-state)
      (state-ref random-source-state-ref-proc)
      (state-set! random-source-state-set-proc
      ) ;state-set!
      (randomize! random-source-randomize-proc
      ) ;randomize!
      (pseudo-randomize! random-source-pseudo-randomize-proc
      ) ;pseudo-randomize!
      (make-integers random-source-make-integers-proc
      ) ;make-integers
      (make-reals random-source-make-reals-proc
      ) ;make-reals
    ) ;define-record-type

    ;; ====================
    ;; Internal Helpers
    ;; ====================

    ;; Get current time in nanoseconds as integer
    ;; Used for randomization
    (define (current-time-nanoseconds)
      (let ((t (current-time TIME-UTC)))
        (+ (* (time-second t) 1000000000)
          (time-nanosecond t)
        ) ;+
      ) ;let
    ) ;define

    ;; Create a new s7 random-state with given seed and carry
    (define (make-s7-random-state seed carry)
      (random-state seed carry)
    ) ;define

    ;; Get state as list (seed carry)
    (define (get-s7-state state)
      (random-state->list state)
    ) ;define

    ;; ====================
    ;; Random Source Operations
    ;; ====================

    (define (make-random-source)
      (let ((state (random-state 0)))
        (%make-random-source state
          ;; state-ref: return external representation
          (lambda ()
            (cons 'random-source-state
              (random-state->list state)
            ) ;cons
          ) ;lambda
          ;; state-set!: set state from external representation
          (lambda (new-state)
            (unless (and (pair? new-state)
                      (eq? (car new-state)
                        'random-source-state
                      ) ;eq?
                      (= (length new-state) 3)
                    ) ;and
              (error 'wrong-type-arg
                "invalid random source state"
                new-state
              ) ;error
            ) ;unless
            (let ((seed (cadr new-state))
                  (carry (caddr new-state))
                 ) ;
              (set! state (random-state seed carry))
            ) ;let
          ) ;lambda
          ;; randomize!: use current time to randomize
          (lambda ()
            (let ((ns (current-time-nanoseconds)))
              ;; Use nanoseconds to create a pseudo-random seed
              (let ((seed (modulo ns 4294967296))
                    (carry (modulo (quotient ns 4294967296)
                             4294967296
                           ) ;modulo
                    ) ;carry
                   ) ;
                (set! state (random-state seed carry))
              ) ;let
            ) ;let
          ) ;lambda
          ;; pseudo-randomize!: use i, j indices
          (lambda (i j)
            (unless (and (integer? i) (exact? i) (>= i 0))
              (error 'wrong-type-arg
                "pseudo-randomize! i must be a non-negative exact integer"
                i
              ) ;error
            ) ;unless
            (unless (and (integer? j) (exact? j) (>= j 0))
              (error 'wrong-type-arg
                "pseudo-randomize! j must be a non-negative exact integer"
                j
              ) ;error
            ) ;unless
            ;; Create a deterministic state based on i and j
            ;; Using a simple hash of i and j to create seed and carry
            (let ((seed (modulo (+ (* i 12345) j) 4294967296)
                  ) ;seed
                  (carry (modulo (+ (* j 54321) i) 4294967296)
                  ) ;carry
                 ) ;
              (set! state (random-state seed carry))
            ) ;let
          ) ;lambda
          ;; make-integers: return a procedure that generates random integers
          (lambda ()
            (lambda (n)
              (unless (and (integer? n)
                        (exact? n)
                        (positive? n)
                      ) ;and
                (error 'wrong-type-arg
                  "random-integer: n must be a positive exact integer"
                  n
                ) ;error
              ) ;unless
              ;; s7's random returns [0, n), we need [0, n-1] which is the same
              (random n state)
            ) ;lambda
          ) ;lambda
          ;; make-reals: return a procedure that generates random reals
          (lambda args
            (let ((unit #f))
              (if (pair? args)
                (begin
                  (set! unit (car args))
                  (unless (and (real? unit) (< 0 unit 1))
                    (error 'wrong-type-arg
                      "random-source-make-reals: unit must be a real in (0,1)"
                      unit
                    ) ;error
                  ) ;unless
                ) ;begin
              ) ;if
              (lambda ()
                (let ((r (random 1.0 state)))
                  ;; random returns [0.0, 1.0), but SRFI-27 requires (0, 1)
                  ;; s7's random for reals already returns (0, 1) when n > 0
                  ;; But we need to ensure we never return 0 or 1
                  (if (zero? r) 1e-16 r)
                ) ;let
              ) ;lambda
            ) ;let
          ) ;lambda
        ) ;%make-random-source
      ) ;let
    ) ;define

    ;; ====================
    ;; Standard Interface
    ;; ====================

    (define default-random-source
      (make-random-source)
    ) ;define

    (define (random-integer n)
     ((random-source-make-integers default-random-source
      ) ;random-source-make-integers
      n
     ) ;
    ) ;define

    (define (random-real)
     ((random-source-make-reals default-random-source
      ) ;random-source-make-reals
     ) ;
    ) ;define

    ;; ====================
    ;; Random Source State Operations
    ;; ====================

    (define (random-source-state-ref s)
      (unless (random-source? s)
        (error 'wrong-type-arg
          "random-source-state-ref: expected random-source"
          s
        ) ;error
      ) ;unless
      ((random-source-state-ref-proc s))
    ) ;define

    (define (random-source-state-set! s new-state)
      (unless (random-source? s)
        (error 'wrong-type-arg
          "random-source-state-set!: expected random-source"
          s
        ) ;error
      ) ;unless
      ((random-source-state-set-proc s)
       new-state
      ) ;
    ) ;define

    (define (random-source-randomize! s)
      (unless (random-source? s)
        (error 'wrong-type-arg
          "random-source-randomize!: expected random-source"
          s
        ) ;error
      ) ;unless
      ((random-source-randomize-proc s))
    ) ;define

    (define (random-source-pseudo-randomize! s i j)
      (unless (random-source? s)
        (error 'wrong-type-arg
          "random-source-pseudo-randomize!: expected random-source"
          s
        ) ;error
      ) ;unless
      ((random-source-pseudo-randomize-proc s)
       i
       j
      ) ;
    ) ;define

    ;; ====================
    ;; Random Source Generator Creation
    ;; ====================

    (define (random-source-make-integers s)
      (unless (random-source? s)
        (error 'wrong-type-arg
          "random-source-make-integers: expected random-source"
          s
        ) ;error
      ) ;unless
      ((random-source-make-integers-proc s))
    ) ;define

    (define (random-source-make-reals s . unit)
      (unless (random-source? s)
        (error 'wrong-type-arg
          "random-source-make-reals: expected random-source"
          s
        ) ;error
      ) ;unless
      (apply (random-source-make-reals-proc s)
        unit
      ) ;apply
    ) ;define

  ) ;begin
) ;define-library
