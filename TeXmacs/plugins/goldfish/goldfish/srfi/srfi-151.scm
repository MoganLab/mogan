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

(define-library (srfi srfi-151)
  (import (liii base) (liii error))
  (export bitwise-not bitwise-and bitwise-ior bitwise-xor bitwise-eqv bitwise-nor bitwise-nand
          bit-count bitwise-orc1 bitwise-orc2 bitwise-andc1 bitwise-andc2 arithmetic-shift
          integer-length bitwise-if bit-set? copy-bit bit-swap any-bit-set? every-bit-set?
          first-set-bit bit-field bit-field-any? bit-field-every? bit-field-clear bit-field-set)
  (begin

    (define bitwise-not lognot)

    (define bitwise-and logand)

    (define bitwise-ior logior)

    (define bitwise-xor logxor)

    (define (bitwise-eqv a b)
      (bitwise-not (bitwise-xor a b)))

    (define (bitwise-nor a b)
      (lognot (bitwise-ior a b)))

    (define (bitwise-nand a b)
      (lognot (bitwise-and a b)))

    (define bit-count
      (typed-lambda ((i integer?))
        (define (bit-count-positive i)
          (let loop ((n i)
                     (cnt 0))
            (if (= n 0)
                cnt
                (loop (logand n (- n 1)) (+ cnt 1)))))

        (cond ((zero? i) 0)
              ((positive? i) (bit-count-positive i))
              (else (bit-count-positive (lognot i))))))

    (define (bitwise-orc1 i j)
      (bitwise-ior (bitwise-not i) j))

    (define (bitwise-orc2 i j)
      (bitwise-ior i (bitwise-not j)))

    (define (bitwise-andc1 i j)
      (bitwise-and (bitwise-not i) j))

    (define (bitwise-andc2 i j)
      (bitwise-and i (bitwise-not j)))

    (define arithmetic-shift ash)

    (define (integer-length n)
      (if (zero? n)
          0
          (let loop ((value (abs n))
                     (count 1))
            (if (<= value 1)
                count
                (loop (ash value -1) (+ count 1))))))

    (define (bitwise-if mask a b)
      (bitwise-ior (bitwise-and mask a) (bitwise-and (bitwise-not mask) b)))

    (define (bit-set? index n)
      (cond ((negative? index)
             (error 'out-of-range "bit-set?: Index cannot be negative" index))
            ((> index 63)
             (error 'out-of-range "bit-set?: Index cannot exceed 63" index))
            ((= index 63) (negative? n))
            (else
             (not
              (zero? (bitwise-and n (arithmetic-shift 1 index)))))))

    (define (copy-bit index n boolean)
      (cond ((negative? index)
             (error 'out-of-range "copy-bit: Index cannot be negative" index))
            ((> index 63)
             (error 'out-of-range "copy-bit: Index cannot exceed 63" index))
            ((= index 63)
             (if boolean
                 (bitwise-ior n -9223372036854775808)
                 (bitwise-and n 9223372036854775807)))
            (else
             (if boolean
                 (bitwise-ior n (arithmetic-shift 1 index))
                 (bitwise-and n (bitwise-not (arithmetic-shift 1 index)))))))

    (define (bit-swap index1 index2 n)
      (cond ((or (negative? index1)
                 (negative? index2))
             (error 'out-of-range "bit-swap: Index cannot be negative" index1 index2))
            ((or (> index1 63) (> index2 63))
             (error 'out-of-range "bit-swap: Index cannot exceed 63" index1 index2))
            (else
             (copy-bit index2 (copy-bit index1 n (bit-set? index2 n)) (bit-set? index1 n)))))

    (define (any-bit-set? test-bits n)
      (not (zero? (bitwise-and test-bits n))))

    (define (every-bit-set? test-bits n)
      (= (bitwise-and test-bits n) test-bits))

    (define (first-set-bit n)
      (if (zero? n)
          -1
          (let ((lsb (bitwise-and n (- n))))
            (- (integer-length lsb) 1))))

    (define (bit-field i start end)
      (let* ((bits (integer-length i)))
        (if (>= start bits)
            (error 'out-of-range
                   "bit-field: Start cannot be greater than or equal to the integer length" start)
            (let* ((end (min end bits))
                   (width (- end start)))
              (if (<= width 0)
                  0
                  (let ((mask (arithmetic-shift (- (expt 2 width) 1) start)))
                    (arithmetic-shift (bitwise-and i mask) (- start))))))))

    (define (bit-field-any? i start end)
      (not
       (zero? (bitwise-and (arithmetic-shift i (- start)) (- (arithmetic-shift 1 (- end start)) 1)))))

    (define (bit-field-every? i start end)
      (= (bitwise-and (arithmetic-shift i (- start)) (- (arithmetic-shift 1 (- end start)) 1))
         (- (arithmetic-shift 1 (- end start)) 1)))

    (define (bit-field-clear i start end)
      (bitwise-and i
                   (bitwise-not
                    (arithmetic-shift (- (arithmetic-shift 1 (- end start)) 1) start))))

    (define (bit-field-set i start end)
      (bitwise-ior i (arithmetic-shift (- (arithmetic-shift 1 (- end start)) 1) start)))))

