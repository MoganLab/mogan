;; syntax-case.scm -- R6RS syntax-case procedural macro system
;;
;; The syntax-case system, pattern matcher, and template expander in this file
;; are derived from Chibi Scheme lib/chibi/syntax-case.scm (tag 0.12)
;; Written by Marc Nieper-Wißkirchen
;;
;; SPDX-FileCopyrightText: 2018-2021 Marc Nieper-Wißkirchen, Alex Shinn
;;
;; SPDX-License-Identifier: BSD-3-Clause
;;
;; Copyright (c) 2026 The Goldfish Scheme Authors
;; Follow the same License as the original one

(define-library (liii syntax-case)
  (export syntax-case syntax quasisyntax unsyntax unsyntax-splicing with-syntax
    datum->syntax syntax->datum free-identifier=? bound-identifier=? identifier?
    generate-temporaries syntax-violation with-ellipsis ellipsis-identifier?
    %syntax-case-split-at %syntax-case-fold-right %syntax-case-length
    %syntax-case-close-identifier %syntax-case-counter _ ...
  ) ;export
  (import (scheme base) (liii syntax) (srfi srfi-1))
  (begin

    (define (free-identifier=? x y)
      (identifier=? (curlet) x (curlet) y)
    ) ;define

    (define (bound-identifier=? x y)
      (cond ((and (identifier? x) (identifier? y))
             (eq? (identifier->symbol x) (identifier->symbol y))
            ) ;
            (else (eq? x y))
      ) ;cond
    ) ;define

    (define (syntax->datum stx)
      (strip-syntactic-closures stx)
    ) ;define

    (define (datum->syntax template-id datum)
      (let ((env (if (syntactic-closure? template-id)
                   (syntactic-closure-env template-id)
                   (curlet)
                 ) ;if
            ) ;env
            (renamer (if (syntactic-closure? template-id) (syntactic-closure-rename template-id) #f)
            ) ;renamer
           ) ;
        (let loop
          ((x datum))
          (cond ((symbol? x) (if renamer (renamer x) (make-syntactic-closure env '() x)))
                ((pair? x) (cons (loop (car x)) (loop (cdr x))))
                ((vector? x) (list->vector (map loop (vector->list x))))
                (else x)
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define *syntax-case-counter* 0)
    (define (%syntax-case-counter)
      *syntax-case-counter*
    ) ;define

    (define (%syntax-case-gen-sym prefix)
      (set! *syntax-case-counter* (+ *syntax-case-counter* 1))
      (string->symbol (string-append "%%sc-" prefix "-" (number->string *syntax-case-counter*))
      ) ;string->symbol
    ) ;define

    (define (generate-temporaries l)
      (map (lambda (x) (%syntax-case-gen-sym "t")) l)
    ) ;define

    (define (syntax-violation who message . form*)
      (let ((prefix (if who (string-append (object->string who) ": ") "")))
        (apply error 'syntax-error (string-append prefix message) form*)
      ) ;let
    ) ;define

    (define (ellipsis-identifier? id)
      (and (identifier? id) (eq? (identifier->symbol id) '...))
    ) ;define

    (define (wildcard-identifier? id)
      (and (identifier? id) (eq? (identifier->symbol id) '_))
    ) ;define

    (define (%syntax-case-length ls)
      (let loop
        ((ls ls) (len 0))
        (cond ((null? ls) len)
              ((pair? ls) (loop (cdr ls) (+ len 1)))
              (else len)
        ) ;cond
      ) ;let
    ) ;define

    (define (%syntax-case-close-identifier id env)
      (if (syntactic-closure? id)
        id
        (let ((sym (identifier->symbol id)))
          (let loop
            ((e env))
            (if (or (not (let? e)) (eq? e (rootlet)))
              sym
              (if (defined? sym e #t) (make-syntactic-closure e '() sym) (loop (outlet e)))
            ) ;if
          ) ;let
        ) ;let
      ) ;if
    ) ;define

    (define (build-cons-list elements tail)
      (let loop
        ((el elements))
        (if (null? el) tail `(cons ,(car el) ,(loop (cdr el))))
      ) ;let
    ) ;define

    (define (%syntax-case-split-at ls n)
      (let loop
        ((ls ls) (n n) (acc '()))
        (if (or (<= n 0) (not (pair? ls)))
          (cons (reverse acc) ls)
          (loop (cdr ls) (- n 1) (cons (car ls) acc))
        ) ;if
      ) ;let
    ) ;define

    (define (%syntax-case-fold-right f init . lists)
      (if (null? lists)
        init
        (if (null? (cdr lists))
          (let loop
            ((ls (car lists)))
            (if (null? ls) init (f (car ls) (loop (cdr ls))))
          ) ;let
          (let loop
            ((lsts lists))
            (if
              (let any-null
                ((l lsts))
                (cond ((null? l) #f)
                      ((null? (car l)) #t)
                      (else (any-null (cdr l)))
                ) ;cond
              ) ;let
              init
              (let ((cars (map car lsts)) (cdrs (map cdr lsts)))
                (apply f (append cars (list (loop cdrs))))
              ) ;let
            ) ;if
          ) ;let
        ) ;if
      ) ;if
    ) ;define

    (define (lookup-pvar id vars)
      (let ((sym (if (identifier? id) (identifier->symbol id) id)))
        (let loop
          ((v vars))
          (cond ((null? v) #f)
                ((let* ((entry (car v))
                        (entry-sym (if (identifier? (car entry)) (identifier->symbol (car entry)) (car entry))
                        ) ;entry-sym
                       ) ;
                   (eq? sym entry-sym)
                 ) ;let*
                 (cdar v)
                ) ;
                (else (loop (cdr v)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (update-envs id x level envs)
      (let loop
        ((level level) (envs envs))
        (cond ((zero? level) envs)
              ((null? envs)
               (error 'syntax-error "too few ellipses following syntax template" id)
              ) ;
              (else
                (let ((outer-envs (loop (- level 1) (cdr envs))))
                  (cond ((member x (car envs) eq?) envs)
                        (else (cons (cons x (car envs)) outer-envs))
                  ) ;cond
                ) ;let
              ) ;else
        ) ;cond
      ) ;let
    ) ;define

    (define (gen-matcher e lit* pattern vars)
      (cond
       ((pair? pattern)
        (cond
         ((and (pair? (cdr pattern))
            (identifier? (cadr pattern))
            (ellipsis-identifier? (cadr pattern))
          ) ;and
          (let* ((l (%syntax-case-length (cddr pattern)))
                 (h (%syntax-case-gen-sym "h"))
                 (t (%syntax-case-gen-sym "t"))
                 (s-pair (%syntax-case-gen-sym "split"))
                ) ;
            (let*-values (((head-matcher vars) (gen-map h lit* (car pattern) vars))
                          ((tail-matcher vars) (gen-matcher* t lit* (cddr pattern) vars))
                         ) ;
              (values
                (lambda (k)
                  `(let ((n (%syntax-case-length ,e)))
                     (if (and n (>= n ,l))
                       (let* ((,s-pair (%syntax-case-split-at ,e (- n ,l)))
                              (,h (car ,s-pair))
                              (,t (cdr ,s-pair)))
                         ,(head-matcher (lambda () (tail-matcher k))))
                       (fail)))
                ) ;lambda
                vars
              ) ;values
            ) ;let*-values
          ) ;let*
         ) ;
         (else
           (let ((e1 (%syntax-case-gen-sym "e1")) (e2 (%syntax-case-gen-sym "e2")))
             (let*-values (((car-matcher vars) (gen-matcher e1 lit* (car pattern) vars))
                           ((cdr-matcher vars) (gen-matcher e2 lit* (cdr pattern) vars))
                          ) ;
               (values
                 (lambda (k)
                   `(if (pair? ,e)
                      (let ((,e1 (car ,e)) (,e2 (cdr ,e)))
                        ,(car-matcher (lambda () (cdr-matcher k))))
                      (fail))
                 ) ;lambda
                 vars
               ) ;values
             ) ;let*-values
           ) ;let
         ) ;else
        ) ;cond
       ) ;
       ((identifier? pattern)
        (cond
         ((member pattern lit* free-identifier=?)
          (values
            (lambda (k) `(if (free-identifier=? (syntax ,pattern) ,e)
                           ,(k)
                           (fail)))
            vars
          ) ;values
         ) ;
         ((ellipsis-identifier? pattern)
          (error 'syntax-error "misplaced ellipsis in pattern" pattern)
         ) ;
         ((wildcard-identifier? pattern) (values (lambda (k) (k)) vars))
         (else (values (lambda (k) (k)) (cons (list pattern e 0) vars)))
        ) ;cond
       ) ;
       ((vector? pattern)
        (let ((e1 (%syntax-case-gen-sym "e1")))
          (let*-values (((matcher vars) (gen-matcher e1 lit* (vector->list pattern) vars)))
            (values
              (lambda (k)
                `(if (vector? ,e)
                   (let ((,e1 (vector->list ,e))) ,(matcher k))
                   (fail))
              ) ;lambda
              vars
            ) ;values
          ) ;let*-values
        ) ;let
       ) ;
       ((null? pattern) (values (lambda (k) `(if (null? ,e) ,(k) (fail))) vars))
       (else
         (values
           (lambda (k) `(if (equal? (syntax->datum ,e) (quote ,pattern))
                          ,(k)
                          (fail)))
           vars
         ) ;values
       ) ;else
      ) ;cond
    ) ;define

    (define (gen-map h lit* pattern vars)
      (let ((g (%syntax-case-gen-sym "g")))
        (let*-values (((matcher inner-vars) (gen-matcher g lit* pattern '())))
          (let ((loop (%syntax-case-gen-sym "loop"))
                (h-var (%syntax-case-gen-sym "h"))
                (g* (map (lambda (v) (%syntax-case-gen-sym "gacc")) inner-vars))
               ) ;
            (values
              (lambda (k)
                `(let ,loop
                   ((,h-var (reverse ,h))
                    ,@(map (lambda (g-acc) `(,g-acc '())) g*))
                   (if (null? ,h-var)
                     ,(k)
                     (let ((,g (car ,h-var)))
                       ,(matcher (lambda ()
                                   `(,loop
                                     (cdr ,h-var)
                                     ,@(map (lambda (var g-acc)
                                              `(cons ,(cadr var) ,g-acc))
                                         inner-vars
                                         g*)))))))
              ) ;lambda
              (let loop-fold
                ((iv inner-vars) (acc-vars vars) (g-list g*))
                (if (null? iv)
                  acc-vars
                  (let ((var (car iv)) (g-acc (car g-list)))
                    (loop-fold (cdr iv)
                      (cons (list (car var) g-acc (+ (caddr var) 1)) acc-vars)
                      (cdr g-list)
                    ) ;loop-fold
                  ) ;let
                ) ;if
              ) ;let
            ) ;values
          ) ;let
        ) ;let*-values
      ) ;let
    ) ;define

    (define (gen-matcher* e lit* pattern* vars)
      (let loop
        ((e e) (pattern* pattern*) (vars vars))
        (cond
         ((null? pattern*) (values (lambda (k) `(if (null? ,e) ,(k) (fail))) vars))
         ((pair? pattern*)
          (let ((e1 (%syntax-case-gen-sym "e1")) (e2 (%syntax-case-gen-sym "e2")))
            (let*-values (((car-matcher vars) (gen-matcher e1 lit* (car pattern*) vars))
                          ((cdr-matcher vars) (loop e2 (cdr pattern*) vars))
                         ) ;
              (values
                (lambda (k)
                  `(if (pair? ,e)
                     (let ((,e1 (car ,e)) (,e2 (cdr ,e)))
                       ,(car-matcher (lambda () (cdr-matcher k))))
                     (fail))
                ) ;lambda
                vars
              ) ;values
            ) ;let*-values
          ) ;let
         ) ;
         (else (gen-matcher e lit* pattern* vars))
        ) ;cond
      ) ;let
    ) ;define

    (define (gen-template tmpl envs ell? level vars)
      (cond
       ((pair? tmpl)
        (cond
         ((and (identifier? (car tmpl)) (eq? (identifier->symbol (car tmpl)) 'unsyntax))
          (if (and level (zero? level))
            (values (transform-output (cadr tmpl) vars ell?) envs)
            (let*-values (((out envs) (gen-template (cadr tmpl) envs ell? (and level (- level 1)) vars)))
              (values `(list 'unsyntax ,out) envs)
            ) ;let*-values
          ) ;if
         ) ;
         ((and (identifier? (car tmpl))
            (eq? (identifier->symbol (car tmpl)) 'quasisyntax)
          ) ;and
          (let*-values (((out envs) (gen-template (cadr tmpl) envs ell? (and level (+ level 1)) vars)))
            (values `(list 'quasisyntax ,out) envs)
          ) ;let*-values
         ) ;
         ((and (pair? (car tmpl))
            (identifier? (caar tmpl))
            (eq? (identifier->symbol (caar tmpl)) 'unsyntax)
          ) ;and
          (if (and level (zero? level))
            (let*-values (((out envs) (gen-template (cdr tmpl) envs ell? level vars)))
              (values
                (build-cons-list (map (lambda (e) (transform-output e vars ell?)) (cdar tmpl))
                  out
                ) ;build-cons-list
                envs
              ) ;values
            ) ;let*-values
            (let*-values (((out1 envs) (gen-template (cdar tmpl) envs ell? (and level (- level 1)) vars))
                          ((out2 envs) (gen-template (cdr tmpl) envs ell? level vars))
                         ) ;
              (values
                `(cons (cons 'unsyntax ,out1) ,out2)
                envs
              ) ;values
            ) ;let*-values
          ) ;if
         ) ;
         ((and (pair? (car tmpl))
            (identifier? (caar tmpl))
            (eq? (identifier->symbol (caar tmpl)) 'unsyntax-splicing)
          ) ;and
          (if (and level (zero? level))
            (let*-values (((out envs) (gen-template (cdr tmpl) envs ell? level vars)))
              (values
                `(append ,@(map (lambda (e) (transform-output e vars ell?))
                             (cdar tmpl))
                   ,out)
                envs
              ) ;values
            ) ;let*-values
            (let*-values (((out1 envs) (gen-template (cdar tmpl) envs ell? (and level (- level 1)) vars))
                          ((out2 envs) (gen-template (cdr tmpl) envs ell? level vars))
                         ) ;
              (values
                `(cons (cons 'unsyntax-splicing ,out1) ,out2)
                envs
              ) ;values
            ) ;let*-values
          ) ;if
         ) ;
         ((and (identifier? (car tmpl)) (ell? (car tmpl)))
          (gen-template (cadr tmpl) envs (lambda (id) #f) level vars)
         ) ;
         ((and (pair? (cdr tmpl)) (identifier? (cadr tmpl)) (ell? (cadr tmpl)))
          (let*-values (((out* envs) (gen-template (cddr tmpl) envs ell? level vars))
                        ((out envs) (gen-template (car tmpl) (cons '() envs) ell? level vars))
                       ) ;
            (if (null? (car envs))
              (error 'syntax-error "too many ellipses following syntax template" (car tmpl))
            ) ;if
            (let ((stx-sym (%syntax-case-gen-sym "stx")))
              (values
                `(%syntax-case-fold-right (lambda (,@(car envs) ,stx-sym)
                                            (cons ,out ,stx-sym))
                   ,out*
                   ,@(car envs))
                (cdr envs)
              ) ;values
            ) ;let
          ) ;let*-values
         ) ;
         (else
           (let*-values (((out1 envs) (gen-template (car tmpl) envs ell? level vars))
                         ((out2 envs) (gen-template (cdr tmpl) envs ell? level vars))
                        ) ;
             (values `(cons ,out1 ,out2) envs)
           ) ;let*-values
         ) ;else
        ) ;cond
       ) ;
       ((vector? tmpl)
        (let*-values (((out envs) (gen-template (vector->list tmpl) envs ell? level vars)))
          (values `(list->vector ,out) envs)
        ) ;let*-values
       ) ;
       ((identifier? tmpl)
        (cond ((ell? tmpl) (error 'syntax-error "misplaced ellipsis in syntax template" tmpl))
              ((lookup-pvar tmpl vars)
               =>
               (lambda (binding)
                 (let ((runtime-sym (car binding)) (depth (cadr binding)))
                   (values runtime-sym (update-envs tmpl runtime-sym depth envs))
                 ) ;let
               ) ;lambda
              ) ;
              (else
                (values `(%syntax-case-close-identifier (quote ,tmpl) (curlet)) envs)
              ) ;else
        ) ;cond
       ) ;
       (else (values `(quote ,tmpl) envs))
      ) ;cond
    ) ;define

    (define (transform-output expr vars ell?)
      (cond ((not (pair? expr)) expr)
            ((and (identifier? (car expr)) (eq? (identifier->symbol (car expr)) 'syntax))
             (let*-values (((out envs) (gen-template (cadr expr) '() ell? #f vars)))
               out
             ) ;let*-values
            ) ;
            ((and (identifier? (car expr))
               (eq? (identifier->symbol (car expr)) 'quasisyntax)
             ) ;and
             (let*-values (((out envs) (gen-template (cadr expr) '() ell? 0 vars)))
               out
             ) ;let*-values
            ) ;
            ((and (identifier? (car expr))
               (eq? (identifier->symbol (car expr)) 'with-syntax)
             ) ;and
             ;; (with-syntax (((p e) ...) body ...) -> (syntax-case (list e ...) () ((p ...) body ...))
             (let* ((bindings (cadr expr))
                    (body (cddr expr))
                    (patterns (map car bindings))
                    (exprs (map cadr bindings))
                    (transformed-exprs (map (lambda (e) (transform-output e vars ell?)) exprs))
                    (expanded-ws
                      `(syntax-case (list ,@transformed-exprs)
                         ,()
                         ((,@patterns) (let ,() ,@body)))
                    ) ;expanded-ws
                   ) ;
               (transform-output expanded-ws vars ell?)
             ) ;let*
            ) ;
            ((and (identifier? (car expr))
               (eq? (identifier->symbol (car expr)) 'syntax-case)
             ) ;and
             ;; 嵌套 syntax-case：递归处理其 sub-expr 与各 clause
             (let* ((sub-expr (transform-output (cadr expr) vars ell?))
                    (lits (caddr expr))
                    (clauses (cdddr expr))
                    (new-clauses
                      (map
                        (lambda (c)
                          (let* ((p (car c))
                                 (has-fender (= 3 (length c)))
                                 (fender (if has-fender (cadr c) #t))
                                 (b
                                   (if has-fender (caddr c) (if (= 2 (length c)) (cadr c) (cons 'begin (cdr c))))
                                 ) ;b
                                ) ;
                            (let*-values (((matcher inner-vars) (gen-matcher (%syntax-case-gen-sym "e") lits p '())))
                              ;; 内层 pattern vars 优先于外层
                              (let ((merged-vars (append inner-vars vars)))
                                (if has-fender
                                  (list p
                                    (transform-output fender merged-vars ell?)
                                    (transform-output b merged-vars ell?)
                                  ) ;list
                                  (list p (transform-output b merged-vars ell?))
                                ) ;if
                              ) ;let
                            ) ;let*-values
                          ) ;let*
                        ) ;lambda
                        clauses
                      ) ;map
                    ) ;new-clauses
                   ) ;
               `(syntax-case ,sub-expr ,lits ,@new-clauses)
             ) ;let*
            ) ;
            (else (cons (transform-output (car expr) vars ell?)
                    (transform-output (cdr expr) vars ell?)
                  ) ;cons
            ) ;else
      ) ;cond
    ) ;define

    (define (expand-clause c e-var lit* ell? next-fail)
      (let* ((pattern (car c))
             (has-fender (= 3 (length c)))
             (fender (if has-fender (cadr c) #t))
             (output-expr
               (if has-fender (caddr c) (if (= 2 (length c)) (cadr c) (cons 'begin (cdr c))))
             ) ;output-expr
            ) ;
        (let*-values (((matcher vars) (gen-matcher e-var lit* pattern '())))
          (let* ((transformed-output (transform-output output-expr vars ell?))
                 (transformed-fender (if (eq? fender #t) #t (transform-output fender vars ell?)))
                ) ;
            `(let ((fail (lambda ,() ,next-fail)))
               ,(matcher (lambda ()
                           (if (eq? transformed-fender #t)
                             transformed-output
                             `(if ,transformed-fender
                                ,transformed-output
                                (fail))))))
          ) ;let*
        ) ;let*-values
      ) ;let*
    ) ;define

    (define-macro (syntax-case expr lit* . clauses)
      (let ((e-var (%syntax-case-gen-sym "stx"))
            (ell? (lambda (id) (ellipsis-identifier? id)))
           ) ;
        (let loop
          ((cls (reverse clauses))
           (chain
             `(error 'syntax-error
                ,"syntax-case: no matching pattern"
                (syntax->datum ,e-var))
           ) ;chain
          ) ;
          (if (null? cls)
            `(let ((,e-var ,expr)) ,chain)
            (loop (cdr cls) (expand-clause (car cls) e-var lit* ell? chain))
          ) ;if
        ) ;let
      ) ;let
    ) ;define-macro

    (define-macro (syntax tmpl)
      (let*-values (((out envs)
                     (gen-template tmpl '() (lambda (id) (ellipsis-identifier? id)) #f '())
                    ) ;
                   ) ;
        out
      ) ;let*-values
    ) ;define-macro

    (define-macro (quasisyntax tmpl)
      (let*-values (((out envs)
                     (gen-template tmpl '() (lambda (id) (ellipsis-identifier? id)) 0 '())
                    ) ;
                   ) ;
        out
      ) ;let*-values
    ) ;define-macro

    (define-macro (unsyntax . args)
      (error 'syntax-error "unsyntax: misplaced outside quasisyntax")
    ) ;define-macro

    (define-macro (unsyntax-splicing . args)
      (error 'syntax-error "unsyntax-splicing: misplaced outside quasisyntax")
    ) ;define-macro

    (define-macro (with-syntax bindings . body)
      (let ((patterns (map car bindings)) (exprs (map cadr bindings)))
        `(syntax-case (list ,@exprs) ,() ((,@patterns) (let ,() ,@body)))
      ) ;let
    ) ;define-macro

    (define-macro (_ . args)
      (error 'syntax-error "invalid use of auxiliary keyword _")
    ) ;define-macro

    (define-macro (... . args)
      (error 'syntax-error "invalid use of auxiliary keyword ...")
    ) ;define-macro

    (define-macro (with-ellipsis ellipsis . body)
      (error 'unimplemented "with-ellipsis: not yet implemented")
    ) ;define-macro

  ) ;begin
) ;define-library
