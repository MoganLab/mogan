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

(define-library (liii hash-table)
  (import (srfi srfi-125)
          (srfi srfi-128))
  (export
    make-hash-table hash-table hash-table-unfold alist->hash-table
    hash-table? hash-table-contains? hash-table-empty? hash-table=?
    hash-table-mutable?
    hash-table-ref hash-table-ref/default
    hash-table-set! hash-table-delete! hash-table-intern! hash-table-update!
    hash-table-update!/default hash-table-pop! hash-table-clear!
    hash-table-size hash-table-keys hash-table-values hash-table-entries
    hash-table-find hash-table-count hash-table-fold
    hash-table-for-each hash-table-map->list
    hash-table->alist hash-table-copy
    )
  (begin
    ) ; end of begin
  ) ; end of library
