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

(define-library (liii stack)
(export
  ; Constructors
  make-stack stack
  ; Predicates
  stack? stack-empty?
  ; Accessors
  stack-top stack-size
  ; Mutators
  stack-push! stack-pop!
  ; Conversion
  stack->list list->stack
  ; Mapping
  stack-map stack-map! stack-for-each
  ; Copy
  stack-copy
) ;export
(begin

(define-record-type stack
  (%make-stack elements)
  stack?
  (elements stack-elements stack-elements-set!)
) ;define-record-type

; Constructors
(define (make-stack . args)
  (if (null? args)
      (%make-stack '())
      (%make-stack (car args))
  ) ;if
) ;define

(define (stack . elements)
  (%make-stack elements)
) ;define

; Predicates
(define (stack-empty? s)
  (null? (stack-elements s))
) ;define

; Accessors
(define (stack-top s)
  (if (stack-empty? s)
      (error 'stack-top "stack is empty")
      (car (stack-elements s))
  ) ;if
) ;define

(define (stack-size s)
  (length (stack-elements s))
) ;define

; Mutators
(define (stack-push! s elem)
  (stack-elements-set! s (cons elem (stack-elements s)))
  s
) ;define

(define (stack-pop! s)
  (if (stack-empty? s)
      (error 'stack-pop! "stack is empty")
      (let ((top (car (stack-elements s))))
        (stack-elements-set! s (cdr (stack-elements s)))
        top
      ) ;let
  ) ;if
) ;define

; Conversion
(define (stack->list s)
  (stack-elements s)
) ;define

(define (list->stack lst)
  (%make-stack lst)
) ;define

; Mapping
(define (stack-map proc s)
  (%make-stack (map proc (stack-elements s)))
) ;define

(define (stack-map! proc s)
  (stack-elements-set! s (map proc (stack-elements s)))
  s
) ;define

(define (stack-for-each proc s)
  (for-each proc (stack-elements s))
) ;define

; Copy
(define (stack-copy s)
  (%make-stack (stack-elements s))
) ;define

) ;begin
) ;define-library
