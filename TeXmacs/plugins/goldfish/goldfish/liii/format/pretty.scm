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

(define-library (liii format pretty)
  (import (scheme base)
    (scheme char)
    (scheme write)
    (srfi srfi-1)
    (liii hash-table)
    (liii format base)
    (liii format color)
  ) ;import
  (export pretty pretty-shared pretty-simply pretty-with-color)
  (begin

    (define (take* ls n)
      (cond ((zero? n) '())
            ((pair? ls) (cons (car ls) (take* (cdr ls) (- n 1))))
            (else '())
      ) ;cond
    ) ;define

    (define (drop* ls n)
      (cond ((zero? n) ls)
            ((pair? ls) (drop* (cdr ls) (- n 1)))
            (else ls)
      ) ;cond
    ) ;define

    (define (make-space n)
      (make-string n #\space)
    ) ;define
    (define (make-nl-space n)
      (string-append "\n" (make-string n #\space))
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

    (define (joined/shares fmt ls shares . o)
      (let ((sep (displayed (if (pair? o) (car o) " "))))
        (fn ()
          (cond ((null? ls) nothing)
                ((pair? ls)
                 (fn ()
                   (let lp
                     ((ls ls))
                     (each (fmt (car ls))
                       (let ((rest (cdr ls)))
                         (cond ((null? rest) nothing)
                               ((pair? rest) (call-with-shared-ref/cdr rest shares each (fn () (lp rest)) sep))
                               (else (each sep ". " (fmt rest)))
                         ) ;cond
                       ) ;let
                     ) ;each
                   ) ;let
                 ) ;fn
                ) ;
                (else (fmt ls))
          ) ;cond
        ) ;fn
      ) ;let
    ) ;define

    (define (write-to-string x)
      (call-with-output-string (lambda (out) (write x out)))
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

    (define (try-fitted2 proc fail)
      (fn (width string-width (orig-output output) (current-col col))
        (call-with-output proc
          (lambda (str)
            (let ((lines (string-split-lines str)))
              (define (fits?)
                (and (pair? lines)
                  (<= (+ (string-width (car lines)) current-col) width)
                  (every (lambda (line) (<= (string-width line) width)) (cdr lines))
                ) ;and
              ) ;define
              (if (fits?) (orig-output str) fail)
            ) ;let
          ) ;lambda
        ) ;call-with-output
      ) ;fn
    ) ;define

    (define (try-fitted proc . fail)
      (let lp
        ((proc proc) (ls fail))
        (if (null? ls) proc (try-fitted2 proc (lp (car ls) (cdr ls))))
      ) ;let
    ) ;define

    (define (fits-in-columns width ls writer set-result!)
      (let ((max-w (quotient width 2)))
        (fn (string-width)
          (let lp
            ((ls ls) (res '()) (widest 0))
            (cond
             ((pair? ls)
              (call-with-output (writer (car ls))
                (lambda (str)
                  (if (or (string-contains-newline? str) (> (string-width str) max-w))
                    (begin
                      (set-result! #f)
                      nothing
                    ) ;begin
                    (lp (cdr ls) (cons str res) (max (string-width str) widest))
                  ) ;if
                ) ;lambda
              ) ;call-with-output
             ) ;
             ((null? ls) (set-result! (cons widest (reverse res))) nothing)
             (else (set-result! #f) nothing)
            ) ;cond
          ) ;let
        ) ;fn
      ) ;let
    ) ;define

    (define syntax-abbrevs
      '((quote . "'")
        (quasiquote . "`")
        (unquote . ",")
        (unquote-splicing . ",@"))
    ) ;define

    (define (pp-let ls pp shares color?)
      (if (and (pair? (cdr ls)) (symbol? (cadr ls)))
        (pp-with-indent 2 ls pp shares color?)
        (pp-with-indent 1 ls pp shares color?)
      ) ;if
    ) ;define

    (define indent-rules
      `((lambda . ,1)
        (define . ,1)
        (define-syntax . ,1)
        (let . ,pp-let)
        (loop . ,pp-let)
        (let* . ,1)
        (letrec . ,1)
        (letrec* . ,1)
        (and-let* . ,1)
        (let1 . ,2)
        (let-values . ,1)
        (let*-values . ,1)
        (receive . ,2)
        (parameterize . ,1)
        (let-syntax . ,1)
        (letrec-syntax . ,1)
        (syntax-rules . ,1)
        (syntax-case . ,2)
        (match . ,1)
        (match-let . ,1)
        (match-let* . ,1)
        (if . ,3)
        (when . ,1)
        (unless . ,1)
        (case . ,1)
        (while . ,1)
        (until . ,1)
        (do . ,2)
        (dotimes . ,1)
        (dolist . ,1)
        (test . ,1)
        (condition-case . ,1)
        (guard . ,1)
        (rec . ,1)
        (call-with-current-continuation . ,0))
    ) ;define

    (define indent-prefix-rules
      `((,"with-" . ,-1) (,"call-with-" . ,-1) (,"define-" . ,1))
    ) ;define

    (define indent-suffix-rules `((,"-case" . ,1)))

    (define pp-macros
      (append (map car indent-rules)
        '(quote quasiquote unquote unquote-splicing set! cond-expand cond)
      ) ;append
    ) ;define

    (define (length+ ls)
      (let lp
        ((fast ls) (slow ls) (len 0) (step 0))
        (cond ((null? fast) len)
              ((not (pair? fast)) #f)
              ((and (> len 0) (eq? fast slow)) #f)
              (else (lp (cdr fast) (if (odd? step) (cdr slow) slow) (+ len 1) (+ step 1)))
        ) ;cond
      ) ;let
    ) ;define

    (define (string-prefix? prefix str)
      (let ((p-len (string-length prefix)) (s-len (string-length str)))
        (and (>= s-len p-len) (string=? prefix (substring str 0 p-len)))
      ) ;let
    ) ;define

    (define (string-suffix? suffix str)
      (let ((p-len (string-length suffix)) (s-len (string-length str)))
        (and (>= s-len p-len) (string=? suffix (substring str (- s-len p-len) s-len)))
      ) ;let
    ) ;define

    (define (pp-indentation form)
      (let ((indent
              (cond ((assq (car form) indent-rules) => cdr)
                    ((and (symbol? (car form))
                       (let ((str (symbol->string (car form))))
                         (or
                           (find (lambda (rx) (string-prefix? (car rx) str)) indent-prefix-rules)
                           (find (lambda (rx) (string-suffix? (car rx) str)) indent-suffix-rules)
                         ) ;or
                       ) ;let
                     ) ;and
                     =>
                     cdr
                    ) ;
                    (else #f)
              ) ;cond
            ) ;indent
           ) ;
        (if (and (number? indent) (negative? indent))
          (max 0 (- (+ (length+ form) indent) 1))
          indent
        ) ;if
      ) ;let
    ) ;define

    (define (with-reset-shares shares proc)
      (let ((orig-count (cdr shares)))
        (fn ()
          (let ((new-count (cdr shares)))
            (when (> new-count orig-count)
              (hash-table-for-each
                (lambda (k v)
                  (if (and (pair? v) (cdr v) (>= (car v) orig-count)) (set-cdr! v #f))
                ) ;lambda
                (car shares)
              ) ;hash-table-for-each
              (set-cdr! shares orig-count)
            ) ;when
            proc
          ) ;let
        ) ;fn
      ) ;let
    ) ;define

    (define (pp-with-indent indent-rule ls pp shares color?)
      (fn ((col1 col))
        (each "("
         ((if (and color? (memq (car ls) pp-macros)) as-blue displayed) (pp (car ls)))
         (fn ((col2 col) width string-width)
           (let ((fixed (take* (cdr ls) (or indent-rule 1)))
                 (tail (drop* (cdr ls) (or indent-rule 1)))
                 (default
                   (let ((sep (make-nl-space (+ col1 1))))
                     (fn () (each sep (joined/shares pp (cdr ls) shares sep)))
                   ) ;let
                 ) ;default
                 (reset-shares (with-reset-shares shares nothing))
                ) ;
             (call-with-output
               (trimmed/lazy (- width col2)
                 (each (if (or (null? fixed) (pair? fixed)) " " " . ")
                   (joined/shares (lambda (x) (pp-flat x pp shares color?)) fixed shares " ")
                 ) ;each
               ) ;trimmed/lazy
               (lambda (first-line)
                 (cond
                  ((< (+ col2 (string-width first-line)) width)
                   (let ((sep (make-nl-space (if indent-rule (+ col1 2) (+ col2 1)))))
                     (each first-line
                       (cond
                        ((not (or (null? tail) (pair? tail))) (each ". " (pp tail)))
                        ((> (or (length+ (cdr ls)) 999999) (or indent-rule 1))
                         (each sep (joined/shares pp tail shares sep))
                        ) ;
                        (else nothing)
                       ) ;cond
                     ) ;each
                   ) ;let
                  ) ;
                  (indent-rule
                    (try-fitted
                      (each reset-shares
                        " "
                        (joined/shares pp fixed shares (make-nl-space (+ col2 1)))
                        (if (pair? tail)
                          (let ((sep (make-nl-space (+ col1 2))))
                            (each sep (joined/shares pp tail shares sep))
                          ) ;let
                          nothing
                        ) ;if
                      ) ;each
                      (each reset-shares default)
                    ) ;try-fitted
                  ) ;indent-rule
                  (else (each reset-shares default))
                 ) ;cond
               ) ;lambda
             ) ;call-with-output
           ) ;let
         ) ;fn
         ")"
        ) ;each
      ) ;fn
    ) ;define

    (define (pp-app ls pp shares color?)
      (let ((indent-rule (pp-indentation ls)))
        (if (procedure? indent-rule)
          (indent-rule ls pp shares color?)
          (pp-with-indent indent-rule ls pp shares color?)
        ) ;if
      ) ;let
    ) ;define

    (define (proper-non-shared-list? ls shares)
      (let ((tab (car shares)))
        (let lp
          ((ls ls))
          (or (null? ls)
            (and (pair? ls) (not (hash-table-ref/default tab ls #f)) (lp (cdr ls)))
          ) ;or
        ) ;let
      ) ;let
    ) ;define

    (define (non-app? x)
      (if (pair? x)
        (or
          (not (or (null? (cdr x)) (pair? (cdr x))))
          (non-app? (car x))
        ) ;or
        (not (symbol? x))
      ) ;if
    ) ;define

    (define (pp-data-list ls pp shares)
      (each "("
        (fn (col width string-width)
          (let ((avail (- width col)))
            (cond
             ((and (pair? (cdr ls)) (pair? (cddr ls)) (pair? (cdr (cddr ls))))
              (let ((out (open-output-string)) (result #f))
                (call-with-output (fits-in-columns width
                                    ls
                                    (lambda (x) (pp-flat x pp shares #f))
                                    (lambda (res) (set! result res))
                                  ) ;fits-in-columns
                  (lambda (str)
                    (fn ()
                      (if (not result)
                        (joined/shares pp ls shares (make-nl-space col))
                        (let* ((prefix (make-nl-space col))
                               (widest (+ 1 (car result)))
                               (columns (quotient width widest))
                              ) ;
                          (let lp
                            ((ls (cdr result)) (i 1))
                            (cond ((null? ls) nothing)
                                  ((null? (cdr ls)) (displayed (car ls)))
                                  ((>= i columns) (each (car ls) prefix (fn () (lp (cdr ls) 1))))
                                  (else
                                    (let ((pad (- widest (string-width (car ls)))))
                                      (each (car ls) (make-space pad) (lp (cdr ls) (+ i 1)))
                                    ) ;let
                                  ) ;else
                            ) ;cond
                          ) ;let
                        ) ;let*
                      ) ;if
                    ) ;fn
                  ) ;lambda
                ) ;call-with-output
              ) ;let
             ) ;
             (else (joined/shares pp ls shares (make-nl-space col)))
            ) ;cond
          ) ;let
        ) ;fn
        ")"
      ) ;each
    ) ;define

    (define (pp-flat x pp shares color?)
      (define (ppf x)
        (pp-flat x pp shares color?)
      ) ;define
      (cond
       ((pair? x)
        (cond
         ((and (pair? (cdr x)) (null? (cddr x)) (assq (car x) syntax-abbrevs))
          =>
          (lambda (abbrev)
            (each (cdr abbrev)
              (call-with-shared-ref (cadr x) shares each (pp-flat (cadr x) pp shares color?))
            ) ;each
          ) ;lambda
         ) ;
         (else
           (fn ()
             (each "("
              ((if (and color? (memq (car x) pp-macros)) as-blue displayed) (pp (car x)))
              (if (null? (cdr x))
                nothing
                (call-with-shared-ref/cdr (cdr x)
                  shares
                  each
                  (cond
                   ((pair? (cdr x)) (each "" (joined/shares ppf (cdr x) shares " ")))
                   (else (each ". " (joined/shares ppf (cdr x) shares " ")))
                  ) ;cond
                  " "
                ) ;call-with-shared-ref/cdr
              ) ;if
              ")"
             ) ;each
           ) ;fn
         ) ;else
        ) ;cond
       ) ;
       ((vector? x) (each "#(" (joined/shares ppf (vector->list x) shares " ") ")"))
       (else (pp x))
      ) ;cond
    ) ;define

    (define (pp-pair ls pp shares color?)
      (cond
       ((null? (cdr ls)) (each "(" (pp (car ls)) ")"))
       ((and (pair? (cdr ls)) (null? (cddr ls)) (assq (car ls) syntax-abbrevs))
        =>
        (lambda (abbrev) (each (cdr abbrev) (pp (cadr ls))))
       ) ;
       (else
         (let ((reset-shares (with-reset-shares shares nothing)))
           (try-fitted (pp-flat ls pp shares color?)
             (each reset-shares
               (fn ()
                 (if (and (non-app? ls) (proper-non-shared-list? ls shares))
                   (pp-data-list ls pp shares)
                   (pp-app ls pp shares color?)
                 ) ;if
               ) ;fn
             ) ;each
           ) ;try-fitted
         ) ;let
       ) ;else
      ) ;cond
    ) ;define

    (define (pp-vector vec pp shares)
      (each "#" (pp-data-list (vector->list vec) pp shares))
    ) ;define

    (define (pp obj shares color?)
      (fn (radix precision)
        (let ((write-number
                (cond
                 ((and (not precision)
                    (assv radix '((16 . "#x") (10 . "") (8 . "#o") (2 . "#b")))
                  ) ;and
                  =>
                  (lambda (cell)
                    (lambda (n)
                      (if (or (exact? n) (eqv? radix 10))
                        (each (cdr cell) (number->string n (car cell)))
                        (with ((radix 10)) (numeric n))
                      ) ;if
                    ) ;lambda
                  ) ;lambda
                 ) ;
                 (else
                   (lambda (n) (with ((radix 10)) (numeric n)))
                 ) ;else
                ) ;cond
              ) ;write-number
             ) ;
          (let pp
            ((obj obj))
            (call-with-shared-ref obj
              shares
              each
              (fn ()
                (cond ((pair? obj) (pp-pair obj pp shares color?))
                      ((vector? obj) (pp-vector obj pp shares))
                      ((number? obj) (write-number obj))
                      ((and color? (string? obj)) (as-green (write-to-string obj)))
                      (else (displayed (write-to-string obj)))
                ) ;cond
              ) ;fn
            ) ;call-with-shared-ref
          ) ;let
        ) ;let
      ) ;fn
    ) ;define

    (define (pretty obj)
      (fn ()
        (call-with-output (each (pp obj (extract-shared-objects obj #t) #f) fl)
          displayed
        ) ;call-with-output
      ) ;fn
    ) ;define

    (define (pretty-shared obj)
      (fn ()
        (call-with-output (each (pp obj (extract-shared-objects obj #f) #f) fl)
          displayed
        ) ;call-with-output
      ) ;fn
    ) ;define

    (define (pretty-simply obj)
      (fn () (each (pp obj (extract-shared-objects #f #f) #f) fl))
    ) ;define

    (define (pretty-with-color obj)
      (fn ()
        (call-with-output (each (pp obj (extract-shared-objects obj #t) #t) fl)
          displayed
        ) ;call-with-output
      ) ;fn
    ) ;define

  ) ;begin
) ;define-library
