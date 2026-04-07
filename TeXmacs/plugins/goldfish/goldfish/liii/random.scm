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

;;
;; (liii random) - Random number generation
;;
;; This module exports the SRFI-27 interface for random number generation.
;; It re-exports all symbols from (srfi srfi-27).
;;
;; Exported functions:
;;   random-integer          - Generate random integer in [0, n-1]
;;   random-real             - Generate random real in (0, 1)
;;   default-random-source   - The default random source
;;   make-random-source      - Create a new random source
;;   random-source?          - Check if object is a random source
;;   random-source-state-ref - Get the state of a random source
;;   random-source-state-set!- Set the state of a random source
;;   random-source-randomize!- Randomize a source with current time
;;   random-source-pseudo-randomize! - Deterministically set source state
;;   random-source-make-integers - Create integer generator from source
;;   random-source-make-reals    - Create real generator from source

(define-library (liii random)
  (import (srfi srfi-27))
  (export
    random-integer
    random-real
    default-random-source
    make-random-source
    random-source?
    random-source-state-ref
    random-source-state-set!
    random-source-randomize!
    random-source-pseudo-randomize!
    random-source-make-integers
    random-source-make-reals
  ) ;export
) ;define-library
