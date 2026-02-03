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

(define-library (liii set)
  (import (rename (srfi srfi-113)
                  (set make-set-with-comparator)
                  (list->set list->set-with-comparator))
          (srfi srfi-128))
  (export set set-unfold list->set list->set! set-copy set->list list->set-with-comparator make-set-with-comparator
          set? set-contains? set-empty? set-disjoint?
          set-element-comparator set-size
          set=? set<? set>? set<=? set>=?
          set-any? set-every? set-find set-count set-member set-search! set-map
          set-for-each set-fold set-filter set-filter! set-remove set-remove!
          set-partition set-partition! set-union set-intersection set-difference set-xor
          set-union! set-intersection! set-difference! set-xor!
          set-adjoin set-adjoin! set-replace set-replace!
          set-delete set-delete! set-delete-all set-delete-all!)
  
  (define comp (make-default-comparator))

  (define (set . elements)
    (apply make-set-with-comparator comp elements))

  (define (list->set elements)
    (list->set-with-comparator comp elements))
  
) ; end of define-library
