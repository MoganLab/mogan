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

(define-library (liii rich-set)
  (import (liii lang)(liii hash-table)(srfi srfi-128))
  (export rich-hash-set)
  (begin


    (define-case-class rich-hash-set ((data hash-table?))

      ;; Factory methods
      (chained-define (@empty) 
        (rich-hash-set (make-hash-table)))
                   
      ;; Basic operations
      (define (%size) (hash-table-size data))

      (define (%empty?) (hash-table-empty? data))

      (define (%contains element)
        (hash-table-contains? data element))

      ;; Modification operations
      (chained-define (%add-one! element)
        (hash-table-set! data element #t)
        (%this))

      (chained-define (%remove! element)
        (hash-table-delete! data element)
        (%this))

      (chained-define (%add-one element)
        (let ((ht (make-hash-table)))
          (hash-table-for-each
            (lambda (k v) (hash-table-set! ht k v))
            data)
          (hash-table-set! ht element #t)
          (rich-hash-set ht)))

      (chained-define (%remove element)
        (let ((ht (make-hash-table)))
          (hash-table-for-each
            (lambda (k v) (hash-table-set! ht k v))
            data)
          (hash-table-delete! ht element)
          (rich-hash-set ht)))

      (chained-define (%clear!)
        (hash-table-clear! data)
        (%this))

      ) ; end of define-case-class
    ) ; end of begin
  ) ; end of define-library

