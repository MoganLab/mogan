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

(define-library (liii fxmapping)
  (import (srfi srfi-224))
  (export
    ;; Constructors
    fxmapping fxmapping-unfold fxmapping-accumulate
    alist->fxmapping alist->fxmapping/combinator
    ;; Predicates
    fxmapping? fxmapping-contains? fxmapping-empty? fxmapping-disjoint?
    ;; Accessors
    fxmapping-ref fxmapping-min fxmapping-max
    ;; Updaters
    fxmapping-adjoin fxmapping-adjoin/combinator
    fxmapping-set fxmapping-adjust fxmapping-delete fxmapping-delete-all
    fxmapping-update fxmapping-alter
    fxmapping-delete-min fxmapping-update-min fxmapping-pop-min
    fxmapping-delete-max fxmapping-update-max fxmapping-pop-max
    ;; Whole fxmapping
    fxmapping-size fxmapping-find fxmapping-count fxmapping-any? fxmapping-every?
    ;; Mapping and folding
    fxmapping-map fxmapping-for-each fxmapping-fold fxmapping-fold-right
    fxmapping-map->list fxmapping-filter fxmapping-remove fxmapping-partition
    ;; Conversion
    fxmapping->alist fxmapping->decreasing-alist fxmapping-keys fxmapping-values
    fxmapping->generator fxmapping->decreasing-generator
    ;; Comparison
    fxmapping=? fxmapping<? fxmapping>? fxmapping<=? fxmapping>=?
    ;; Set theory operations
    fxmapping-union fxmapping-intersection fxmapping-difference fxmapping-xor
    fxmapping-union/combinator fxmapping-intersection/combinator
    ;; Subsets
    fxsubmapping= fxmapping-open-interval fxmapping-closed-interval
    fxmapping-open-closed-interval fxmapping-closed-open-interval
    fxsubmapping< fxsubmapping<= fxsubmapping> fxsubmapping>=
    fxmapping-split
    ;; Relations
    fxmapping-relation-map
  ) ;export
) ;define-library
