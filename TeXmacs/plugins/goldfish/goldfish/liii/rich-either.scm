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
  (import (liii option) (liii oop) (liii base))
  (export rich-either left right)
  (begin

    (define-case-class rich-either
      ((type symbol?)
       (value any?))

      (define (%left?)
        (eq? type 'left))

      (define (%right?)
        (eq? type 'right))

      (define (%get)
        value)

      (define (%or-else default)
        (when (not (rich-either :is-type-of default))
          (type-error "The first parameter of either%or-else must be a either case class"))

        (if (%right?)
            (%this)
            default))

      (define (%get-or-else default)
        (cond ((%right?) value)
              ((and (procedure? default) (not (case-class? default)))
               (default))
              (else default)))

      (define (%filter-or-else pred zero)
        (unless (procedure? pred) 
          (type-error 
            (format #f "In funtion #<~a ~a>: argument *~a* must be *~a*!    **Got ~a**" 
              %filter-or-else '(pred zero) 'pred "procedure" (object->string pred))))
  
        (unless (any? zero) 
          (type-error 
            (format #f "In funtion #<~a ~a>: argument *~a* must be *~a*!    **Got ~a**" 
              %filter-or-else '(pred zero) 'zero "any" (object->string zero))))  
        (if (%right?)
            (if (pred value)
                (%this)
                (left zero))
            (%this)))

      (define (%contains x)
        (and (%right?)
             (class=? x value)))

      (define (%for-each f)
        (when (%right?)
          (f value)))

      (define (%to-option)
        (if (%right?)
            (option value)
            (none)))

      (define (%map f . args)
        (chain-apply args
          (if (%right?)
            (right (f value))
            (%this))))

      (define (%flat-map f . args)
        (chain-apply args
          (if (%right?)
            (f value)
            (%this))))

      (define (%forall pred)
        (unless (procedure? pred) 
          (type-error 
            (format #f "In funtion #<~a ~a>: argument *~a* must be *~a*!    **Got ~a**" 
              %forall '(pred) 'pred "procedure" (object->string pred))))
        (if (%right?)
            (pred value)
            #t))

      (define (%exists pred)
        (unless (procedure? pred) 
          (type-error 
            (format #f "In funtion #<~a ~a>: argument *~a* must be *~a*!    **Got ~a**" 
              %exists '(pred) 'pred "procedure" (object->string pred))))
        (if (%right?)
            (pred value)
            #f))

      )

    (define (left v)
      (rich-either 'left v))

    (define (right v)
      (rich-either 'right v))


    ) ; end of begin
  ) ; end of define-library
