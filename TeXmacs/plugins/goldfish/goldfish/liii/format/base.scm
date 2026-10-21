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

(define-library (liii format base)
  (import (scheme base)
    (scheme char)
    (scheme complex)
    (scheme inexact)
    (scheme write)
    (srfi srfi-1)
    (liii comparator)
    (liii hash-table)
    (rename (srfi srfi-165)
      (computation-with! with!)
      (computation-forked forked)
      (make-computation-environment-variable make-state-variable)
    ) ;rename
  ) ;import
  (export
    ;; basic
    show
    displayed
    written
    written-shared
    written-simply
    escaped
    maybe-escaped
    numeric
    numeric/comma
    numeric/si
    numeric/fitted
    nl
    fl
    space-to
    tab-to
    nothing
    each
    each-in-list
    joined
    joined/prefix
    joined/suffix
    joined/last
    joined/dot
    joined/range
    padded
    padded/right
    padded/both
    trimmed
    trimmed/right
    trimmed/both
    trimmed/lazy
    fitted
    fitted/right
    fitted/both
    output-default
    ;; computations
    fn
    with
    with!
    forked
    call-with-output
    %format-with
    %format-with!
    computation-bind
    computation-ask
    computation-environment-ref
    extract-shared-objects
    call-with-shared-ref
    call-with-shared-ref/cdr
    ;; state variables
    make-state-variable
    port
    row
    col
    width
    output
    writer
    pad-char
    ellipsis
    string-width
    substring/width
    substring/preserve
    radix
    precision
    decimal-sep
    decimal-align
    sign-rule
    comma-sep
    comma-rule
    word-separator?
    ambiguous-is-wide?
    pretty-environment
  ) ;export
  (begin

    (define sequence computation-each)

    (define-syntax let-optionals*
      (syntax-rules ()
        ((let-optionals* opt-ls () . body) (begin . body))
        ((let-optionals* (op . args) vars . body)
         (let ((tmp (op . args)))
           (let-optionals* tmp vars . body)
         ) ;let
        ) ;
        ((let-optionals* tmp ((var default) . rest) . body)
         (let ((var (if (pair? tmp) (car tmp) default)) (tmp2 (if (pair? tmp) (cdr tmp) '())))
           (let-optionals* tmp2 rest . body)
         ) ;let
        ) ;
        ((let-optionals* tmp tail . body) (let ((tail tmp)) . body))
      ) ;syntax-rules
    ) ;define-syntax

    (define-syntax fn
      (syntax-rules ()
        ((fn . x) (computation-fn . x))
      ) ;syntax-rules
    ) ;define-syntax

    (define (substring-length str . o)
      (let ((start (if (pair? o) (car o) 0))
            (end
              (if (and (pair? o) (pair? (cdr o))) (cadr o) (string-length str))
            ) ;end
           ) ;
        (- end start)
      ) ;let
    ) ;define

    (define (call-with-output-string proc)
      (let ((out (open-output-string)))
        (proc out)
        (let ((res (get-output-string out)))
          (close-output-port out)
          res
        ) ;let
      ) ;let
    ) ;define

    (define (string-index-right-char str ch)
      (let loop
        ((i (- (string-length str) 1)))
        (cond ((< i 0) 0)
              ((char=? (string-ref str i) ch) (+ i 1))
              (else (loop (- i 1)))
        ) ;cond
      ) ;let
    ) ;define

    (define (string-count-char str ch)
      (let ((len (string-length str)))
        (let loop
          ((i 0) (cnt 0))
          (cond ((= i len) cnt)
                ((char=? (string-ref str i) ch) (loop (+ i 1) (+ cnt 1)))
                (else (loop (+ i 1) cnt))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (output-default str)
      (fn (port (r row) (c col) string-width)
        (let ((nl-index (string-index-right-char str #\newline)))
          (write-string str port)
          (if (> nl-index 0)
            (with! (row (+ r (string-count-char str #\newline)))
              (col (string-width str nl-index))
            ) ;with!
            (with! (col (+ c (string-width str))))
          ) ;if
        ) ;let
      ) ;fn
    ) ;define

    (define (interaction-env)
      (if (defined? 'interaction-environment) (interaction-environment) (rootlet))
    ) ;define

    (define-computation-type make-show-env
      show-run
      (port (current-output-port))
      (col 0)
      (row 0)
      (width 78)
      (radix 10)
      (pad-char #\space)
      (output output-default)
      (string-width substring-length)
      (substring/width substring)
      (substring/preserve #f)
      (word-separator? char-whitespace?)
      (ambiguous-is-wide? #f)
      (ellipsis "")
      (decimal-align #f)
      (decimal-sep #f)
      (comma-sep #f)
      (comma-rule #f)
      (sign-rule #f)
      (precision #f)
      (writer #f)
      (pretty-environment (interaction-env))
    ) ;define-computation-type

    (define (show out . args)
      (cond
       ((output-port? out)
        (if (any procedure? args)
          (show-run (sequence (with! (port out)) (each-in-list args)))
          (for-each (lambda (arg) (display arg out)) args)
        ) ;if
       ) ;
       ((eq? #t out) (apply show (current-output-port) args))
       ((eq? #f out)
        (call-with-output-string (lambda (port-out) (apply show port-out args)))
       ) ;
       (else (error "unknown output to show, expected a port or #t/#f" out))
      ) ;cond
    ) ;define

    (define (%format-with-update env bindings)
      (let loop
        ((b bindings) (e env))
        (if (null? b)
          e
          (loop (cdr b) (computation-environment-update e (caar b) (cdar b)))
        ) ;if
      ) ;let
    ) ;define

    (define (%format-with bindings comp)
      (computation-local (lambda (env) (%format-with-update env bindings)) comp)
    ) ;define

    (define (%format-with! bindings)
      (computation-bind (computation-ask)
        (lambda (env)
          (for-each (lambda (b) (computation-environment-update! env (car b) (cdr b)))
            bindings
          ) ;for-each
          (computation-pure (if #f #f))
        ) ;lambda
      ) ;computation-bind
    ) ;define

    (define-syntax with
      (syntax-rules ()
        ((with ((var val) ...) x ... y)
         (%format-with (list (cons var val) ...) (each x ... y))
        ) ;
        ((with () x ... y) (each x ... y))
      ) ;syntax-rules
    ) ;define-syntax

    (define-syntax with!
      (syntax-rules ()
        ((with! (var val) ...) (%format-with! (list (cons var val) ...)))
        ((with!) (computation-pure (if #f #f)))
      ) ;syntax-rules
    ) ;define-syntax

    (define nothing (fn () (with!)))

    (define (displayed x)
      (cond ((procedure? x) x)
            ((string? x) (fn ((output1 output)) (output1 x)))
            ((char? x) (displayed (string x)))
            (else (written x))
      ) ;cond
    ) ;define

    (define (written x)
      (fn (writer) ((or writer written-default) x))
    ) ;define

    (define (each-in-list args)
      (if (pair? args)
        (if (pair? (cdr args))
          (sequence (displayed (car args)) (each-in-list (cdr args)))
          (fn () (displayed (car args)))
        ) ;if
        nothing
      ) ;if
    ) ;define

    (define (each . args)
      (each-in-list args)
    ) ;define

    (define (call-with-output producer consumer)
      (let ((out (open-output-string)))
        (forked (with ((port out) (output output-default)) producer)
          (fn () (consumer (get-output-string out)))
        ) ;forked
      ) ;let
    ) ;define

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;; shared structure utilities

    (define (extract-shared-objects x cyclic-only?)
      (let ((seen (make-hash-table (make-eq-comparator))))
        (let find
          ((x x))
          (cond
           ((or (pair? x) (vector? x))
            (hash-table-set! seen x (+ 1 (hash-table-ref/default seen x 0)))
            (cond ((> (hash-table-ref/default seen x 0) 1))
                  ((pair? x) (find (car x)) (find (cdr x)))
                  ((vector? x)
                   (do ((i 0 (+ i 1)))
                     ((= i (vector-length x)))
                     (find (vector-ref x i))
                   ) ;do
                  ) ;
            ) ;cond
            (if (and cyclic-only? (<= (hash-table-ref/default seen x 0) 1))
              (hash-table-delete! seen x)
            ) ;if
           ) ;
          ) ;cond
        ) ;let
        (let ((res (make-hash-table (make-eq-comparator))) (count 0))
          (hash-table-for-each
            (lambda (k v)
              (cond ((> v 1) (hash-table-set! res k (cons count #f)) (set! count (+ count 1)))
              ) ;cond
            ) ;lambda
            seen
          ) ;hash-table-for-each
          (cons res 0)
        ) ;let
      ) ;let
    ) ;define

    (define (gen-shared-ref cell shares)
      (set-car! cell (cdr shares))
      (set-cdr! cell #t)
      (set-cdr! shares (+ (cdr shares) 1))
      (string-append (number->string (car cell)))
    ) ;define

    (define (call-with-shared-ref obj shares each proc)
      (let ((cell (hash-table-ref/default (car shares) obj #f)))
        (cond
         ((and (pair? cell) (cdr cell)) (each "#" (number->string (car cell)) "#"))
         ((pair? cell) (each "#" (gen-shared-ref cell shares) "=" proc))
         (else (each proc))
        ) ;cond
      ) ;let
    ) ;define

    (define (call-with-shared-ref/cdr obj shares each proc . o)
      (let ((sep (if (pair? o) (car o) ""))
            (cell (hash-table-ref/default (car shares) obj #f))
           ) ;
        (cond
         ((and (pair? cell) (cdr cell)) (each sep ". #" (number->string (car cell)) "#"))
         ((pair? cell) (each sep ". #" (gen-shared-ref cell shares) "=(" proc ")"))
         (else (each sep proc))
        ) ;cond
      ) ;let
    ) ;define

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;; string and escaping utilities

    (define (write-to-string x)
      (call-with-output-string (lambda (out) (write x out)))
    ) ;define

    (define (string-replace-all str ch1 ch2)
      (let ((out (open-output-string)))
        (string-for-each
          (lambda (ch) (display (if (eqv? ch ch1) ch2 ch) out))
          str
        ) ;string-for-each
        (get-output-string out)
      ) ;let
    ) ;define

    (define (string-index str ch)
      (let ((len (string-length str)))
        (let loop
          ((i 0))
          (cond ((= i len) len)
                ((char=? (string-ref str i) ch) i)
                (else (loop (+ i 1)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (string-contains str sub)
      (let ((len (string-length str)) (sub-len (string-length sub)))
        (if (> sub-len len)
          #f
          (let loop
            ((i 0))
            (cond ((> (+ i sub-len) len) #f)
                  ((string=? (substring str i (+ i sub-len)) sub) i)
                  (else (loop (+ i 1)))
            ) ;cond
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (reverse-list->string ls)
      (list->string (reverse ls))
    ) ;define

    (define (string-intersperse-right str sep rule)
      (let ((len (string-length str)))
        (let lp
          ((i len) (rule rule) (res '()))
          (let* ((offset (if (pair? rule) (car rule) rule))
                 (i2 (if offset (max 0 (- i offset)) 0))
                ) ;
            (if (<= i2 0)
              (apply string-append (cons (substring str 0 i) res))
              (lp i2
                (if
                  (and (pair? rule) (not (null? (cdr rule))))
                  (cdr rule)
                  rule
                ) ;if
                (cons sep (cons (substring str i2 i) res))
              ) ;lp
            ) ;if
          ) ;let*
        ) ;let
      ) ;let
    ) ;define

    (define (escaped fmt . o)
      (let-optionals* o
       ((quot #\") (esc #\\) (rename (lambda (x) #f)))
       (let ((esc-str (cond ((char? esc) (string esc)) ((not esc) (string quot)) (else esc)))
            ) ;
         (fn ((orig-output output))
           (define (output* str)
             (let ((len (string-length str)))
               (let lp
                 ((i 0) (j 0))
                 (define (collect)
                   (if (= i j) "" (substring str i j))
                 ) ;define
                 (if (>= j len)
                   (orig-output (collect))
                   (let ((c (string-ref str j)) (j2 (+ j 1)))
                     (cond
                      ((or (eqv? c quot) (eqv? c esc))
                       (each (orig-output (collect))
                         (orig-output esc-str)
                         (orig-output (string c))
                         (fn () (lp j2 j2))
                       ) ;each
                      ) ;
                      ((rename c)
                       =>
                       (lambda (c2)
                         (each (orig-output (collect))
                           (orig-output esc-str)
                           (orig-output (if (char? c2) (string c2) c2))
                           (fn () (lp j2 j2))
                         ) ;each
                       ) ;lambda
                      ) ;
                      (else (lp i j2))
                     ) ;cond
                   ) ;let
                 ) ;if
               ) ;let
             ) ;let
           ) ;define
           (with ((output output*)) (displayed fmt))
         ) ;fn
       ) ;let
      ) ;let-optionals*
    ) ;define
    (define (maybe-escaped fmt pred . o)
      (let-optionals* o
       ((quot #\") (esc #\\) (rename (lambda (x) #f)))
       (define (esc? c)
         (or (eqv? c quot) (eqv? c esc) (rename c) (pred c))
       ) ;define
       (call-with-output fmt
         (lambda (str)
           (let ((len (string-length str)))
             (let loop
               ((i 0) (found? #f))
               (if (= i len)
                 (if found? (each quot (escaped str quot esc rename) quot) (displayed str))
                 (if (esc? (string-ref str i)) (loop len #t) (loop (+ i 1) #f))
               ) ;if
             ) ;let
           ) ;let
         ) ;lambda
       ) ;call-with-output
      ) ;let-optionals*
    ) ;define
    (define (char-mirror c)
      (case c
       ((#\() #\))
       ((#\[) #\])
       ((#\{) #\})
       ((#\<) #\>)
       (else c)
      ) ;case
    ) ;define
    (define (integer-log a base)
      (if (zero? a)
        0
        (let lp
          ((n 1) (res 0))
          (if (> n a) res (lp (* n base) (+ res 1)))
        ) ;let
      ) ;if
    ) ;define
    (define (default var dflt)
      (if (eq? var 'unspec-srfi-166) dflt var)
    ) ;define
    (define (numeric n . o)
      (let* ((rad (if (pair? o) (car o) 'unspec-srfi-166))
             (o1 (if (pair? o) (cdr o) '()))
             (prec (if (pair? o1) (car o1) 'unspec-srfi-166))
             (o2 (if (pair? o1) (cdr o1) '()))
             (sgn (if (pair? o2) (car o2) 'unspec-srfi-166))
             (o3 (if (pair? o2) (cdr o2) '()))
             (comma (if (pair? o3) (car o3) 'unspec-srfi-166))
             (o4 (if (pair? o3) (cdr o4) '()))
             (commasep (if (pair? o4) (car o4) 'unspec-srfi-166))
             (o5 (if (pair? o4) (cdr o4) '()))
             (decsep (if (pair? o5) (car o5) 'unspec-srfi-166))
            ) ;
        (fn (radix precision sign-rule comma-rule comma-sep decimal-sep decimal-align)
          (let* ((radix (default rad radix))
                 (precision (default prec precision))
                 (sign-rule (default sgn sign-rule))
                 (comma-rule (default comma comma-rule))
                 (comma-sep (default commasep comma-sep))
                 (dec-sep
                   (default decsep (or decimal-sep (if (eqv? comma-sep #\.) #\, #\.)))
                 ) ;dec-sep
                 (dec-ls (if (char? dec-sep) (list dec-sep) (reverse (string->list dec-sep))))
                ) ;
            (define (get-scale q)
              (expt radix (- (integer-log q radix) 1))
            ) ;define
            (define (char-digit d)
              (cond ((char? d) d)
                    ((< d 10) (integer->char (+ d (char->integer #\0))))
                    (else (integer->char (+ (- d 10) (char->integer #\a))))
              ) ;cond
            ) ;define
            (define (digit-value ch)
              (let ((res (- (char->integer ch) (char->integer #\0))))
                (if (<= 0 res 9) res ch)
              ) ;let
            ) ;define
            (define (round-up ls)
              (let lp
                ((ls ls) (res '()))
                (cond ((null? ls) (append (reverse res) '(1)))
                      ((not (number? (car ls))) (lp (cdr ls) (cons (car ls) res)))
                      ((= (car ls) (- radix 1)) (lp (cdr ls) (cons 0 res)))
                      (else
                        (append (reverse res) (cons (+ 1 (car ls)) (cdr ls)))
                      ) ;else
                ) ;cond
              ) ;let
            ) ;define
            (define (maybe-round n d ls)
              (let* ((q (quotient n d))
                     (digit
                       (* 2 (if (>= q radix) (quotient q (get-scale q)) q))
                     ) ;digit
                    ) ;
                (if
                  (or (> digit radix)
                    (and (= digit radix) (let ((prev (find integer? ls))) (and prev (odd? prev))))
                  ) ;or
                  (round-up ls)
                  ls
                ) ;if
              ) ;let*
            ) ;define
            (define (maybe-trim-zeros i res inexact?)
              (if (and (not precision) (positive? i))
                (let lp
                  ((res res))
                  (cond
                   ((and (pair? res) (eqv? 0 (car res))) (lp (cdr res)))
                   ((and (pair? res) (eqv? (car dec-ls) (car res)) (null? (cdr dec-ls)))
                    (if inexact? (cons 0 res) (cdr res))
                   ) ;
                   (else res)
                  ) ;cond
                ) ;let
                res
              ) ;if
            ) ;define
            (define (gen-general n-orig)
              (let* ((p (exact n-orig)) (n (numerator p)) (d (denominator p)))
                (let lp
                  ((n n) (i (if (zero? p) -1 (- (integer-log p radix)))) (res '()))
                  (cond
                   ((if precision (< i precision) (< i 16))
                    (let ((res
                            (if (zero? i) (append dec-ls (if (null? res) (cons 0 res) res)) res)
                          ) ;res
                          (q (quotient n d))
                         ) ;
                      (cond
                       ((< i -1)
                        (let* ((scale (expt radix (- -1 i)))
                               (digit (quotient q scale))
                               (n2 (- n (* d digit scale)))
                              ) ;
                          (lp n2 (+ i 1) (cons digit res))
                        ) ;let*
                       ) ;
                       (else (lp (* (remainder n d) radix) (+ i 1) (cons q res)))
                      ) ;cond
                    ) ;let
                   ) ;
                   (else
                     (reverse-list->string (map char-digit (maybe-trim-zeros i (maybe-round n d res) (inexact? n-orig)))
                     ) ;reverse-list->string
                   ) ;else
                  ) ;cond
                ) ;let
              ) ;let*
            ) ;define
            (define (gen-fixed n)
              (cond
               ((and (eqv? radix 10) (zero? precision) (inexact? n))
                (number->string (exact (round n)))
               ) ;
               ((and (eqv? radix 10) (or (integer? n) (inexact? n)))
                (let* ((s (number->string n))
                       (len (string-length s))
                       (dec (string-index s #\.))
                       (e (string-index s #\e))
                       (digits (- len dec))
                      ) ;
                  (cond
                   ((< e len)
                    (if (positive? precision)
                      (let* ((e2 (+ e 1)) (exp (string->number (substring s e2 len))))
                        (if (<= (- exp) precision)
                          (gen-general n)
                          (let ((n2 (string->number (substring s 0 e))))
                            (string-append (gen-fixed n2) (substring s e len))
                          ) ;let
                        ) ;if
                      ) ;let*
                      (gen-general n)
                    ) ;if
                   ) ;
                   ((= dec len)
                    (string-append s
                      (if (char? dec-sep) (string dec-sep) dec-sep)
                      (make-string precision #\0)
                    ) ;string-append
                   ) ;
                   ((<= digits precision)
                    (string-append s (make-string (- precision digits -1) #\0))
                   ) ;
                   (else
                     (let* ((last (- len (- digits precision 1))) (res (substring s 0 last)))
                       (if
                         (and (< last len)
                           (let ((next (digit-value (string-ref s last))))
                             (or (> next 5)
                               (and (= next 5)
                                 (> last 0)
                                 (memv (digit-value (string-ref s (- last 1))) '(1
                                                                                 3
                                                                                 5
                                                                                 7
                                                                                 9))
                               ) ;and
                             ) ;or
                           ) ;let
                         ) ;and
                         (reverse-list->string
                           (map char-digit (round-up (reverse (map digit-value (string->list res)))))
                         ) ;reverse-list->string
                         res
                       ) ;if
                     ) ;let*
                   ) ;else
                  ) ;cond
                ) ;let*
               ) ;
               (else (gen-general n))
              ) ;cond
            ) ;define
            (define (gen-positive-real n)
              (cond (precision (gen-fixed n))
                    ((memv radix (if (exact? n) '(2 8 10 16) '(10))) (number->string n radix))
                    (else (gen-general n))
              ) ;cond
            ) ;define
            (define (insert-commas str)
              (let* ((len (string-length str))
                     (dec-pos (if (string? dec-sep)
                                (or (string-contains str dec-sep) len)
                                (string-index str dec-sep)
                              ) ;if
                     ) ;dec-pos
                     (left (substring str 0 dec-pos))
                     (right (substring str dec-pos len))
                     (sep (cond ((char? comma-sep) (string comma-sep))
                                ((string? comma-sep) comma-sep)
                                ((eqv? #\, dec-sep) ".")
                                (else ",")
                          ) ;cond
                     ) ;sep
                    ) ;
                (string-append (string-intersperse-right left sep comma-rule) right)
              ) ;let*
            ) ;define
            (define (wrap-comma n)
              (if (and (not precision) (exact? n) (not (integer? n)))
                (string-append (wrap-comma (numerator n)) "/" (wrap-comma (denominator n)))
                (let* ((s0 (gen-positive-real n))
                       (s1 (if (or (eqv? #\. dec-sep) (equal? "." dec-sep))
                             s0
                             (string-replace-all s0 #\. dec-sep)
                           ) ;if
                       ) ;s1
                      ) ;
                  (if comma-rule (insert-commas s1) s1)
                ) ;let*
              ) ;if
            ) ;define
            (define (negative?* n)
              (or (negative? n) (eqv? -0.0 n))
            ) ;define
            (define (wrap-sign n sign-rule)
              (cond
               ((negative?* n)
                (cond
                 ((char? sign-rule)
                  (string-append (string sign-rule)
                    (wrap-comma (- n))
                    (string (char-mirror sign-rule))
                  ) ;string-append
                 ) ;
                 ((pair? sign-rule)
                  (string-append (car sign-rule) (wrap-comma (- n)) (cdr sign-rule))
                 ) ;
                 (else (string-append "-" (wrap-comma (- n))))
                ) ;cond
               ) ;
               ((eq? #t sign-rule) (string-append "+" (wrap-comma n)))
               (else (wrap-comma n))
              ) ;cond
            ) ;define
            (define (format-num n sign-rule)
              (cond
               ((finite? n)
                (let* ((s (wrap-sign n sign-rule))
                       (len (string-length s))
                       (dec-pos
                         (if decimal-align
                           (if (char? dec-sep)
                             (string-index s dec-sep)
                             (or (string-contains s dec-sep) len)
                           ) ;if
                           0
                         ) ;if
                       ) ;dec-pos
                       (diff (- (or decimal-align 0) dec-pos 1))
                      ) ;
                  (if (positive? diff) (string-append (make-string diff #\space) s) s)
                ) ;let*
               ) ;
               (else (number->string n))
              ) ;cond
            ) ;define
            (define (write-complex n)
              (cond
               ((and radix (not (and (integer? radix) (<= 2 radix 36))))
                (error "invalid radix for numeric formatting" radix)
               ) ;
               ((zero? (imag-part n)) (displayed (format-num (real-part n) sign-rule)))
               (else (each (format-num (real-part n) sign-rule) (format-num (imag-part n) #t) "i")
               ) ;else
              ) ;cond
            ) ;define
            (write-complex n)
          ) ;let*
        ) ;fn
      ) ;let*
    ) ;define
    (define numeric/si
      (let* ((names10 '#("" "k" "M" "G" "T" "E" "P" "Z" "Y"))
             (names-10 '#("" "m" "µ" "n" "p" "f" "a" "z" "y"))
             (names2
               (list->vector
                 (cons ""
                   (cons "Ki"
                     (map (lambda (s) (string-append s "i")) (cddr (vector->list names10)))
                   ) ;cons
                 ) ;cons
               ) ;list->vector
             ) ;names2
             (names-2
               (list->vector
                 (cons "" (map (lambda (s) (string-append s "i")) (cdr (vector->list names-10))))
               ) ;list->vector
             ) ;names-2
            ) ;
        (define (round-to n k)
          (/ (round (* n k)) k)
        ) ;define
        (lambda (n . o)
          (let ((base (if (pair? o) (car o) 1000))
                (separator
                  (if (and (pair? o) (pair? (cdr o))) (cadr o) "")
                ) ;separator
               ) ;
            (if (zero? n)
              "0"
              (let* ((log-n (log (abs n)))
                     (names (if (negative? log-n)
                              (if (= base 1024) names-2 names-10)
                              (if (= base 1024) names2 names10)
                            ) ;if
                     ) ;names
                     (k
                       (min
                         (exact ((if (negative? log-n) ceiling floor) (/ (abs log-n) (log base))))
                         (- (vector-length names) 1)
                       ) ;min
                     ) ;k
                     (n2
                       (round-to
                         (/ (abs n) (expt base (if (negative? log-n) (- k) k)))
                         10
                       ) ;round-to
                     ) ;n2
                    ) ;
                (each (if (negative? n) "-" "")
                  (if (integer? n2) (number->string (exact n2)) (number->string (inexact n2)))
                  separator
                  (vector-ref names k)
                ) ;each
              ) ;let*
            ) ;if
          ) ;let
        ) ;lambda
      ) ;let*
    ) ;define
    (define (numeric/fitted width n . args)
      (call-with-output (apply numeric n args)
        (lambda (str)
          (if (> (string-length str) width)
            (fn (precision decimal-sep comma-sep)
              (let ((prec (if (and (pair? args) (pair? (cdr args))) (cadr args) precision)))
                (if (and prec (not (zero? prec)))
                  (let* ((dec-sep (or decimal-sep (if (eqv? #\. comma-sep) #\, #\.)))
                         (diff
                           (- width (+ prec (if (char? dec-sep) 1 (string-length dec-sep))))
                         ) ;diff
                        ) ;
                    (each (if (positive? diff) (make-string diff #\#) "")
                      dec-sep
                      (make-string prec #\#)
                    ) ;each
                  ) ;let*
                  (displayed (make-string width #\#))
                ) ;if
              ) ;let
            ) ;fn
            (displayed str)
          ) ;if
        ) ;lambda
      ) ;call-with-output
    ) ;define
    (define (numeric/comma n . o)
      (fn ((orig-comma-rule comma-rule))
        (with
         ((comma-rule (if (pair? o) (car o) (or orig-comma-rule 3))))
         (apply numeric n (if (pair? o) (cdr o) '()))
        ) ;with
      ) ;fn
    ) ;define
    (define (write-with-shares obj shares)
      (fn ((orig-radix radix) precision)
        (let ((write-number
                (cond
                 ((and (not precision)
                    (assv orig-radix '((16 . "#x")
                                       (10 . "")
                                       (8 . "#o")
                                       (2 . "#b")))
                  ) ;and
                  =>
                  (lambda (cell)
                    (lambda (n)
                      (cond
                       ((eqv? orig-radix 10) (displayed (number->string n (car cell))))
                       ((exact? n) (each (cdr cell) (number->string n (car cell))))
                       (else (with ((radix 10)) (numeric n)))
                      ) ;cond
                    ) ;lambda
                  ) ;lambda
                 ) ;
                 (else
                   (lambda (n) (with ((radix 10)) (numeric n)))
                 ) ;else
                ) ;cond
              ) ;write-number
             ) ;
          (let wr
            ((obj obj))
            (call-with-shared-ref obj
              shares
              each
              (fn ()
                (cond
                 ((pair? obj)
                  (each "("
                    (fn ()
                      (let lp
                        ((ls obj))
                        (let ((rest (cdr ls)))
                          (each (wr (car ls))
                            (cond ((null? rest) nothing)
                                  ((pair? rest)
                                   (each " " (call-with-shared-ref/cdr rest shares each (fn () (lp rest))))
                                  ) ;
                                  (else (each " . " (wr rest)))
                            ) ;cond
                          ) ;each
                        ) ;let
                      ) ;let
                    ) ;fn
                    ")"
                  ) ;each
                 ) ;
                 ((vector? obj)
                  (let ((len (vector-length obj)))
                    (if (zero? len)
                      (displayed "#()")
                      (each "#("
                        (wr (vector-ref obj 0))
                        (fn ()
                          (let lp
                            ((i 1))
                            (if (>= i len) nothing (each " " (wr (vector-ref obj i)) (fn () (lp (+ i 1)))))
                          ) ;let
                        ) ;fn
                        ")"
                      ) ;each
                    ) ;if
                  ) ;let
                 ) ;
                 ((number? obj) (write-number obj))
                 (else (displayed (write-to-string obj)))
                ) ;cond
              ) ;fn
            ) ;call-with-shared-ref
          ) ;let
        ) ;let
      ) ;fn
    ) ;define
    (define (written-default obj)
      (write-with-shares obj (extract-shared-objects obj #t))
    ) ;define
    (define (written-shared obj)
      (write-with-shares obj (extract-shared-objects obj #f))
    ) ;define
    (define (written-simply obj)
      (displayed (write-to-string obj))
    ) ;define
    (define nl (displayed "\n"))
    (define fl (fn (col) (if (zero? col) nothing nl)))
    (define (tab-to . o)
      (fn (col pad-char)
        (let* ((tab-width (if (pair? o) (car o) 8)) (rem (modulo col tab-width)))
          (if (positive? rem)
            (displayed (make-string (- tab-width rem) pad-char))
            nothing
          ) ;if
        ) ;let*
      ) ;fn
    ) ;define
    (define (space-to where)
      (fn (col pad-char) (displayed (make-string (max 0 (- where col)) pad-char)))
    ) ;define
    (define (padded/both width . ls)
      (call-with-output (each-in-list ls)
        (lambda (str)
          (fn (string-width pad-char)
            (let ((diff (- width (string-width str))))
              (if (positive? diff)
                (let* ((diff/2 (quotient diff 2))
                       (left (make-string diff/2 pad-char))
                       (right (if (even? diff) left (make-string (+ 1 diff/2) pad-char)))
                      ) ;
                  (each left str right)
                ) ;let*
                (displayed str)
              ) ;if
            ) ;let
          ) ;fn
        ) ;lambda
      ) ;call-with-output
    ) ;define
    (define (padded/right width . ls)
      (fn ((col1 col))
        (each (each-in-list ls)
          (fn ((col2 col) pad-char)
            (displayed
              (make-string (max 0 (- width (- col2 col1))) pad-char)
            ) ;displayed
          ) ;fn
        ) ;each
      ) ;fn
    ) ;define
    (define (padded/left width . ls)
      (call-with-output (each-in-list ls)
        (lambda (str)
          (fn (string-width pad-char)
            (let ((diff (- width (string-width str))))
              (each (make-string (max 0 diff) pad-char) str)
            ) ;let
          ) ;fn
        ) ;lambda
      ) ;call-with-output
    ) ;define
    (define padded padded/left)
    (define (trimmed/buffered width producer proc)
      (call-with-output producer
        (lambda (str)
          (fn (string-width)
            (let* ((str-width (string-width str)) (diff (- str-width width)))
              (displayed (if (positive? diff) (proc str str-width diff) str))
            ) ;let*
          ) ;fn
        ) ;lambda
      ) ;call-with-output
    ) ;define
    (define (trimmed/right width . ls)
      (trimmed/buffered width
        (each-in-list ls)
        (lambda (str str-width diff)
          (fn (ellipsis string-width substring/width substring/preserve)
            (let* ((ell (if (char? ellipsis) (string ellipsis) (or ellipsis "")))
                   (ell-len (string-width ell))
                   (diff (- (+ str-width ell-len) width))
                   (end (- width ell-len))
                  ) ;
              (each (if substring/preserve (substring/preserve (substring/width str -1 0)) nothing)
                (if (negative? diff) nothing (substring/width str 0 end))
                ell
                (if (and substring/preserve (< end str-width))
                  (substring/preserve (substring/width str end str-width))
                  nothing
                ) ;if
              ) ;each
            ) ;let*
          ) ;fn
        ) ;lambda
      ) ;trimmed/buffered
    ) ;define
    (define (trimmed/left width . ls)
      (trimmed/buffered width
        (each-in-list ls)
        (lambda (str str-width diff)
          (fn (ellipsis string-width substring/width substring/preserve)
            (let* ((ell (if (char? ellipsis) (string ellipsis) (or ellipsis "")))
                   (ell-len (string-width ell))
                   (diff (- (+ str-width ell-len) width))
                  ) ;
              (each (if (and substring/preserve (positive? diff))
                      (substring/preserve (substring/width str 0 diff))
                      nothing
                    ) ;if
                ell
                (if (negative? diff) nothing (substring/width str diff str-width))
              ) ;each
            ) ;let*
          ) ;fn
        ) ;lambda
      ) ;trimmed/buffered
    ) ;define
    (define trimmed trimmed/left)
    (define (trimmed/both width . ls)
      (trimmed/buffered width
        (each-in-list ls)
        (lambda (str str-width diff)
          (fn (ellipsis string-width substring/width substring/preserve)
            (let* ((ell (if (char? ellipsis) (string ellipsis) (or ellipsis "")))
                   (ell-len (string-width ell))
                   (diff (- (+ str-width ell-len ell-len) width))
                   (left (quotient diff 2))
                   (right (- str-width (quotient (+ diff 1) 2)))
                  ) ;
              (each (if substring/preserve
                      (substring/preserve (substring/width str 0 left))
                      nothing
                    ) ;if
                (if (negative? diff) ell (each ell (substring/width str left right) ell))
                (if substring/preserve
                  (substring/preserve (substring/width str right str-width))
                  nothing
                ) ;if
              ) ;each
            ) ;let*
          ) ;fn
        ) ;lambda
      ) ;trimmed/buffered
    ) ;define
    (define (trimmed/lazy width . ls)
      (fn ((orig-output output) string-width substring/width)
        (let ((chars-written 0) (orig-output (or orig-output output-default)))
          (define (output* str)
            (let ((len (string-width str)))
              (cond ((>= chars-written width) nothing)
                    ((> (+ chars-written len) width)
                     (let* ((end (max 0 (- width chars-written))) (s (substring/width str 0 end)))
                       (set! chars-written width)
                       (orig-output s)
                     ) ;let*
                    ) ;
                    (else (set! chars-written (+ chars-written len)) (orig-output str))
              ) ;cond
            ) ;let
          ) ;define
          (with ((output output*)) (each-in-list ls))
        ) ;let
      ) ;fn
    ) ;define
    (define (fitted/right width . ls)
      (padded/right width (trimmed/right width (each-in-list ls)))
    ) ;define
    (define (fitted/left width . ls)
      (padded/left width (trimmed/left width (each-in-list ls)))
    ) ;define
    (define fitted fitted/left)
    (define (fitted/both width . ls)
      (padded/both width (trimmed/both width (each-in-list ls)))
    ) ;define
    (define (joined/general elt-f last-f dot-f init-ls sep)
      (fn ()
        (let lp
          ((ls init-ls))
          (cond
           ((pair? ls)
            (each (if (eq? ls init-ls) nothing sep)
             ((if (and last-f (null? (cdr ls))) last-f elt-f) (car ls))
             (lp (cdr ls))
            ) ;each
           ) ;
           ((and dot-f (not (null? ls)))
            (each (if (eq? ls init-ls) nothing sep) (dot-f ls))
           ) ;
           (else nothing)
          ) ;cond
        ) ;let
      ) ;fn
    ) ;define
    (define (joined elt-f ls . o)
      (joined/general elt-f #f #f ls (if (pair? o) (car o) ""))
    ) ;define
    (define (joined/prefix elt-f ls . o)
      (if (null? ls)
        nothing
        (let ((sep (if (pair? o) (car o) "")))
          (each sep (joined elt-f ls sep))
        ) ;let
      ) ;if
    ) ;define
    (define (joined/suffix elt-f ls . o)
      (if (null? ls)
        nothing
        (let ((sep (if (pair? o) (car o) "")))
          (each (joined elt-f ls sep) sep)
        ) ;let
      ) ;if
    ) ;define
    (define (joined/last elt-f last-f ls . o)
      (joined/general elt-f last-f #f ls (if (pair? o) (car o) ""))
    ) ;define
    (define (joined/dot elt-f dot-f ls . o)
      (joined/general elt-f #f dot-f ls (if (pair? o) (car o) ""))
    ) ;define
    (define (joined/range elt-f start . o)
      (let ((end (and (pair? o) (car o)))
            (sep
              (if (and (pair? o) (pair? (cdr o))) (cadr o) "")
            ) ;sep
           ) ;
        (let lp
          ((i start))
          (if (and end (>= i end))
            nothing
            (each (if (> i start) sep nothing) (elt-f i) (fn () (lp (+ i 1))))
          ) ;if
        ) ;let
      ) ;let
    ) ;define
  ) ;begin
) ;define-library
