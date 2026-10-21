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

;;
;; Based on Alex Shinn's SRFI-166 implementation:
;; Copyright (c) 2006-2020 Alex Shinn.  All rights reserved.
;; BSD-style license: http://synthcode.com/license.txt
;;
;; Redistribution and use in source and binary forms, with or without
;; modification, are permitted provided that the following conditions
;; are met:
;; 1. Redistributions of source code must retain the above copyright
;;    notice, this list of conditions and the following disclaimer.
;; 2. Redistributions in binary form must reproduce the above copyright
;;    notice, this list of conditions and the following disclaimer in the
;;    documentation and/or other materials provided with the distribution.
;; 3. The name of the author may not be used to endorse or promote products
;;    derived from this software without specific prior written permission.
;;
;; THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
;; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
;; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
;; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
;; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
;; NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
;; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;

(define-library (liii format unicode)
  (import (scheme base) (scheme char) (liii format base))
  (export terminal-aware string-terminal-width string-terminal-width/wide
    substring-terminal-width substring-terminal-width/wide
    substring-terminal-preserve upcased downcased
  ) ;export
  (begin

    (define (zero-width? n)
      (or (<= 0 n 31)
        (<= 127 n 159)
        (<= 768 n 879)
        (<= 1155 n 1161)
        (<= 1425 n 1469)
        (= n 1471)
        (<= 1473 n 1474)
        (<= 1476 n 1477)
        (= n 1479)
        (<= 1552 n 1557)
        (<= 1611 n 1630)
        (= n 1648)
        (<= 1750 n 1764)
        (<= 1767 n 1768)
        (<= 1770 n 1773)
        (<= 8203 n 8207)
        (<= 8234 n 8238)
        (<= 8288 n 8303)
        (<= 65024 n 65039)
        (= n 65279)
        (<= 65529 n 65531)
        (<= 917504 n 921599)
      ) ;or
    ) ;define

    (define (full-width? n)
      (or (<= 4352 n 4447)
        (<= 9001 n 9002)
        (<= 11904 n 12350)
        (<= 12353 n 13311)
        (<= 13312 n 19903)
        (<= 19968 n 40959)
        (<= 43360 n 43388)
        (<= 44032 n 55203)
        (<= 55216 n 55238)
        (<= 63744 n 64255)
        (<= 65040 n 65049)
        (<= 65072 n 65135)
        (<= 65281 n 65376)
        (<= 65504 n 65510)
        (<= 127744 n 128591)
        (<= 129280 n 129535)
        (<= 131072 n 196605)
        (<= 196608 n 262141)
      ) ;or
    ) ;define

    (define (ambiguous-width? n)
      (or (= n 161)
        (= n 164)
        (<= 167 n 168)
        (= n 170)
        (<= 173 n 174)
        (<= 176 n 180)
        (<= 182 n 186)
        (<= 188 n 191)
        (= n 198)
        (= n 208)
        (<= 215 n 216)
        (<= 222 n 225)
        (= n 230)
        (<= 232 n 234)
        (<= 236 n 237)
        (= n 240)
        (<= 242 n 243)
        (<= 247 n 250)
        (= n 252)
        (= n 254)
        (<= 880 n 1023)
        ;; 希腊字母（包含 Π, Μ, Ε）
        (<= 1024 n 1279)
        (<= 8208 n 8231)
        (<= 8240 n 8286)
        (<= 8592 n 8703)
      ) ;or
    ) ;define

    (define (unicode-codepoint-width ci ambiguous-is-wide?)
      (cond ((zero-width? ci) 0)
            ((full-width? ci) 2)
            ((and ambiguous-is-wide? (ambiguous-width? ci)) 2)
            (else 1)
      ) ;cond
    ) ;define

    (define (utf8-step str i len)
      (let ((b (char->integer (string-ref str i))))
        (cond ((< b 128) (values b (+ i 1)))
              ((< b 224)
               (if (< (+ i 1) len)
                 (let ((b2 (char->integer (string-ref str (+ i 1)))))
                   (values (+ (* (- b 192) 64) (- b2 128)) (+ i 2))
                 ) ;let
                 (values b (+ i 1))
               ) ;if
              ) ;
              ((< b 240)
               (if (< (+ i 2) len)
                 (let ((b2 (char->integer (string-ref str (+ i 1))))
                       (b3 (char->integer (string-ref str (+ i 2))))
                      ) ;
                   (values (+ (* (- b 224) 4096) (* (- b2 128) 64) (- b3 128)) (+ i 3))
                 ) ;let
                 (values b (+ i 1))
               ) ;if
              ) ;
              (else
                (if (< (+ i 3) len)
                  (let ((b2 (char->integer (string-ref str (+ i 1))))
                        (b3 (char->integer (string-ref str (+ i 2))))
                        (b4 (char->integer (string-ref str (+ i 3))))
                       ) ;
                    (values (+ (* (- b 240) 262144) (* (- b2 128) 4096) (* (- b3 128) 64) (- b4 128))
                      (+ i 4)
                    ) ;values
                  ) ;let
                  (values b (+ i 1))
                ) ;if
              ) ;else
        ) ;cond
      ) ;let
    ) ;define

    (define (string-terminal-width/aux str start end ambiguous-is-wide?)
      (let lp1
        ((i start) (width 0))
        (if (>= i end)
          width
          (let ((c (string-ref str i)))
            (cond
             ((and (= 27 (char->integer c))
                (< (+ i 1) end)
                (eqv? #\[ (string-ref str (+ i 1)))
              ) ;and
              (let lp2
                ((j (+ i 2)))
                (cond ((>= j end) width)
                      ((memv (string-ref str j) '(#\m #\newline)) (lp1 (+ j 1) width))
                      (else (lp2 (+ j 1)))
                ) ;cond
              ) ;let
             ) ;
             ((char<=? c #\~) (lp1 (+ i 1) (+ width 1)))
             (else
               (call-with-values (lambda () (utf8-step str i end))
                 (lambda (cp next-i)
                   (lp1 next-i (+ width (unicode-codepoint-width cp ambiguous-is-wide?)))
                 ) ;lambda
               ) ;call-with-values
             ) ;else
            ) ;cond
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (string-terminal-width str . o)
      (let* ((len (string-length str))
             (start (if (pair? o) (car o) 0))
             (end
               (if (and (pair? o) (pair? (cdr o))) (cadr o) len)
             ) ;end
            ) ;
        (string-terminal-width/aux str start end #f)
      ) ;let*
    ) ;define

    (define (string-terminal-width/wide str . o)
      (let* ((len (string-length str))
             (start (if (pair? o) (car o) 0))
             (end
               (if (and (pair? o) (pair? (cdr o))) (cadr o) len)
             ) ;end
            ) ;
        (string-terminal-width/aux str start end #t)
      ) ;let*
    ) ;define

    (define (substring-terminal-width/aux str lo hi ambiguous-is-wide?)
      (let ((len (string-length str)))
        (let lp1
          ((i 0) (from (and (negative? lo) 0)) (width 0))
          (if (>= i len)
            (if from (substring str from len) str)
            (let ((c (string-ref str i)))
              (cond
               ((and (= 27 (char->integer c))
                  (< (+ i 1) len)
                  (eqv? #\[ (string-ref str (+ i 1)))
                ) ;and
                (let lp2
                  ((j (+ i 2)))
                  (cond ((>= j len) (lp1 j from width))
                        ((memv (string-ref str j) '(#\m #\newline)) (lp1 (+ j 1) from width))
                        (else (lp2 (+ j 1)))
                  ) ;cond
                ) ;let
               ) ;
               (else
                 (call-with-values (lambda () (utf8-step str i len))
                   (lambda (cp next-i)
                     (let ((width2 (+ width (unicode-codepoint-width cp ambiguous-is-wide?))))
                       (cond ((> width2 hi) (if from (substring str from i) ""))
                             ((and (not from) (> width2 lo)) (lp1 next-i i width2))
                             (else (lp1 next-i from width2))
                       ) ;cond
                     ) ;let
                   ) ;lambda
                 ) ;call-with-values
               ) ;else
              ) ;cond
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (substring-terminal-width str lo hi)
      (substring-terminal-width/aux str lo hi #f)
    ) ;define

    (define (substring-terminal-width/wide str lo hi)
      (substring-terminal-width/aux str lo hi #t)
    ) ;define

    (define non-local-controls
      '(#\x061C #\x200E #\x200F #\x202A #\x202B #\x202C #\x202D #\x202E #\x2066
        #\x2067 #\x2068 #\x2069)
    ) ;define

    (define (substring-terminal-preserve str)
      (let ((len (string-length str)))
        (let lp1
          ((i 0) (escapes '()))
          (if (>= i len)
            (apply string-append (reverse escapes))
            (let ((c (string-ref str i)) (i2 (+ i 1)))
              (cond
               ((and (= 27 (char->integer c)) (< i2 len) (eqv? #\[ (string-ref str i2)))
                (let lp2
                  ((j i2))
                  (if (>= j len)
                    (apply string-append (reverse escapes))
                    (let ((c2 (string-ref str j)) (j2 (+ j 1)))
                      (if (eqv? #\m c2) (lp1 j2 (cons (substring str i j2) escapes)) (lp2 j2))
                    ) ;let
                  ) ;if
                ) ;let
               ) ;
               ((memv c non-local-controls) (lp1 i2 (cons (string c) escapes)))
               (else (lp1 i2 escapes))
              ) ;cond
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (terminal-aware . args)
      (fn (ambiguous-is-wide?)
        (with ((string-width (if ambiguous-is-wide? string-terminal-width/wide string-terminal-width)
               ) ;string-width
               (substring/width (if ambiguous-is-wide? substring-terminal-width/wide substring-terminal-width)
               ) ;substring/width
               (substring/preserve substring-terminal-preserve)
              ) ;
          (each-in-list args)
        ) ;with
      ) ;fn
    ) ;define

    (define (with-string-transformer proc . ls)
      (fn ((orig-output output))
        (let ((output* (lambda (str) (orig-output (proc str)))))
          (with ((output output*)) (each-in-list ls))
        ) ;let
      ) ;fn
    ) ;define

    (define (upcased . ls)
      (apply with-string-transformer string-upcase ls)
    ) ;define

    (define (downcased . ls)
      (apply with-string-transformer string-downcase ls)
    ) ;define

  ) ;begin
) ;define-library
