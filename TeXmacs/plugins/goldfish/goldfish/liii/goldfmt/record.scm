;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.

(define-library (liii goldfmt record)
  (export make-env env? make-atom atom? env-tag-name env-depth env-indent
    env-children env-left-line env-right-line env-value atom-depth atom-indent
    atom-left-line atom-right-line atom-value make-raw-string-literal
    raw-string-literal? raw-string-literal-source raw-string-literal-value
    make-char-literal char-literal? char-literal-source char-literal-value
    make-dotted-tail dotted-tail? dotted-tail-form good-env? assert-env
  ) ;export
  (import (liii base) (liii error))
  (begin
    (define-record-type env
      (%make-env tag-name depth indent children left-line right-line value)
      env?
      (tag-name env-tag-name)
      (depth env-depth)
      (indent env-indent)
      (children env-children)
      (left-line env-left-line)
      (right-line env-right-line)
      (value env-value)
    ) ;define-record-type
    (define-record-type atom
      (%make-atom depth indent left-line right-line value)
      atom?
      (depth atom-depth)
      (indent atom-indent)
      (left-line atom-left-line)
      (right-line atom-right-line)
      (value atom-value)
    ) ;define-record-type
    (define* (make-atom (depth 0) (indent -1) (left-line 0) (right-line 0) (value #f))
      (when (not (integer? depth))
        (value-error "make-atom in liii/goldfmt-record: depth must be an integer")
      ) ;when
      (when (< depth 0)
        (value-error "make-atom in liii/goldfmt-record: depth must be non-negative")
      ) ;when
      (when (not (integer? indent))
        (value-error "make-atom in liii/goldfmt-record: indent must be an integer")
      ) ;when
      (when (< indent -1)
        (value-error "make-atom in liii/goldfmt-record: indent must be >= -1")
      ) ;when
      (when (not (integer? left-line))
        (value-error "make-atom in liii/goldfmt-record: left-line must be an integer")
      ) ;when
      (when (< left-line 0)
        (value-error "make-atom in liii/goldfmt-record: left-line must be non-negative")
      ) ;when
      (when (not (integer? right-line))
        (value-error "make-atom in liii/goldfmt-record: right-line must be an integer")
      ) ;when
      (when (< right-line 0)
        (value-error "make-atom in liii/goldfmt-record: right-line must be non-negative"
        ) ;value-error
      ) ;when
      (%make-atom depth indent left-line right-line value)
    ) ;define*
    (define-record-type raw-string-literal
      (%make-raw-string-literal source value)
      raw-string-literal?
      (source raw-string-literal-source)
      (value raw-string-literal-value)
    ) ;define-record-type
    (define* (make-raw-string-literal (source "") (value ""))
      (when (not (string? source))
        (value-error "make-raw-string-literal in liii/goldfmt-record: source must be a string"
        ) ;value-error
      ) ;when
      (when (not (string? value))
        (value-error "make-raw-string-literal in liii/goldfmt-record: value must be a string"
        ) ;value-error
      ) ;when
      (%make-raw-string-literal source value)
    ) ;define*
    (define-record-type char-literal
      (%make-char-literal source value)
      char-literal?
      (source char-literal-source)
      (value char-literal-value)
    ) ;define-record-type
    (define* (make-char-literal (source "") (value #\space))
      (when (not (string? source))
        (value-error "make-char-literal in liii/goldfmt-record: source must be a string"
        ) ;value-error
      ) ;when
      (when (not (char? value))
        (value-error "make-char-literal in liii/goldfmt-record: value must be a char")
      ) ;when
      (%make-char-literal source value)
    ) ;define*
    (define-record-type dotted-tail
      (%make-dotted-tail form)
      dotted-tail?
      (form dotted-tail-form)
    ) ;define-record-type
    (define* (make-dotted-tail (form '())) (%make-dotted-tail form))
    (define* (make-env (tag-name "")
               (depth 0)
               (indent -1)
               (children (vector))
               (left-line 0)
               (right-line 0)
               (value #f)
             ) ;make-env
      (let ((tag-name (if (symbol? tag-name) (symbol->string tag-name) tag-name))
            (children (if (list? children) (list->vector children) children))
           ) ;
        (when (not (or (string? tag-name) (eq? tag-name #f)))
          (value-error "make-env in liii/goldfmt record: tag-name must be a string or #f")
        ) ;when
        (when (not (integer? depth))
          (value-error "make-env in liii/goldfmt record: depth must be an integer")
        ) ;when
        (when (< depth 0)
          (value-error "make-env in liii/goldfmt record: depth must be non-negative")
        ) ;when
        (when (not (integer? indent))
          (value-error "make-env in liii/goldfmt record: indent must be an integer")
        ) ;when
        (when (< indent -1)
          (value-error "make-env in liii/goldfmt record: indent must be >= -1")
        ) ;when
        (when (not (or (vector? children) (eq? children #f)))
          (value-error "make-env in liii/goldfmt record: children must be a vector or #f")
        ) ;when
        (when (not (integer? left-line))
          (value-error "make-env in liii/goldfmt record: left-line must be an integer")
        ) ;when
        (when (< left-line 0)
          (value-error "make-env in liii/goldfmt record: left-line must be non-negative")
        ) ;when
        (when (not (integer? right-line))
          (value-error "make-env in liii/goldfmt record: right-line must be an integer")
        ) ;when
        (when (< right-line 0)
          (value-error "make-env in liii/goldfmt record: right-line must be non-negative")
        ) ;when
        (%make-env tag-name depth indent children left-line right-line value)
      ) ;let
    ) ;define*
    (define (env-valid? env parent-depth parent-indent)
      (and (= (env-depth env) (+ parent-depth 1))
        (if (= (env-depth env) 0)
          (or (= (env-indent env) 0) (= (env-indent env) -1))
          (>= (env-indent env) parent-indent)
        ) ;if
      ) ;and
    ) ;define
    (define (check-env env parent-depth parent-indent)
      (if (not (env-valid? env parent-depth parent-indent))
        #f
        (let ((children (env-children env))
              (my-depth (env-depth env))
              (my-indent (env-indent env))
             ) ;
          (let loop
            ((i 0))
            (if (>= i (vector-length children))
              #t
              (if (check-env (vector-ref children i) my-depth my-indent) (loop (+ i 1)) #f)
            ) ;if
          ) ;let
        ) ;let
      ) ;if
    ) ;define
    (define (good-env? env)
      (check-env env -1 0)
    ) ;define
    (define (assert-env env)
      (if (not (good-env? env)) (value-error "Invalid env structure") #t)
    ) ;define
  ) ;begin
) ;define-library
