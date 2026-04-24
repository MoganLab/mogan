;
; Copyright (C) 2024 The Goldfish Scheme Authors
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

(define-library (liii queue)
  (export
    ; Constructors
    make-list-queue list-queue list-queue-copy
    list-queue-unfold list-queue-unfold-right
    ; Predicates
    list-queue? list-queue-empty?
    ; Accessors
    list-queue-front list-queue-back list-queue-list list-queue-first-last
    ; Mutators
    list-queue-add-front! list-queue-add-back!
    list-queue-remove-front! list-queue-remove-back!
    list-queue-remove-all! list-queue-set-list!
    ; Whole queue
    list-queue-append list-queue-append! list-queue-concatenate
    ; Mapping
    list-queue-map list-queue-map! list-queue-for-each
  ) ;export
  (import (srfi srfi-117))
) ;define-library
