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
    ; SRFI-2
    and-let*
    ; SRFI-8
    receive
    ; S7 extensions
    define*
    procedure-source
    procedure-arglist
    arity
    defined?
    object->string
    eval-string
    signature
    ; Keywords
    keyword?
    string->keyword
    symbol->keyword
    keyword->symbol
    ; Extra routines
    loose-car
    loose-cdr
    compose
    identity
    any?
    ; Extra structure
    typed-lambda
  ) ;export
  (begin

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
