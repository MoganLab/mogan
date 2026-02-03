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

(define-library (liii bag)
  (import (rename (srfi srfi-113)
                  (bag make-bag-with-comparator)
                  (list->bag list->bag-with-comparator))
          (only (srfi srfi-113)
                bag-unfold bag-member bag-comparator bag->list
                list->bag! bag-copy
                bag? bag-contains? bag-empty? bag-disjoint?
                bag-size bag-find bag-count bag-any? bag-every?
                bag=? bag<? bag>? bag<=? bag>=?
                bag-union bag-intersection bag-difference bag-xor
                bag-union! bag-intersection! bag-difference! bag-xor!
                bag-adjoin bag-adjoin! bag-replace bag-replace!
                bag-delete bag-delete! bag-delete-all bag-delete-all!
                bag-search!)
          (srfi srfi-128))
  (export bag bag-unfold bag-member bag-comparator
          bag->list list->bag list->bag! bag-copy
          bag? bag-contains? bag-empty? bag-disjoint?
          bag-size bag-find bag-count bag-any? bag-every?
          bag=? bag<? bag>? bag<=? bag>=?
          bag-union bag-intersection bag-difference bag-xor
          bag-union! bag-intersection! bag-difference! bag-xor!
          bag-adjoin bag-adjoin! bag-replace bag-replace!
          bag-delete bag-delete! bag-delete-all bag-delete-all!
          bag-search!)

  (define comp (make-default-comparator))

  (define (bag . elements)
    (apply make-bag-with-comparator comp elements))

  (define (list->bag elements)
    (list->bag-with-comparator comp elements))

) ; end of define-library
