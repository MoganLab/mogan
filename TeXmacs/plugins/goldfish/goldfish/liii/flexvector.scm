;
; Copyright (C) 2020 Adam Nelson
;
; Permission is hereby granted, free of charge, to any person obtaining a
; copy of this software and associated documentation files (the
; "Software"), to deal in the Software without restriction, including
; without limitation the rights to use, copy, modify, merge, publish,
; distribute, sublicense, and/or sell copies of the Software, and to
; permit persons to whom the Software is furnished to do so, subject to
; the following conditions:
;
; The above copyright notice and this permission notice shall be included
; in all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
; OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
; IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
; CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
; TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
; SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

(define-library (liii flexvector)
  (import (srfi srfi-214))
  (export
    ;; Constructors
    make-flexvector flexvector
    ;; Predicates
    flexvector? flexvector-empty?
    ;; Accessors
    flexvector-ref flexvector-front flexvector-back
    flexvector-length
    ;; Mutators
    flexvector-set! flexvector-add! flexvector-add-back!
    flexvector-add-front! flexvector-remove! flexvector-remove-back!
    flexvector-remove-front! flexvector-remove-range!
    flexvector-clear! flexvector-fill! flexvector-swap! flexvector-reverse!
    ;; Conversion
    flexvector->vector vector->flexvector
    flexvector->list list->flexvector reverse-flexvector->list reverse-list->flexvector
    flexvector->string string->flexvector
    ;; Copying
    flexvector-copy flexvector-copy!
    flexvector-reverse-copy flexvector-reverse-copy!
    ;; Iteration
    flexvector-for-each flexvector-for-each/index
    flexvector-map flexvector-map! flexvector-map/index flexvector-map/index!
    flexvector-fold flexvector-fold-right
    flexvector-filter flexvector-filter! flexvector-filter/index flexvector-filter/index!
    flexvector-append-map flexvector-append-map/index
    flexvector-count flexvector-cumulate
    ;; Searching
    flexvector-index flexvector-index-right flexvector-skip flexvector-skip-right
    flexvector-any flexvector-every flexvector-binary-search
    ;; Partitioning
    flexvector-partition
    ;; Concatenation
    flexvector-append flexvector-concatenate flexvector-append-subvectors
    flexvector-append!
    ;; Comparison
    flexvector=?
    ;; Unfolding
    flexvector-unfold flexvector-unfold-right
    ;; Generators
    flexvector->generator generator->flexvector
  ) ;export
) ;define-library
