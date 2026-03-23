;
; Copyright (C) 2025 The Goldfish Scheme Authors
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

(define-library (liii rich-either)
  (import (rename (liii rich-option) (rich-option option) (rich-none none))
          (liii oop) (liii base))
  (export rich-either left right)
  (begin

    (define-case-class rich-either
      ((type symbol?)
       (value any?)
      ) ;

      (define (%left?)
        (eq? type 'left)
      ) ;define

      (define (%right?)
        (eq? type 'right)
      ) ;define

      (define (%get)
        value
      ) ;define

      (define (%or-else default)
        (when (not (rich-either :is-type-of default))
          (type-error "The first parameter of either%or-else must be a either case class")
        ) ;when

        (if (%right?)
            (%this)
            default
        ) ;if
      ) ;define

      (define (%get-or-else default)
        (cond ((%right?) value)
              ((and (procedure? default) (not (case-class? default)))
               (default)
              ) ;
              (else default)
        ) ;cond
      ) ;define

      (define (%filter-or-else pred zero)
        (unless (procedure? pred) 
          (type-error 
            (format #f "In funtion #<~a ~a>: argument *~a* must be *~a*!    **Got ~a**" 
              %filter-or-else '(pred zero) 'pred "procedure" (object->string pred)
            ) ;format
          ) ;type-error
        ) ;unless
  
        (unless (any? zero) 
          (type-error 
            (format #f "In funtion #<~a ~a>: argument *~a* must be *~a*!    **Got ~a**" 
              %filter-or-else '(pred zero) 'zero "any" (object->string zero)  
            ) ;format
          ) ;type-error
        ) ;unless
        (if (%right?)
            (if (pred value)
                (%this)
                (left zero)
            ) ;if
            (%this)
        ) ;if
      ) ;define

      (define (%contains x)
        (and (%right?)
             (class=? x value)
        ) ;and
      ) ;define

      (define (%for-each f)
        (when (%right?)
          (f value)
        ) ;when
      ) ;define

      (define (%to-option)
        (if (%right?)
            (option value)
            (none)
        ) ;if
      ) ;define

      (define (%map f . args)
        (chain-apply args
          (if (%right?)
            (right (f value))
            (%this)
          ) ;if
        ) ;chain-apply
      ) ;define

      (define (%flat-map f . args)
        (chain-apply args
          (if (%right?)
            (f value)
            (%this)
          ) ;if
        ) ;chain-apply
      ) ;define

      (define (%forall pred)
        (unless (procedure? pred) 
          (type-error 
            (format #f "In funtion #<~a ~a>: argument *~a* must be *~a*!    **Got ~a**" 
              %forall '(pred) 'pred "procedure" (object->string pred)
            ) ;format
          ) ;type-error
        ) ;unless
        (if (%right?)
            (pred value)
            #t
        ) ;if
      ) ;define

      (define (%exists pred)
        (unless (procedure? pred) 
          (type-error 
            (format #f "In funtion #<~a ~a>: argument *~a* must be *~a*!    **Got ~a**" 
              %exists '(pred) 'pred "procedure" (object->string pred)
            ) ;format
          ) ;type-error
        ) ;unless
        (if (%right?)
            (pred value)
            #f)
        ) ;if

      ) ;define

    (define (left v)
      (rich-either 'left v)
    ) ;define

    (define (right v)
      (rich-either 'right v)
    ) ;define


    ) ;define-case-class
  ) ;begin
