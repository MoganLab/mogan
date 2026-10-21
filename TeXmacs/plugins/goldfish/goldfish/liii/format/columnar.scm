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

(define-library (liii format columnar)
  (import (scheme base)
    (scheme char)
    (scheme file)
    (srfi srfi-1)
    (liii format base)
  ) ;import
  (export columnar tabular wrapped wrapped/list wrapped/char justified from-file
    line-numbers
  ) ;export
  (begin

    (define (string-split-words str separator?)
      (let ((len (string-length str)))
        (let lp
          ((i 0) (res '()))
          (cond ((>= i len) (reverse res))
                (else
                  (let ((j
                          (let loop
                            ((k i))
                            (cond ((= k len) len)
                                  ((separator? (string-ref str k)) k)
                                  (else (loop (+ k 1)))
                            ) ;cond
                          ) ;let
                        ) ;j
                       ) ;
                    (lp (+ j 1) (if (= i j) res (cons (substring str i j) res)))
                  ) ;let
                ) ;else
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (string-split-lines str)
      (let ((len (string-length str)))
        (let lp
          ((i 0) (res '()))
          (if (>= i len)
            (reverse res)
            (let loop
              ((j i))
              (cond
               ((= j len) (reverse (cons (substring str i len) res)))
               ((char=? (string-ref str j) #\newline)
                (lp (+ j 1) (cons (substring str i j) res))
               ) ;
               (else (loop (+ j 1)))
              ) ;cond
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (make-finite-line-gen str)
      (let ((lines (string-split-lines str)))
        (lambda ()
          (if (null? lines)
            (read-char (open-input-string ""))
            (let ((line (car lines)))
              (set! lines (cdr lines))
              line
            ) ;let
          ) ;if
        ) ;lambda
      ) ;let
    ) ;define

    (define (make-infinite-const-gen str)
      (lambda () str)
    ) ;define

    (define (make-infinite-number-gen start)
      (let ((n start))
        (lambda () (let ((res (number->string n))) (set! n (+ n 1)) res))
      ) ;let
    ) ;define

    (define (call-with-column-generators columns consumers)
      (let lp
        ((cols columns) (gens '()))
        (if (null? cols)
          (consumers (reverse gens))
          (let* ((col (car cols)) (p (column-generate col)) (inf? (column-infinite? col)))
            (cond
             ((string? p) (lp (cdr cols) (cons (make-infinite-const-gen p) gens)))
             (inf?
               (lp (cdr cols) (cons (lambda () (show #f p)) gens))
             ) ;inf?
             (else
               (call-with-output p
                 (lambda (str) (lp (cdr cols) (cons (make-finite-line-gen str) gens)))
               ) ;call-with-output
             ) ;else
            ) ;cond
          ) ;let*
        ) ;if
      ) ;let
    ) ;define

    (define-record-type <Column>
      (make-column format generate infinite?)
      column?
      (format column-format)
      (generate column-generate)
      (infinite? column-infinite?)
    ) ;define-record-type

    (define (show-columns . ls)
      (fn ()
        (let* ((cols
                 (map
                   (lambda (x)
                     (make-column (or (car x) displayed)
                       (cadr x)
                       (and (pair? (cddr x)) (car (cddr x)))
                     ) ;make-column
                   ) ;lambda
                   ls
                 ) ;map
               ) ;cols
               (num-infinite (count column-infinite? cols))
              ) ;
          (call-with-column-generators cols
            (lambda (gens)
              (let lp
                ()
                (let* ((lines (map (lambda (gen) (gen)) gens))
                       (num-present
                         (count
                           (lambda (x) (and (string? x) (not (eof-object? x))))
                           lines
                         ) ;count
                       ) ;num-present
                      ) ;
                  (if (<= num-present num-infinite)
                    nothing
                    (each
                      (each-in-list
                        (map
                          (lambda (col line) ((column-format col) (if (eof-object? line) "" line)))
                          cols
                          lines
                        ) ;map
                      ) ;each-in-list
                      "\n"
                      (fn () (lp))
                    ) ;each
                  ) ;if
                ) ;let*
              ) ;let
            ) ;lambda
          ) ;call-with-column-generators
        ) ;let*
      ) ;fn
    ) ;define

    (define (proportional-width? w)
      (and (number? w) (or (< 0 w 1) (and (inexact? w) (= w 1.0))))
    ) ;define

    (define (build-column ls)
      (let ((fixed-width (if (pair? ls) (car ls) #f)) (ls1 (if (pair? ls) (cdr ls) '())))
        (let ((col-width (if (pair? ls1) (car ls1) #f)) (ls2 (if (pair? ls1) (cdr ls1) '())))
          (let ((last? (if (pair? ls2) (car ls2) #t)) (ls3 (if (pair? ls2) (cdr ls2) '())))
            (let ((tail (if (pair? ls3) (car ls3) '())) (ls4 (if (pair? ls3) (cdr ls3) '())))
              (let ((gen (if (pair? ls4) (car ls4) #f)) (ls5 (if (pair? ls4) (cdr ls4) '())))
                (let ((prefix (if (pair? ls5) (car ls5) '())) (ls6 (if (pair? ls5) (cdr ls5) '())))
                  (let ((align (if (pair? ls6) (car ls6) 'left)) (ls7 (if (pair? ls6) (cdr ls6) '())))
                    (let ((infinite? (if (pair? ls7) (car ls7) #f)))
                      (define (scale-width width)
                        (max 1 (exact (truncate (* col-width (- width fixed-width)))))
                      ) ;define
                      (define (padder)
                        (if (proportional-width? col-width)
                          (case align
                           ((right) (lambda (str) (fn (width) (padded (scale-width width) str))))
                           ((center) (lambda (str) (fn (width) (padded/both (scale-width width) str))))
                           (else
                             (lambda (str) (fn (width) (padded/right (scale-width width) str)))
                           ) ;else
                          ) ;case
                          (case align
                           ((right) (lambda (str) (padded col-width str)))
                           ((center) (lambda (str) (padded/both col-width str)))
                           (else (lambda (str) (padded/right col-width str)))
                          ) ;case
                        ) ;if
                      ) ;define
                      (define (affix x)
                        (cond
                         ((pair? tail)
                          (lambda (str) (each (each-in-list prefix) (x str) (each-in-list tail)))
                         ) ;
                         ((pair? prefix) (lambda (str) (each (each-in-list prefix) (x str))))
                         (else (displayed x))
                        ) ;cond
                      ) ;define
                      (list
                        (affix
                          (let ((pad (padder)))
                            (if (and last? (not (pair? tail)) (eq? align 'left))
                              (lambda (str)
                                (fn (pad-char)
                                 ((if (or (not pad-char) (char-whitespace? pad-char)) displayed pad) str)
                                ) ;fn
                              ) ;lambda
                              pad
                            ) ;if
                          ) ;let
                        ) ;affix
                        (if (proportional-width? col-width)
                          (fn ((orig-width width)) (with ((width (scale-width orig-width))) gen))
                          (with ((width col-width)) gen)
                        ) ;if
                        infinite?
                      ) ;list
                    ) ;let
                  ) ;let
                ) ;let
              ) ;let
            ) ;let
          ) ;let
        ) ;let
      ) ;let
    ) ;define

    (define (adjust-widths ls border-width)
      (let* ((fixed-ls
               (filter
                 (lambda (x) (and (number? (car x)) (>= (car x) 1)))
                 ls
               ) ;filter
             ) ;fixed-ls
             (fixed-total (fold + border-width (map car fixed-ls)))
             (scaled-ls
               (filter (lambda (x) (proportional-width? (car x))) ls)
             ) ;scaled-ls
             (denom (- (length ls) (+ (length fixed-ls) (length scaled-ls))))
             (rest
               (if (zero? denom) 0 (inexact (/ (- 1 (fold + 0 (map car scaled-ls))) denom)))
             ) ;rest
            ) ;
        (if (negative? rest)
          (error "fractional widths must sum to less than 1" (map car scaled-ls))
        ) ;if
        (map
          (lambda (col)
            (cons fixed-total (if (not (number? (car col))) (cons rest (cdr col)) col))
          ) ;lambda
          ls
        ) ;map
      ) ;let*
    ) ;define

    (define (finish ls border-width)
      (apply show-columns
        (map build-column (adjust-widths (reverse ls) border-width))
      ) ;apply
    ) ;define

    (define (string-contains-newline? str)
      (let ((len (string-length str)))
        (let loop
          ((i 0))
          (cond ((= i len) #f)
                ((char=? (string-ref str i) #\newline) #t)
                (else (loop (+ i 1)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (columnar . ls)
      (let lp
        ((ls ls)
         (strs '())
         (align 'left)
         (infinite? #f)
         (width #t)
         (border-width 0)
         (res '())
        ) ;
        (cond
         ((null? ls)
          (cond ((null? res) nl)
                ((pair? strs)
                 (finish
                   (cons
                     (cons (caar res)
                       (cons #t (cons (append (reverse strs) (cadr (cdar res))) (cddr (cdar res))))
                     ) ;cons
                     (cdr res)
                   ) ;cons
                   border-width
                 ) ;finish
                ) ;
                (else
                  (finish
                    (cons
                      (cons (caar res) (cons #t (cddr (car res))))
                      (cdr res)
                    ) ;cons
                    border-width
                  ) ;finish
                ) ;else
          ) ;cond
         ) ;
         ((char? (car ls))
          (lp (cons (string (car ls)) (cdr ls))
            strs
            align
            infinite?
            width
            border-width
            res
          ) ;lp
         ) ;
         ((string? (car ls))
          (if (string-contains-newline? (car ls))
            (error "column string literals can't contain newlines")
            (lp (cdr ls)
              (cons (car ls) strs)
              align
              infinite?
              width
              (+ border-width (string-length (car ls)))
              res
            ) ;lp
          ) ;if
         ) ;
         ((number? (car ls))
          (lp (cdr ls) strs align infinite? (car ls) border-width res)
         ) ;
         ((eq? (car ls) 'infinite) (lp (cdr ls) strs align #t width border-width res))
         ((symbol? (car ls))
          (lp (cdr ls) strs (car ls) infinite? width border-width res)
         ) ;
         ((procedure? (car ls))
          (lp (cdr ls)
            '()
            'left
            #f
            #t
            border-width
            (cons (list width #f '() (car ls) (reverse strs) align infinite?) res)
          ) ;lp
         ) ;
         (else (error "invalid column" (car ls)))
        ) ;cond
      ) ;let
    ) ;define

    (define (max-line-width str-width-fn str)
      (let ((lines (string-split-lines str)))
        (if (null? lines) 0 (fold max 0 (map str-width-fn lines)))
      ) ;let
    ) ;define

    (define (pad-finite proc width string-width k)
      (call-with-output proc
        (lambda (str)
          (let ((w (max-line-width (or string-width string-length) str)))
            (k (displayed str) (if (and (integer? width) (exact? width)) (max width w) w))
          ) ;let
        ) ;lambda
      ) ;call-with-output
    ) ;define

    (define (tabular . ls)
      (fn (string-width)
        (let lp
          ((ls ls) (infinite? #f) (width #t) (res '()))
          (cond ((null? ls) (apply columnar (reverse res)))
                ((number? (car ls)) (lp (cdr ls) infinite? (car ls) res))
                ((eq? 'infinite (car ls)) (lp (cdr ls) #t width (cons (car ls) res)))
                ((procedure? (car ls))
                 (if infinite?
                   (if width
                     (lp (cdr ls) #f #t (cons (car ls) (cons width res)))
                     (lp (cdr ls) #f #t (cons (car ls) res))
                   ) ;if
                   (pad-finite (car ls)
                     width
                     string-width
                     (lambda (gen width) (lp (cdr ls) #f #t (cons gen (cons width res))))
                   ) ;pad-finite
                 ) ;if
                ) ;
                (else (lp (cdr ls) infinite? width (cons (car ls) res)))
          ) ;cond
        ) ;let
      ) ;fn
    ) ;define

    (define (wrapped/char . ls)
      (fn ((orig-output output) width string-width substring/width)
        (define (kons-in-line str)
          (fn (col)
            (let ((len ((or string-width string-length) str)) (space (- width col)))
              (cond ((equal? "" str) nothing)
                    ((or (<= len space) (not (positive? space)))
                     (each (orig-output str) (orig-output "\n"))
                    ) ;
                    (else
                      (each (orig-output (substring/width str 0 space))
                        (orig-output "\n")
                        (fn () (kons-in-line (substring/width str space len)))
                      ) ;each
                    ) ;else
              ) ;cond
            ) ;let
          ) ;fn
        ) ;define
        (with
         ((output
            (lambda (str)
              (let ((len (string-length str)))
                (let lp
                  ((i 0))
                  (let loop
                    ((j i))
                    (cond ((>= i len) nothing)
                          ((= j len) (kons-in-line (substring str i len)))
                          ((char=? (string-ref str j) #\newline)
                           (each (fn () (kons-in-line (substring str i j))) (fn () (lp (+ j 1))))
                          ) ;
                          (else (loop (+ j 1)))
                    ) ;cond
                  ) ;let
                ) ;let
              ) ;let
            ) ;lambda
          ) ;output
         ) ;
         (each-in-list ls)
        ) ;with
      ) ;fn
    ) ;define

    (define (wrap-fold-words seq knil max-width get-width line . o)
      (let* ((last-line (if (pair? o) (car o) line))
             (vec (if (list? seq) (list->vector seq) seq))
             (len (vector-length vec))
             (len-1 (- len 1))
             (breaks (make-vector len #f))
             (penalties (make-vector len #f))
             (widths
               (list->vector (map get-width (if (list? seq) seq (vector->list vec))))
             ) ;widths
            ) ;
        (define (largest-fit i)
          (let lp
            ((j (+ i 1)) (width (vector-ref widths i)))
            (let ((width (+ width 1 (vector-ref widths j))))
              (cond ((>= width max-width) (- j 1))
                    ((>= j len-1) len-1)
                    (else (lp (+ j 1) width))
              ) ;cond
            ) ;let
          ) ;let
        ) ;define
        (define (min-penalty! i)
          (cond ((>= i len-1) 0)
                ((vector-ref penalties i))
                (else (vector-set! penalties i (expt (+ max-width 1) 3))
                  (vector-set! breaks i i)
                  (let ((k (largest-fit i)))
                    (let lp
                      ((j i) (width 0))
                      (if (<= j k)
                        (let* ((width (+ width (vector-ref widths j)))
                               (break-penalty
                                 (+
                                   (max 0 (expt (- max-width (+ width (- j i))) 3))
                                   (min-penalty! (+ j 1))
                                 ) ;+
                               ) ;break-penalty
                              ) ;
                          (cond ((< break-penalty (vector-ref penalties i))
                                 (vector-set! breaks i j)
                                 (vector-set! penalties i break-penalty)
                                ) ;
                          ) ;cond
                          (lp (+ j 1) width)
                        ) ;let*
                      ) ;if
                    ) ;let
                  ) ;let
                  (if (>= (vector-ref breaks i) len-1) (vector-set! penalties i 0))
                  (vector-ref penalties i)
                ) ;else
          ) ;cond
        ) ;define
        (define (sub-list i j)
          (let lp
            ((k i) (res '()))
            (if (> k j) (reverse res) (lp (+ k 1) (cons (vector-ref vec k) res)))
          ) ;let
        ) ;define
        (cond ((zero? len) (last-line '() knil))
              (else (vector-set! breaks len-1 len-1)
                (vector-set! penalties len-1 0)
                (min-penalty! 0)
                (let lp
                  ((i 0) (acc knil))
                  (let ((break (vector-ref breaks i)))
                    (if (>= break len-1)
                      (last-line (sub-list i len-1) acc)
                      (lp (+ break 1) (line (sub-list i break) acc))
                    ) ;if
                  ) ;let
                ) ;let
              ) ;else
        ) ;cond
      ) ;let*
    ) ;define

    (define (wrapped/list ls)
      (fn (width string-width pad-char)
        (joined (lambda (ls) (joined displayed ls pad-char))
          (reverse (wrap-fold-words ls '() width (or string-width string-length) cons))
          "\n"
        ) ;joined
      ) ;fn
    ) ;define

    (define (wrapped . ls)
      (call-with-output (each-in-list ls)
        (lambda (str)
          (fn (word-separator?)
            (wrapped/list (string-split-words str (or word-separator? char-whitespace?)))
          ) ;fn
        ) ;lambda
      ) ;call-with-output
    ) ;define

    (define (justified . ls)
      (fn (output width string-width)
        (define (justify-line ls)
          (if (null? ls)
            nl
            (let* ((sum
                     (fold
                       (lambda (s n) (+ n ((or string-width string-length) s)))
                       0
                       ls
                     ) ;fold
                   ) ;sum
                   (len (length ls))
                   (diff (max 0 (- width sum)))
                   (sep
                     (make-string (if (= len 1) 0 (quotient diff (- len 1))) #\space)
                   ) ;sep
                   (rem (if (= len 1) diff (remainder diff (- len 1))))
                   (p (open-output-string))
                  ) ;
              (write-string (car ls) p)
              (let lp
                ((ls (cdr ls)) (i 1))
                (when (pair? ls)
                  (write-string sep p)
                  (if (<= i rem) (write-char #\space p))
                  (write-string (car ls) p)
                  (lp (cdr ls) (+ i 1))
                ) ;when
              ) ;let
              (displayed (get-output-string p))
            ) ;let*
          ) ;if
        ) ;define
        (define (justify-last ls)
          (each (joined displayed ls " ") "\n")
        ) ;define
        (call-with-output (each-in-list ls)
          (lambda (str)
            (fn (word-separator?)
              (joined/last justify-line
                justify-last
                (reverse (wrap-fold-words (string-split-words str (or word-separator? char-whitespace?))
                           '()
                           width
                           (or string-width string-length)
                           cons
                         ) ;wrap-fold-words
                ) ;reverse
                "\n"
              ) ;joined/last
            ) ;fn
          ) ;lambda
        ) ;call-with-output
      ) ;fn
    ) ;define

    (define (from-file path . ls)
      (let ((sep (if (pair? ls) (car ls) nl)))
        (fn ()
          (let ((in (open-input-file path)))
            (let lp
              ()
              (let ((line (read-line in)))
                (if (eof-object? line)
                  (begin
                    (close-input-port in)
                    nothing
                  ) ;begin
                  (each line sep (fn () (lp)))
                ) ;if
              ) ;let
            ) ;let
          ) ;let
        ) ;fn
      ) ;let
    ) ;define

    (define (line-numbers . o)
      (let ((n (if (pair? o) (car o) 1)))
        (fn () (let ((res (number->string n))) (set! n (+ n 1)) (displayed res)))
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
