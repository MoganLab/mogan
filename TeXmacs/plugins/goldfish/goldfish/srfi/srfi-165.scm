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

;; Based on Marc Nieper-WiÃŸkirchen MIT implementation

;; Copyright (C) Marc Nieper-WiÃŸkirchen (2019).  All Rights Reserved.

;; Permission is hereby granted, free of charge, to any person
;; obtaining a copy of this software and associated documentation
;; files (the "Software"), to deal in the Software without
;; restriction, including without limitation the rights to use, copy,
;; modify, merge, publish, distribute, sublicense, and/or sell copies
;; of the Software, and to permit persons to whom the Software is
;; furnished to do so, subject to the following conditions:

;; The above copyright notice and this permission notice (including
;; the next paragraph) shall be included in all copies or substantial
;; portions of the Software.

;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
;; BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
;; ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
;; CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;; SOFTWARE.

(define-library (srfi srfi-165)
  (import (srfi srfi-1)
          (srfi srfi-128)
          (srfi srfi-125))
  (export
     make-computation-environment-variable
     make-computation-environment
     computation-environment-ref
     computation-environment-update
     computation-environment-update!
     computation-environment-copy

     make-computation
     computation-run
     computation-ask
     computation-local

     computation-pure
     computation-each
     computation-each-in-list
     computation-bind
     computation-sequence
     computation-forked
     computation-bind/forked

     computation-fn
     computation-with
     computation-with!

     default-computation

     define-computation-type
     make-hash-table
     variable-comparator)
  (begin

    ;; Box 模拟（暂时无 SRFI-111）
    (define (box x) (cons x 'box))
    (define (unbox b) (car b))
    (define (set-box! b x) (set-car! b x))

    (define-record-type <computation-environment-variable>
      (make-environment-variable name default immutable? id)
      computation-environment-variable?
      (name environment-variable-name)
      (default environment-variable-default)
      (immutable? environment-variable-immutable?)
      (id environment-variable-id))

    (define make-computation-environment-variable
      (let ((count 0))
        (lambda (name default immutable?)
          (set! count (+ count 1))
          (make-environment-variable name default immutable? (- count)))))

    (define (computation-environment? obj)
      (and (vector? obj)
           (> (vector-length obj) 2)
           (hash-table? (vector-ref obj 0))
           (list? (vector-ref obj 1))))

    (define (predefined? var)
      (not (negative? (environment-variable-id var))))

    (define variable-comparator
      (make-comparator computation-environment-variable?
                       eq?
                       (lambda (x y)
                         (< (environment-variable-id x)
                            (environment-variable-id y)))
                       (lambda (x . y)
                         (environment-variable-id x))))

    ;; Alist 替代 mapping
    (define (local-ref alist var default-thunk success)
      (let ((pair (assq var alist)))
        (if pair
            (success (cdr pair))
            (default-thunk))))

    (define (local-set alist var box)
      (cons (cons var box) alist))

    (define (local-for-each proc alist)
      (for-each (lambda (p) (proc (car p) (cdr p))) alist))

    (define (environment-global env)
      (vector-ref env 0))

    (define (environment-local env)
      (vector-ref env 1))

    (define (environment-set-global! env global)
      (vector-set! env 0 global))

    (define (environment-set-local! env local)
      (vector-set! env 1 local))

    (define (environment-cell-set! env var box)
      (vector-set! env (+ 2 (environment-variable-id var)) box))

    (define (environment-cell env var)
      (vector-ref env (+ 2 (environment-variable-id var))))

    (define default-computation
      (make-computation-environment-variable 'default-computation #f #f))

    (define-macro (define-computation-type make-environment run . vars)
      (letrec ((process-vars 
                (lambda (vars n acc)
                  (if (null? vars)
                      (reverse acc)
                      (let ((v (car vars))
                            (rest (cdr vars)))
                        (cond
                         ((and (pair? v) (pair? (cdr v)) (pair? (cddr v))
                               (string=? (caddr v) "immutable"))
                          (let ((var (car v))
                                (default (cadr v)))
                            (process-vars rest (+ n 1)
                                          (cons (list var default #t n) acc))))
                         ((and (pair? v) (pair? (cdr v)))
                          (let ((var (car v))
                                (default (cadr v)))
                            (process-vars rest (+ n 1)
                                          (cons (list var default #f n) acc))))
                         (else
                          (process-vars rest (+ n 1)
                                        (cons (list v #f #f n) acc)))))))))
        (let* ((processed (process-vars vars 0 '()))
               (n (length processed))
               (default-syms (map (lambda (x) (gensym "default")) processed))
               (env-sym (gensym "env")))
          `(begin
             ,@(map (lambda (p ds) `(define ,ds ,(cadr p)))
                    processed default-syms)
             ,@(map (lambda (p ds)
                      `(define ,(car p)
                         (,make-environment-variable ',(car p) ,ds ,(caddr p) ,(cadddr p))))
                    processed default-syms)
             (define (,make-environment)
               (let ((,env-sym (make-vector ,(+ n 2))))
                 (,environment-set-global! ,env-sym (make-hash-table variable-comparator))
                 (,environment-set-local! ,env-sym '())
                 ,@(map (lambda (p ds)
                          `(vector-set! ,env-sym ,(+ (cadddr p) 2) (,box ,ds)))
                        processed default-syms)
                 ,env-sym))
             (define (,run computation)
               (,execute computation (,make-environment)))))))

    (define (computation-environment-ref env var)
      (if (predefined? var)
          (unbox (environment-cell env var))
          (local-ref
           (environment-local env)
           var
           (lambda ()
             (hash-table-ref/default (environment-global env)
                                     var
                                     (environment-variable-default var)))
           unbox)))

    (define (computation-environment-update env . arg*)
      (let ((new-env (vector-copy env)))
        (let loop ((arg* arg*)
                   (local (environment-local env)))
          (if (null? arg*)
              (begin
                (environment-set-local! new-env local)
                new-env)
              (let ((var (car arg*))
                    (val (cadr arg*)))
                (if (predefined? var)
                    (begin
                      (environment-cell-set! new-env var (box val))
                      (loop (cddr arg*) local))
                    (loop (cddr arg*) (local-set local var (box val)))))))))

    ;; TODO: check immutable?
    (define (computation-environment-update! env var val)
      (if (predefined? var)
          (set-box! (environment-cell env var) val)
          (local-ref (environment-local env)
                     var
                     (lambda ()
                       (hash-table-set! (environment-global env) var val))
                     (lambda (cell)
                       (set-box! cell val)))))

    (define (computation-environment-copy env)
      (let ((global (hash-table-copy (environment-global env) #t)))
        (local-for-each (lambda (var cell)
                          (hash-table-set! global var (unbox cell)))
                        (environment-local env))
        (let ((new-env (make-vector (vector-length env))))
          (environment-set-global! new-env global)
          (environment-set-local! new-env '())
          (do ((i (- (vector-length env) 1) (- i 1)))
              ((< i 2) new-env)
            (vector-set! new-env i (box (unbox (vector-ref env i))))))))

    (define (execute computation env)
      (let ((coerce (if (procedure? computation)
                        values
                        (or (computation-environment-ref env default-computation)
                            (error "not a computation" computation)))))
        ((coerce computation) env)))

    (define (make-computation proc)
      (lambda (env)
        (proc (lambda (c) (execute c env)))))

    (define (computation-pure . args)
      (make-computation
       (lambda (compute)
         (apply values args))))

    (define (computation-each a . a*)
      (computation-each-in-list (cons a a*)))

    (define (computation-each-in-list a*)
      (make-computation
       (lambda (compute)
         (let loop ((a (car a*)) (a* (cdr a*)))
           (if (null? a*)
               (compute a)
               (begin
                 (compute a)
                 (loop (car a*) (cdr a*))))))))

    (define (computation-bind a . f*)
      (make-computation
       (lambda (compute)
         (let loop ((a a) (f* f*))
           (if (null? f*)
               (compute a)
               (loop (call-with-values
                         (lambda () (compute a))
                       (car f*))
                     (cdr f*)))))))

    (define (computation-ask)
      (lambda (env)
        env))

    (define (computation-local updater computation)
      (lambda (env)
        (computation (updater env))))

    (define-macro (computation-fn . args)
      (let ((clauses (car args))
            (body (cdr args)))
        (define (parse-clauses clauses)
          (map (lambda (c)
                 (if (pair? c)
                     (let ((id (car c))
                           (var (cadr c)))
                       (list id var (gensym "tmp")))
                     (let ((id c))
                       (list id id (gensym "tmp")))))
               clauses))
        (let* ((parsed (parse-clauses clauses))
               (env-sym (gensym "env"))
               (ids (map car parsed))
               (vars (map cadr parsed))
               (tmps (map caddr parsed)))
          `(let ,(map list tmps vars)
             (computation-bind
              (computation-ask)
              (lambda (,env-sym)
                (let ,(map (lambda (id tmp)
                             `(,id (computation-environment-ref ,env-sym ,tmp)))
                           ids tmps)
                  ,@body)))))))

    (define-macro (computation-with . args)
      (let ((bindings (car args))
            (comps (cdr args)))
        (let ((var-tmps (map (lambda (b) (gensym "var")) bindings))
              (val-tmps (map (lambda (b) (gensym "val")) bindings))
              (comp-tmps (map (lambda (c) (gensym "comp")) comps)))
          `(let ,(append (map (lambda (b vt) `(,vt ,(car b))) bindings var-tmps)
                         (map (lambda (b vt) `(,vt ,(cadr b))) bindings val-tmps)
                         (map (lambda (c ct) `(,ct ,c)) comps comp-tmps))
             (computation-local
              (lambda (env)
                (computation-environment-update env 
                  ,@(apply append (map list var-tmps val-tmps))))
              (computation-each ,@comp-tmps))))))

    (define-macro (computation-with! . bindings)
      (let ((var-tmps (map (lambda (b) (gensym "var")) bindings))
            (val-tmps (map (lambda (b) (gensym "val")) bindings))
            (env-sym (gensym "env")))
        `(let ,(append (map (lambda (b vt) `(,vt ,(car b))) bindings var-tmps)
                       (map (lambda (b vt) `(,vt ,(cadr b))) bindings val-tmps))
           (computation-bind
            (computation-ask)
            (lambda (,env-sym)
              ,@(map (lambda (vt val-t)
                       `(computation-environment-update! ,env-sym ,vt ,val-t))
                     var-tmps val-tmps)
              (computation-pure (if #f #f)))))))

    (define (computation-forked a . a*)
      (make-computation
       (lambda (compute)
         (let loop ((a a) (a* a*))
           (if (null? a*)
               (compute a)
               (begin
                 (compute (computation-local
                           (lambda (env)
                             (computation-environment-copy env))
                           a))
                 (loop (car a*) (cdr a*))))))))

    (define (computation-bind/forked computation . proc*)
      (apply computation-bind
             (computation-local computation-environment-copy computation)
             proc*))

    (define (computation-sequence fmt*)
      (fold-right
       (lambda (fmt res)
         (computation-bind
          res
          (lambda (vals)
            (computation-bind
             fmt
             (lambda (val)
               (computation-pure (cons val vals)))))))
       (computation-pure '()) fmt*))

    (define-computation-type make-computation-environment computation-run)))

