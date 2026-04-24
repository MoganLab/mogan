;
; Copyright (C) 2026 The Goldfish Scheme Authors
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
(import (liii error))
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
      (let ((arg (car args)))
        (if (list? arg)
            (%make-stack arg)
            (type-error (format #f "make-stack in (liii stack): argument must be *list* type! **Got ~a**" (object->string arg)))
        ) ;if
      ) ;let
  ) ;if
) ;define

(define (stack . elements)
  (%make-stack elements)
) ;define

; Predicates
(define (stack-empty? s)
  (unless (stack? s)
    (type-error (format #f "stack-empty? in (liii stack): argument *s* must be *stack* type! **Got ~a**" (object->string s)))
  ) ;unless
  (null? (stack-elements s))
) ;define

; Accessors
(define (stack-top s)
  (unless (stack? s)
    (type-error (format #f "stack-top in (liii stack): argument *s* must be *stack* type! **Got ~a**" (object->string s)))
  ) ;unless
  (if (stack-empty? s)
      (value-error "stack-top in (liii stack): stack is empty")
      (car (stack-elements s))
  ) ;if
) ;define

(define (stack-size s)
  (unless (stack? s)
    (type-error (format #f "stack-size in (liii stack): argument *s* must be *stack* type! **Got ~a**" (object->string s)))
  ) ;unless
  (length (stack-elements s))
) ;define

; Mutators
(define (stack-push! s elem)
  (unless (stack? s)
    (type-error (format #f "stack-push! in (liii stack): argument *s* must be *stack* type! **Got ~a**" (object->string s)))
  ) ;unless
  (stack-elements-set! s (cons elem (stack-elements s)))
  s
) ;define

(define (stack-pop! s)
  (unless (stack? s)
    (type-error (format #f "stack-pop! in (liii stack): argument *s* must be *stack* type! **Got ~a**" (object->string s)))
  ) ;unless
  (if (stack-empty? s)
      (value-error "stack-pop! in (liii stack): stack is empty")
      (let ((top (car (stack-elements s))))
        (stack-elements-set! s (cdr (stack-elements s)))
        top
      ) ;let
  ) ;if
) ;define

; Conversion
(define (stack->list s)
  (unless (stack? s)
    (type-error (format #f "stack->list in (liii stack): argument *s* must be *stack* type! **Got ~a**" (object->string s)))
  ) ;unless
  (stack-elements s)
) ;define

(define (list->stack lst)
  (unless (list? lst)
    (type-error (format #f "list->stack in (liii stack): argument *lst* must be *list* type! **Got ~a**" (object->string lst)))
  ) ;unless
  (%make-stack lst)
) ;define

; Mapping
(define (stack-map proc s)
  (unless (procedure? proc)
    (type-error (format #f "stack-map in (liii stack): argument *proc* must be *procedure* type! **Got ~a**" (object->string proc)))
  ) ;unless
  (unless (stack? s)
    (type-error (format #f "stack-map in (liii stack): argument *s* must be *stack* type! **Got ~a**" (object->string s)))
  ) ;unless
  (%make-stack (map proc (stack-elements s)))
) ;define

(define (stack-map! proc s)
  (unless (procedure? proc)
    (type-error (format #f "stack-map! in (liii stack): argument *proc* must be *procedure* type! **Got ~a**" (object->string proc)))
  ) ;unless
  (unless (stack? s)
    (type-error (format #f "stack-map! in (liii stack): argument *s* must be *stack* type! **Got ~a**" (object->string s)))
  ) ;unless
  (stack-elements-set! s (map proc (stack-elements s)))
  s
) ;define

(define (stack-for-each proc s)
  (unless (procedure? proc)
    (type-error (format #f "stack-for-each in (liii stack): argument *proc* must be *procedure* type! **Got ~a**" (object->string proc)))
  ) ;unless
  (unless (stack? s)
    (type-error (format #f "stack-for-each in (liii stack): argument *s* must be *stack* type! **Got ~a**" (object->string s)))
  ) ;unless
  (for-each proc (stack-elements s))
) ;define

; Copy
(define (stack-copy s)
  (unless (stack? s)
    (type-error (format #f "stack-copy in (liii stack): argument *s* must be *stack* type! **Got ~a**" (object->string s)))
  ) ;unless
  (%make-stack (stack-elements s))
) ;define

) ;begin
) ;define-library
