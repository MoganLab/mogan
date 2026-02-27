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
          (liii path))
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
          njson-string->json
          njson-file->json
          njson-json->string
          njson-json->file
          let-njson
          njson-ref
          njson-set
          njson-set!
          njson-push
          njson-push!
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

    (define (njson-string->json json-string)
      (unless (string? json-string)
        (type-error "njson-string->json: input must be string" json-string))
      (g_njson-string->json json-string))

    (define (njson-file->json path)
      (unless (string? path)
        (type-error "njson-file->json: path must be string" path))
      (njson-string->json (path-read-text path)))

    (define (njson-json->string x)
      (unless (njson-json-value? x)
        (type-error "njson-json->string: input must be njson-handle or strict json scalar" x))
      (g_njson-json->string x))

    (define (njson-json->file path x)
      (unless (string? path)
        (type-error "njson-json->file: path must be string" path))
      (unless (njson-json-value? x)
        (type-error "njson-json->file: input must be njson-handle or strict json scalar" x))
      (path-write-text path (njson-json->string x)))

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

    ;; In-place update style:
    ;; (njson-set! j key value)
    ;; (njson-set! j k1 k2 ... kn value)
    (define (njson-set! json key val . keys)
      (unless (njson? json)
        (type-error "njson-set!: json must be njson-handle" json))
      (apply g_njson-set! (cons json (cons key (cons val keys)))))

    ;; Same calling style as (liii json):
    ;; (njson-push j key value)
    ;; (njson-push j k1 k2 ... kn value)
    (define (njson-push json key val . keys)
      (unless (njson? json)
        (type-error "njson-push: json must be njson-handle" json))
      (apply g_njson-push (cons json (cons key (cons val keys)))))

    ;; In-place update style:
    ;; (njson-push! j key value)
    ;; (njson-push! j k1 k2 ... kn value)
    (define (njson-push! json key val . keys)
      (unless (njson? json)
        (type-error "njson-push!: json must be njson-handle" json))
      (apply g_njson-push! (cons json (cons key (cons val keys)))))

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
