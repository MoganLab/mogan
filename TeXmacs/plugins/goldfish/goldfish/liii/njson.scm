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

(define-library (liii njson)
  (import (liii base)
          (liii error)
          (liii path)
          (rename (liii json)
                  (string->json ljson-string->json)
                  (json->string ljson-json->string)
                  (json-object? ljson-object?)
                  (json-array? ljson-array?)
                  (json-ref ljson-ref)
          ) ;rename
  ) ;import
  (export njson?
          njson-null?
          njson-object?
          njson-array?
          njson-string?
          njson-number?
          njson-integer?
          njson-boolean?
          njson-size
          njson-empty?
          njson-free
          string->njson
          file->njson
          njson->string
          njson-format-string
          njson->file
          json->njson
          njson->json
          njson-object->alist
          njson-object->hash-table
          njson-array->list
          njson-array->vector
          let-njson
          njson-ref
          njson-set
          njson-append
          njson-set!
          njson-append!
          njson-merge
          njson-merge!
          njson-deep-merge
          njson-deep-merge!
          njson-drop
          njson-drop!
          njson-contains-key?
          njson-keys
          njson-schema-report
  ) ;export
  (begin
    (define (njson-null-symbol? x)
      (and (symbol? x) (symbol=? x 'null))
    ) ;define

    (define (njson-json-value? x)
      (or (njson? x) (string? x) (number? x) (boolean? x) (njson-null-symbol? x))
    ) ;define

    (define (ljson-json-value? x)
      (or (ljson-object? x) (ljson-array? x) (string? x) (number? x) (boolean? x) (njson-null-symbol? x))
    ) ;define

    (define njson-bridge-key "__njson_bridge")

    (define (njson? x)
      (g_njson-handle? x)
    ) ;define

    (define (njson-null? x)
      (g_njson-null? x)
    ) ;define

    (define (njson-object? x)
      (g_njson-object? x)
    ) ;define

    (define (njson-array? x)
      (g_njson-array? x)
    ) ;define

    (define (njson-string? x)
      (g_njson-string? x)
    ) ;define

    (define (njson-number? x)
      (g_njson-number? x)
    ) ;define

    (define (njson-integer? x)
      (g_njson-integer? x)
    ) ;define

    (define (njson-boolean? x)
      (g_njson-boolean? x)
    ) ;define

    (define (njson%%single-binding? x)
      (and (pair? x)
           (symbol? (car x))
           (pair? (cdr x))
           (null? (cddr x))
      ) ;and
    ) ;define

    (define (njson%%binding-list? xs)
      (and (pair? xs)
           (let loop ((rest xs))
             (and (pair? rest)
                  (njson%%single-binding? (car rest))
                  (or (null? (cdr rest))
                      (loop (cdr rest))
                  ) ;or
             ) ;and
           ) ;let
      ) ;and
    ) ;define

    (define (njson%%normalize-bindings binding)
      (cond
        ((njson%%single-binding? binding)
         (list binding)
        ) ;
        ((njson%%binding-list? binding)
         binding
        ) ;
        (else
         #f
        ) ;else
      ) ;cond
    ) ;define

    (define (njson%%expand-with-value-bindings bindings body)
      (if (null? bindings)
          `(begin ,@body)
          (let* ((binding (car bindings))
                 (var (car binding))
                 (value-expr (cadr binding))
                 (inner (njson%%expand-with-value-bindings (cdr bindings) body))
                 (released? (gensym "njson-released?")))
            `(let ((,var ,value-expr))
               (if (njson? ,var)
                   (let ((,released? #f))
                     (dynamic-wind
                       (lambda () #f)
                       (lambda () ,inner)
                       (lambda ()
                         (when (not ,released?)
                           (set! ,released? #t)
                           ;; Ignore type-error in finalizer so caller can safely free inside body.
                           (catch 'type-error
                             (lambda () (njson-free ,var))
                             (lambda args #f))))))
                   ,inner))
          ) ;let*
      ) ;if
    ) ;define

    (define-macro (let-njson binding . body)
      (let ((bindings (njson%%normalize-bindings binding)))
        (if bindings
            (njson%%expand-with-value-bindings bindings body)
            `(type-error "let-njson: expected (var value) or non-empty ((var value) ...)" ',binding)
        ) ;if
      ) ;let
    ) ;define-macro

    (define (njson-free x)
      (unless (njson? x)
        (type-error "njson-free: input must be njson-handle" x)
      ) ;unless
      (g_njson-free x)
    ) ;define

    (define (njson-size json)
      (unless (njson? json)
        (type-error "njson-size: json must be njson-handle" json)
      ) ;unless
      (g_njson-size json)
    ) ;define

    (define (njson-empty? json)
      (unless (njson? json)
        (type-error "njson-empty?: json must be njson-handle" json)
      ) ;unless
      (g_njson-empty? json)
    ) ;define

    (define (string->njson json-string)
      (unless (string? json-string)
        (type-error "string->njson: input must be string" json-string)
      ) ;unless
      (g_njson-string->json json-string)
    ) ;define

    (define (file->njson path)
      (unless (string? path)
        (type-error "file->njson: path must be string" path)
      ) ;unless
      (string->njson (path-read-text path))
    ) ;define

    (define (njson->string x)
      (unless (njson-json-value? x)
        (type-error "njson->string: input must be njson-handle or strict json scalar" x)
      ) ;unless
      (g_njson-json->string x)
    ) ;define

    (define (njson-format-string json-string . rest)
      (unless (string? json-string)
        (type-error "njson-format-string: input must be string" json-string)
      ) ;unless
      (cond
        ((null? rest)
         (g_njson-format-string json-string)
        ) ;
        ((and (pair? rest) (null? (cdr rest)))
         (let ((indent (car rest)))
           (unless (integer? indent)
             (type-error "njson-format-string: indent must be integer?" indent)
           ) ;unless
           (when (< indent 0)
             (value-error "njson-format-string: indent must be >= 0" indent)
           ) ;when
           (g_njson-format-string json-string indent)
         ) ;let
        ) ;
        (else
         (value-error "njson-format-string: expected (json-string [indent])" rest)
        ) ;else
      ) ;cond
    ) ;define

    (define (njson->file path x)
      (unless (string? path)
        (type-error "njson->file: path must be string" path)
      ) ;unless
      (unless (njson-json-value? x)
        (type-error "njson->file: input must be njson-handle or strict json scalar" x)
      ) ;unless
      (path-write-text path (njson-format-string (njson->string x)))
    ) ;define

    (define (json->njson x)
      (unless (ljson-json-value? x)
        (type-error "json->njson: input must be liii-json value or strict json scalar" x)
      ) ;unless
      (if (or (ljson-object? x) (ljson-array? x))
          (string->njson (ljson-json->string x))
          (string->njson (njson->string x))
      ) ;if
    ) ;define

    (define (njson->json x)
      (unless (njson-json-value? x)
        (type-error "njson->json: input must be njson-handle or strict json scalar" x)
      ) ;unless
      (let ((wrapped (ljson-string->json (string-append "{\"" njson-bridge-key "\":" (njson->string x) "}"))))
        (ljson-ref wrapped njson-bridge-key)
      ) ;let
    ) ;define

    (define (njson-object->alist json)
      (unless (njson-object? json)
        (type-error "njson-object->alist: json must be njson object-handle" json)
      ) ;unless
      (g_njson-object->alist json)
    ) ;define

    (define (njson-object->hash-table json)
      (unless (njson-object? json)
        (type-error "njson-object->hash-table: json must be njson object-handle" json)
      ) ;unless
      (g_njson-object->hash-table json)
    ) ;define

    (define (njson-array->list json)
      (unless (njson-array? json)
        (type-error "njson-array->list: json must be njson array-handle" json)
      ) ;unless
      (g_njson-array->list json)
    ) ;define

    (define (njson-array->vector json)
      (unless (njson-array? json)
        (type-error "njson-array->vector: json must be njson array-handle" json)
      ) ;unless
      (g_njson-array->vector json)
    ) ;define

    (define (njson-ref json key . keys)
      (unless (njson? json)
        (type-error "njson-ref: json must be njson-handle" json)
      ) ;unless
      (apply g_njson-ref (cons json (cons key keys)))
    ) ;define

    ;; Same calling style as (liii json):
    ;; (njson-set j key value)
    ;; (njson-set j k1 k2 ... kn value)
    (define (njson-set json key val . keys)
      (unless (njson? json)
        (type-error "njson-set: json must be njson-handle" json)
      ) ;unless
      (apply g_njson-set (cons json (cons key (cons val keys))))
    ) ;define

    ;; Append value to target array:
    ;; (njson-append j value)                   ; root must be array
    ;; (njson-append j k1 k2 ... kn value)      ; target path must be array
    (define (njson-append json . args)
      (unless (njson? json)
        (type-error "njson-append: json must be njson-handle" json)
      ) ;unless
      (when (null? args)
        (key-error "njson-append: expected (json [key ...] value)" json)
      ) ;when
      (apply g_njson-append (cons json args))
    ) ;define

    ;; In-place update style:
    ;; (njson-set! j key value)
    ;; (njson-set! j k1 k2 ... kn value)
    (define (njson-set! json key val . keys)
      (unless (njson? json)
        (type-error "njson-set!: json must be njson-handle" json)
      ) ;unless
      (apply g_njson-set! (cons json (cons key (cons val keys))))
    ) ;define

    ;; Append value to target array in place:
    ;; (njson-append! j value)                   ; root must be array
    ;; (njson-append! j k1 k2 ... kn value)      ; target path must be array
    (define (njson-append! json . args)
      (unless (njson? json)
        (type-error "njson-append!: json must be njson-handle" json)
      ) ;unless
      (when (null? args)
        (key-error "njson-append!: expected (json [key ...] value)" json)
      ) ;when
      (apply g_njson-append! (cons json args))
    ) ;define

    (define (njson%%check-merge api-name target-json source-json)
      (unless (njson-object? target-json)
        (type-error (string-append api-name ": target-json must be njson object-handle") target-json)
      ) ;unless
      (unless (njson-object? source-json)
        (type-error (string-append api-name ": source-json must be njson object-handle") source-json)
      ) ;unless
    ) ;define

    (define (njson-merge target-json source-json)
      (njson%%check-merge "njson-merge" target-json source-json)
      (g_njson-merge target-json source-json)
    ) ;define

    (define (njson-merge! target-json source-json)
      (njson%%check-merge "njson-merge!" target-json source-json)
      (g_njson-merge! target-json source-json)
    ) ;define

    (define (njson-deep-merge target-json source-json)
      (njson%%check-merge "njson-deep-merge" target-json source-json)
      (g_njson-deep-merge target-json source-json)
    ) ;define

    (define (njson-deep-merge! target-json source-json)
      (njson%%check-merge "njson-deep-merge!" target-json source-json)
      (g_njson-deep-merge! target-json source-json)
    ) ;define

    (define (njson-drop json key . keys)
      (unless (njson? json)
        (type-error "njson-drop: json must be njson-handle" json)
      ) ;unless
      (apply g_njson-drop (cons json (cons key keys)))
    ) ;define

    (define (njson-drop! json key . keys)
      (unless (njson? json)
        (type-error "njson-drop!: json must be njson-handle" json)
      ) ;unless
      (apply g_njson-drop! (cons json (cons key keys)))
    ) ;define

    (define (njson-contains-key? json key)
      (unless (njson? json)
        (type-error "njson-contains-key?: json must be njson-handle" json)
      ) ;unless
      (g_njson-contains-key? json key)
    ) ;define

    (define (njson-keys json)
      (unless (njson? json)
        (type-error "njson-keys: json must be njson-handle" json)
      ) ;unless
      (g_njson-keys json)
    ) ;define

    (define (njson-schema-report schema instance)
      (unless (njson? schema)
        (type-error "njson-schema-report: schema must be njson-handle" schema)
      ) ;unless
      (unless (njson-json-value? instance)
        (type-error "njson-schema-report: instance must be njson-handle or strict json scalar" instance)
      ) ;unless
      (g_njson-schema-report schema instance)
    ) ;define

  ) ;begin
) ;define-library
