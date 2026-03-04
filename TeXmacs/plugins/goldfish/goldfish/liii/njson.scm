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
                  (json-ref ljson-ref)))
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
          njson-schema-report)
  (begin
    (define (njson-null-symbol? x)
      (and (symbol? x) (symbol=? x 'null)))

    (define (njson-json-value? x)
      (or (njson? x) (string? x) (number? x) (boolean? x) (njson-null-symbol? x)))

    (define (ljson-json-value? x)
      (or (ljson-object? x) (ljson-array? x) (string? x) (number? x) (boolean? x) (njson-null-symbol? x)))

    (define njson-bridge-key "__njson_bridge")

    (define (njson? x)
      (g_njson-handle? x))

    (define (njson-null? x)
      (g_njson-null? x))

    (define (njson-object? x)
      (g_njson-object? x))

    (define (njson-array? x)
      (g_njson-array? x))

    (define (njson-string? x)
      (g_njson-string? x))

    (define (njson-number? x)
      (g_njson-number? x))

    (define (njson-integer? x)
      (g_njson-integer? x))

    (define (njson-boolean? x)
      (g_njson-boolean? x))

    (define (njson%%single-binding? x)
      (and (pair? x)
           (symbol? (car x))
           (pair? (cdr x))
           (null? (cddr x))))

    (define (njson%%binding-list? xs)
      (and (pair? xs)
           (let loop ((rest xs))
             (and (pair? rest)
                  (njson%%single-binding? (car rest))
                  (or (null? (cdr rest))
                      (loop (cdr rest)))))))

    (define (njson%%normalize-bindings binding)
      (cond
        ((njson%%single-binding? binding)
         (list binding))
        ((njson%%binding-list? binding)
         binding)
        (else
         #f)))

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
                   ,inner)))))

    (define-macro (let-njson binding . body)
      (let ((bindings (njson%%normalize-bindings binding)))
        (if bindings
            (njson%%expand-with-value-bindings bindings body)
            `(type-error "let-njson: expected (var value) or non-empty ((var value) ...)" ',binding))))

    (define (njson-free x)
      (unless (njson? x)
        (type-error "njson-free: input must be njson-handle" x))
      (g_njson-free x))

    (define (njson-size json)
      (unless (njson? json)
        (type-error "njson-size: json must be njson-handle" json))
      (g_njson-size json))

    (define (njson-empty? json)
      (unless (njson? json)
        (type-error "njson-empty?: json must be njson-handle" json))
      (g_njson-empty? json))

    (define (string->njson json-string)
      (unless (string? json-string)
        (type-error "string->njson: input must be string" json-string))
      (g_njson-string->json json-string))

    (define (file->njson path)
      (unless (string? path)
        (type-error "file->njson: path must be string" path))
      (string->njson (path-read-text path)))

    (define (njson->string x)
      (unless (njson-json-value? x)
        (type-error "njson->string: input must be njson-handle or strict json scalar" x))
      (g_njson-json->string x))

    (define (njson-format-string json-string . rest)
      (unless (string? json-string)
        (type-error "njson-format-string: input must be string" json-string))
      (cond
        ((null? rest)
         (g_njson-format-string json-string))
        ((and (pair? rest) (null? (cdr rest)))
         (let ((indent (car rest)))
           (unless (integer? indent)
             (type-error "njson-format-string: indent must be integer?" indent))
           (when (< indent 0)
             (value-error "njson-format-string: indent must be >= 0" indent))
           (g_njson-format-string json-string indent)))
        (else
         (value-error "njson-format-string: expected (json-string [indent])" rest))))

    (define (njson->file path x)
      (unless (string? path)
        (type-error "njson->file: path must be string" path))
      (unless (njson-json-value? x)
        (type-error "njson->file: input must be njson-handle or strict json scalar" x))
      (path-write-text path (njson-format-string (njson->string x))))

    (define (json->njson x)
      (unless (ljson-json-value? x)
        (type-error "json->njson: input must be liii-json value or strict json scalar" x))
      (if (or (ljson-object? x) (ljson-array? x))
          (string->njson (ljson-json->string x))
          (string->njson (njson->string x))))

    (define (njson->json x)
      (unless (njson-json-value? x)
        (type-error "njson->json: input must be njson-handle or strict json scalar" x))
      (let ((wrapped (ljson-string->json (string-append "{\"" njson-bridge-key "\":" (njson->string x) "}"))))
        (ljson-ref wrapped njson-bridge-key)))

    (define (njson-ref json key . keys)
      (unless (njson? json)
        (type-error "njson-ref: json must be njson-handle" json))
      (apply g_njson-ref (cons json (cons key keys))))

    ;; Same calling style as (liii json):
    ;; (njson-set j key value)
    ;; (njson-set j k1 k2 ... kn value)
    (define (njson-set json key val . keys)
      (unless (njson? json)
        (type-error "njson-set: json must be njson-handle" json))
      (apply g_njson-set (cons json (cons key (cons val keys)))))

    ;; Append value to target array:
    ;; (njson-append j value)                   ; root must be array
    ;; (njson-append j k1 k2 ... kn value)      ; target path must be array
    (define (njson-append json . args)
      (unless (njson? json)
        (type-error "njson-append: json must be njson-handle" json))
      (when (null? args)
        (key-error "njson-append: expected (json [key ...] value)" json))
      (apply g_njson-append (cons json args)))

    ;; In-place update style:
    ;; (njson-set! j key value)
    ;; (njson-set! j k1 k2 ... kn value)
    (define (njson-set! json key val . keys)
      (unless (njson? json)
        (type-error "njson-set!: json must be njson-handle" json))
      (apply g_njson-set! (cons json (cons key (cons val keys)))))

    ;; Append value to target array in place:
    ;; (njson-append! j value)                   ; root must be array
    ;; (njson-append! j k1 k2 ... kn value)      ; target path must be array
    (define (njson-append! json . args)
      (unless (njson? json)
        (type-error "njson-append!: json must be njson-handle" json))
      (when (null? args)
        (key-error "njson-append!: expected (json [key ...] value)" json))
      (apply g_njson-append! (cons json args)))

    (define (njson%%check-merge api-name target-json source-json)
      (unless (njson-object? target-json)
        (type-error (string-append api-name ": target-json must be njson object-handle") target-json))
      (unless (njson-object? source-json)
        (type-error (string-append api-name ": source-json must be njson object-handle") source-json)))

    (define (njson-merge target-json source-json)
      (njson%%check-merge "njson-merge" target-json source-json)
      (g_njson-merge target-json source-json))

    (define (njson-merge! target-json source-json)
      (njson%%check-merge "njson-merge!" target-json source-json)
      (g_njson-merge! target-json source-json))

    (define (njson-deep-merge target-json source-json)
      (njson%%check-merge "njson-deep-merge" target-json source-json)
      (g_njson-deep-merge target-json source-json))

    (define (njson-deep-merge! target-json source-json)
      (njson%%check-merge "njson-deep-merge!" target-json source-json)
      (g_njson-deep-merge! target-json source-json))

    (define (njson-drop json key . keys)
      (unless (njson? json)
        (type-error "njson-drop: json must be njson-handle" json))
      (apply g_njson-drop (cons json (cons key keys))))

    (define (njson-drop! json key . keys)
      (unless (njson? json)
        (type-error "njson-drop!: json must be njson-handle" json))
      (apply g_njson-drop! (cons json (cons key keys))))

    (define (njson-contains-key? json key)
      (unless (njson? json)
        (type-error "njson-contains-key?: json must be njson-handle" json))
      (g_njson-contains-key? json key))

    (define (njson-keys json)
      (unless (njson? json)
        (type-error "njson-keys: json must be njson-handle" json))
      (g_njson-keys json))

    (define (njson-schema-report schema instance)
      (unless (njson? schema)
        (type-error "njson-schema-report: schema must be njson-handle" schema))
      (unless (njson-json-value? instance)
        (type-error "njson-schema-report: instance must be njson-handle or strict json scalar" instance))
      (g_njson-schema-report schema instance))

    ) ; end of begin
  ) ; end of define-library
