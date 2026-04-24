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

(define-library (liii enum)
  (import (srfi srfi-209))
  (export
    ; Predicates
    enum-type? enum? enum-type-contains?
    enum=? enum<? enum>? enum<=? enum>=?
    ; Enum type constructor
    make-enum-type
    ; Enum accessors
    enum-type enum-name enum-ordinal enum-value
    ; Enum finders
    enum-name->enum enum-ordinal->enum
    enum-name->ordinal enum-name->value
    enum-ordinal->name enum-ordinal->value
    ; Enum type accessors
    enum-type-size enum-min enum-max
    enum-type-enums enum-type-names enum-type-values
    ; Enum operations
    enum-next enum-prev
    ; Comparators
    make-enum-comparator
    ; Enum set constructors
    enum-empty-set enum-type->enum-set enum-set list->enum-set
    enum-set-projection enum-set-copy
    make-enumeration enum-set-universe enum-set-constructor enum-set-indexer
    ; Enum set predicates
    enum-set? enum-set-contains? enum-set-member? enum-set-empty?
    enum-set-disjoint? enum-set=? enum-set<? enum-set>?
    enum-set<=? enum-set>=? enum-set-subset?
    enum-set-any? enum-set-every?
    ; Enum set accessors
    enum-set-type
    ; Enum set mutators
    enum-set-adjoin enum-set-adjoin!
    enum-set-delete enum-set-delete!
    enum-set-delete-all enum-set-delete-all!
    ; Enum set operations
    enum-set-size enum-set->enum-list enum-set->list enum-set-map->list
    enum-set-count enum-set-filter enum-set-filter!
    enum-set-remove enum-set-remove!
    enum-set-for-each enum-set-fold
    ; Enum set logical operations
    enum-set-union enum-set-union!
    enum-set-intersection enum-set-intersection!
    enum-set-difference enum-set-difference!
    enum-set-xor enum-set-xor!
    enum-set-complement enum-set-complement!
  ) ;export
  (begin
  ) ;begin
) ;define-library
