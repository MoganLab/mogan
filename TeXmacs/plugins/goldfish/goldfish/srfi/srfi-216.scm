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

(define-library (srfi srfi-216)
  (export true false nil runtime)
  (import (scheme time))
  (begin

    (define true #t)

    (define false #f)

    (define nil '())

    (define (runtime)
      (round (* 1000 (current-second)))
    ) ;define

  ) ;begin
) ;define-library
