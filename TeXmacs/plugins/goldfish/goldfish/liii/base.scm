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

(define-library (liii base)
  (import (scheme base)
          (srfi srfi-2)
          (srfi srfi-8)
  ) ;import
  (export
    ; (scheme base) defined by R7RS
    let-values
    ; R7RS 5: Program Structure
    define-values define-record-type lambda case cond if
    ; R7RS 6.2: Numbers
    + - * / abs square exact inexact max min floor floor/ s7-floor ceiling s7-ceiling truncate truncate/ s7-truncate
    round s7-round floor-quotient floor-remainder gcd lcm s7-lcm modulo exact-integer-sqrt
    quotient remainder rationalize numerator denominator
    zero? positive? negative? odd? even?
    complex? real? rational? integer? exact? inexact? exact-integer?
    number? number->string string->number
    ; R7RS 6.3: Booleans
    boolean=? boolean? not
    ; R7RS 6.4: list
    pair? cons car cdr set-car! set-cdr! caar cadr cdar cddr
    null? list? make-list list length append reverse list-tail
    list-ref list-set! memq memv member assq assv assoc list-copy map
    ; R7RS 6.5: Symbol
    symbol? symbol=? eq? eqv? equal? string->symbol symbol->string
    ; R7RS 6.6: Characters
    char? char=? char<? char>? char<=? char>=? char->integer integer->char digit-value
    ; R7RS 6.7: String
    string? make-string string=? string-ci=? string-length string-ref string-set! string-copy
    ; R7RS 6.8 Vector
    vector->string string->vector vector-copy vector-copy! vector-fill! vector-append
    ; R7RS 6.9 Bytevectors
    bytevector? make-bytevector bytevector bytevector-length bytevector-u8-ref
    bytevector-u8-set! bytevector-copy bytevector-append
    utf8->string string->utf8 utf8-string-length u8-substring bytevector-advance-utf8
    ; Input and Output
    call-with-port port? binary-port? textual-port? input-port-open? output-port-open?
    open-binary-input-file open-binary-output-file close-port eof-object
    open-input-string open-output-string get-output-string read write
    ; Control flow
    string-map vector-map string-for-each vector-for-each and
    ; Exception
    raise guard read-error? file-error?
    ; SRFI-2
    and-let*
    ; SRFI-8
    receive
    ; Extra routines
    loose-car loose-cdr compose identity any?
    ; Extra structure
    typed-lambda
  ) ;export
  (begin

    (define* (u8-substring str (start 0) (end #t))
      (utf8->string (string->utf8 str start end))
    ) ;define*

    (define (loose-car pair-or-empty)
      (if (eq? '() pair-or-empty)
          '()
          (car pair-or-empty)
      ) ;if
    ) ;define

    (define (loose-cdr pair-or-empty)
      (if (eq? '() pair-or-empty)
          '()
          (cdr pair-or-empty)
      ) ;if
    ) ;define

    (define identity (lambda (x) x))

    (define (compose . fs)
      (if (null? fs)
          (lambda (x) x)
          (lambda (x)
            ((car fs) ((apply compose (cdr fs)) x))
          ) ;lambda
      ) ;if
    ) ;define
  
    (define (any? x) #t)

    ; 0 clause BSD, from S7 repo stuff.scm
    (define-macro (typed-lambda args . body)
      ; (typed-lambda ((var [type])...) ...)
      (if (symbol? args)
          (apply lambda args body)
          (let ((new-args (copy args)))
            (do ((p new-args (cdr p)))
                ((not (pair? p)))
                (if (pair? (car p))
                    (set-car! p (caar p))
                ) ;if
            ) ;do
            `(lambda ,new-args
               ,@(map (lambda (arg)
                        (if (pair? arg)
                            `(unless (,(cadr arg) ,(car arg))
                               (error 'type-error
                                 "~S is not ~S~%" ',(car arg) ',(cadr arg)))
                            (values)))
                      args)
               ,@body)
          ) ;let
      ) ;if
    ) ;define-macro

  ) ;begin
) ;define-library
