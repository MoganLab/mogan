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

(define-library (liii iset)
  (import (srfi srfi-217))
  (export
    ;; Constructors
    iset iset-unfold make-range-iset
    ;; Predicates
    iset? iset-contains? iset-empty? iset-disjoint?
    ;; Accessors
    iset-member iset-min iset-max
    ;; Updaters
    iset-adjoin iset-adjoin! iset-delete iset-delete!
    iset-delete-all iset-delete-all! iset-search iset-search!
    iset-delete-min iset-delete-min! iset-delete-max iset-delete-max!
    ;; The whole iset
    iset-size iset-find iset-count iset-any? iset-every?
    ;; Mapping and folding
    iset-map iset-for-each iset-fold iset-fold-right
    iset-filter iset-filter! iset-remove iset-remove!
    iset-partition iset-partition!
    ;; Copying and conversion
    iset-copy iset->list list->iset list->iset!
    ;; Subsets
    iset=? iset<? iset>? iset<=? iset>=?
    ;; Set theory operations
    iset-union iset-union! iset-intersection iset-intersection!
    iset-difference iset-difference! iset-xor iset-xor!
    ;; Intervals and ranges
    iset-open-interval iset-closed-interval
    iset-open-closed-interval iset-closed-open-interval
    isubset= isubset< isubset<= isubset> isubset>=
  )
) ;define-library
