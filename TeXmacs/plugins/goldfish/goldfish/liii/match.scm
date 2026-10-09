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

(define-library (liii match)
  (export match match-lambda match-lambda* match-let match-let* match-letrec)
  (import (scheme base))
  (begin
    (define-syntax is-a?
      (syntax-rules ()
        ((_ rec rtd) #f)
      ) ;syntax-rules
    ) ;define-syntax

    (define-syntax slot-ref
      (syntax-rules ()
        ((_ rtd rec n) #f)
      ) ;syntax-rules
    ) ;define-syntax

    (define-syntax slot-set!
      (syntax-rules ()
        ((_ rtd rec n value) #f)
      ) ;syntax-rules
    ) ;define-syntax
  ) ;begin
  (include "match/match-core.scm")
) ;define-library
