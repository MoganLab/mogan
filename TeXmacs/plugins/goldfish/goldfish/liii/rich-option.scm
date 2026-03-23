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

(define-library (liii rich-option)
  (import (liii oop) (liii base))
  (export rich-option rich-none)
  (begin

    (define-final-class rich-option ((value any?))

      (define (%get)
        (if (null? value)
            (value-error "option is empty, cannot get value")
            value
        ) ;if
      ) ;define

      (define (%get-or-else default)
        (cond ((not (null? value)) value)
              ((and (procedure? default) (not (case-class? default)))
               (default)
              ) ;
              (else default)
        ) ;cond
      ) ;define

      (define (%or-else default . args)
        (when (not (rich-option :is-type-of default))
          (type-error "The first parameter of rich-option%or-else must be a rich-option case class")
        ) ;when

        (chain-apply args
          (if (null? value)
              default
              (rich-option value)
          ) ;if
        ) ;chain-apply
      ) ;define

      (define (%equals that)
        (and (rich-option :is-type-of that)
             (class=? value (that 'value))
        ) ;and
      ) ;define

      (define (%defined?) (not (null? value)))
  
      (define (%empty?)
        (null? value)
      ) ;define

      (define (%forall f)
        (if (null? value)
            #f
            (f value)
        ) ;if
      ) ;define

      (define (%exists f)
        (if (null? value)
            #f
            (f value)
        ) ;if
      ) ;define

      (define (%contains elem)
        (if (null? value)
            #f
            (equal? value elem)
        ) ;if
      ) ;define

      (define (%for-each f)
        (when (not (null? value))
              (f value)
        ) ;when
      ) ;define

      (define (%map f . args)
        (chain-apply args
          (if (null? value)
              (rich-option '())
              (rich-option (f value))
          ) ;if
        ) ;chain-apply
      ) ;define

      (define (%flat-map f . args)
        (chain-apply args
          (if (null? value)
              (rich-option '())
              (f value)
          ) ;if
        ) ;chain-apply
      ) ;define

      (define (%filter pred . args)
        (chain-apply args
          (if (or (null? value) (not (pred value)))
              (rich-option '())
              (rich-option value))
          ) ;if
        ) ;chain-apply

      ) ;define

    (define (rich-none) (rich-option '()))

    ) ;define-final-class
  ) ;begin
