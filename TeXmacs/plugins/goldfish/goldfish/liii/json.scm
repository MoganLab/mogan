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

(define-library (liii json)
  (import (liii base)
          (liii list)
          (rename (guenchi json)
            (json-ref g:json-ref) (json-ref* g:json-ref*)
            (json-set g:json-set) (json-set* g:json-set*)
            (json-push g:json-push) (json-push* g:json-push*)
            (json-drop g:json-drop) (json-drop* g:json-drop*)
            (json-reduce g:json-reduce) (json-reduce* g:json-reduce*)))
  (export
    json-string-escape 
    string->json 
    json->string
    
    json-ref json-set json-push json-drop json-reduce

    json-null? json-object? json-array? json-string? json-float? json-number? json-integer? json-boolean?
    
    json-contains-key? 
    
    json-ref-string json-ref-number json-ref-integer json-ref-boolean json-get-or-else   
    
    json-keys)
  
  (begin

    ;;; ---------------------------------------------------------
    ;;; 0. 统一接口
    ;;; ---------------------------------------------------------

    (define (ensure-json-structure x)
      (unless (or (json-object? x) (json-array? x))
        (type-error "Value is not a JSON object or array" x)))

    (define (json-ref json key . args)
      (if (null? json) ; Allow () to pass through as "not found" to support safe navigation
          '()
          (begin
            (ensure-json-structure json)
            (let ((val (if (and (json-object? json) (equal? json '(())))
                           '()
                           (g:json-ref json key))))
              (if (null? args)
                  val
                  (apply json-ref (cons val args)))))))

    (define (json-set json key val . args)
      (ensure-json-structure json)
      (if (null? args)
          (if (and (json-object? json) (equal? json '(())))
              json
              (g:json-set json key val))
          (json-set json key 
                    (lambda (x)
                      (apply json-set (cons x (cons val args)))))))

    (define (json-push json key val . args)
      (ensure-json-structure json)
      (if (null? args)
          (if (and (json-object? json) (equal? json '(())))
              (g:json-push '() key val)
              (g:json-push json key val))
          (json-set json key
                    (lambda (x)
                      (apply json-push (cons x (cons val args)))))))

    (define (json-drop json key . args)
      (ensure-json-structure json)
      (if (null? args)
          (if (and (json-object? json) (equal? json '(())))
              json
              (g:json-drop json key))
          (json-set json key
                    (lambda (x)
                      (apply json-drop (cons x args))))))

    (define (json-reduce json key . args)
      (if (null? json)
          '()
          (begin
            (ensure-json-structure json)
            (if (null? args)
                (value-error "json-reduce: missing arguments")
                (if (null? (cdr args))
                    ;; Single level: (json-reduce json key proc)
                    (let ((proc (car args)))
                      (if (and (json-object? json) (equal? json '(())))
                          json
                          (g:json-reduce json key proc)))
                    ;; Multi level
                    (let* ((keys (cons key (drop-right args 1)))
                           (proc (last args))
                           (top-key (car keys))
                           (rest-keys (cdr keys)))
                      (json-reduce json top-key 
                                   (lambda (k v)
                                     (apply json-reduce (append (list v) rest-keys (list proc)))))))))))

    ;;; ---------------------------------------------------------
    ;;; 1. 类型谓词 
    ;;; ---------------------------------------------------------

    (define (json-null? x)
      (eq? x 'null))

    (define (json-object? x)
      (and (list? x)
           (not (null? x))
           (or (equal? x '(()))
               (every pair? x))))

    (define (json-array? x)
      (vector? x))

    (define (json-string? x)
      (string? x))

    (define (json-number? x)
      (number? x))
      
    (define (json-integer? x)
      (integer? x))

    (define (json-float? x)
      (float? x))

    (define (json-boolean? x)
      (boolean? x))

    ;;; ---------------------------------------------------------
    ;;; 2. 状态检查
    ;;; ---------------------------------------------------------

    (define (json-contains-key? json key)
      (if (not (json-object? json))
          #f
          (if (equal? json '(()))
              #f
              (if (assoc key json) #t #f))))

    ;;; ---------------------------------------------------------
    ;;; 3. 安全获取器
    ;;; ---------------------------------------------------------

    (define (json-get-or-else json default)
      (if (json-null? json)
          default
          json))

    (define (json-ref-string json key default)
      (let ((val (json-ref json key)))
        (if (string? val) val default)))

    (define (json-ref-number json key default)
      (let ((val (json-ref json key)))
        (if (number? val) val default)))
    
    (define (json-ref-integer json key default)
      (let ((val (json-ref json key)))
        (if (integer? val) val default)))

    (define (json-ref-boolean json key default)
      (let ((val (json-ref json key)))
        (if (boolean? val) val default)))

    ;;; ---------------------------------------------------------
    ;;; 4. 辅助工具
    ;;; ---------------------------------------------------------

    (define (json-keys json)
      (if (json-object? json)
          (if (equal? json '(()))
              '()
              (map car json))
          '()))

  ) ; end of begin
) ; end of define-library