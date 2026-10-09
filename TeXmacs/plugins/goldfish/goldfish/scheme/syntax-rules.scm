;; syntax-rules.scm -- R7RS hygienic macro system (syntax-rules et al.)
;;
;; The pattern matcher and template expander in this file
;; (syntax-rules-transformer, expand-pattern, expand-template,
;;  make-renamer, er-macro-transformer, any, every, find, length*,
;;  cons-source, close-syntax)
;; are derived from Chibi Scheme lib/init-7.scm (tag 0.12)
;;
;; SPDX-FileCopyrightText: 2009-2021 Alex Shinn
;;
;; SPDX-License-Identifier: BSD-3-Clause
;;
;; Copyright (c) 2026 The Goldfish Scheme Authors
;; Follow the same License as the original one

(define (any pred ls)
  (cond ((null? ls) #f)
        ((pred (car ls)) #t)
        (else (any pred (cdr ls)))
  ) ;cond
) ;define

(define (every pred ls)
  (cond ((null? ls) #t)
        ((pred (car ls)) (every pred (cdr ls)))
        (else #f)
  ) ;cond
) ;define

(define (find pred ls)
  (cond ((null? ls) #f)
        ((pred (car ls)) (car ls))
        (else (find pred (cdr ls)))
  ) ;cond
) ;define

(define (length* ls)
  (if (pair? ls) (+ 1 (length* (cdr ls))) 0)
) ;define

(define (%syntax-case-length ls)
  (if (pair? ls) (+ 1 (%syntax-case-length (cdr ls))) 0)
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

(define (cons-source kar kdr source)
  (cons kar kdr)
) ;define

(define (close-syntax form env)
  (make-syntactic-closure env '() form)
) ;define

(define %synclo-id (lambda (i) i))

(define (make-renamer mac-env)
  (let ((renames '()))
    (lambda (identifier)
      (let ((cell (assq identifier renames)))
        (if cell
          (cdr cell)
          (let ((id (close-syntax identifier mac-env)))
            (syntactic-closure-set-rename! id %synclo-id)
            (set! renames (cons (cons identifier id) renames))
            id
          ) ;let
        ) ;if
      ) ;let
    ) ;lambda
  ) ;let
) ;define

(define (er-macro-transformer f)
  (let ((cached-use-env #f) (cached-cmp #f))
    (lambda (expr use-env mac-env)
      (let ((cmp
              (if (eq? use-env cached-use-env)
                cached-cmp
                (let ((c (lambda (x y) (identifier=? use-env x use-env y))))
                  (set! cached-use-env use-env)
                  (set! cached-cmp c)
                  c
                ) ;let
              ) ;if
            ) ;cmp
           ) ;
        (f expr (make-renamer mac-env) cmp)
      ) ;let
    ) ;lambda
  ) ;let
) ;define

(define (syntax-rules-transformer expr rename compare)
  (let ((ellipsis-specified? (and (pair? (cdr expr)) (identifier? (cadr expr))))
        (count 0)
        (_er-macro-transformer (rename 'er-macro-transformer))
        (_lambda (rename 'lambda))
        (_let (rename 'let))
        (_begin (rename 'begin))
        (_if (rename 'if))
        (_and (rename 'and))
        (_or (rename 'or))
        (_eq? (rename 'eq?))
        (_equal? (rename 'equal?))
        (_car (rename 'car))
        (_cdr (rename 'cdr))
        (_cons (rename 'cons))
        (_pair? (rename 'pair?))
        (_null? (rename 'null?))
        (_expr (rename 'expr))
        (_rename (rename 'rename))
        (_compare (rename 'compare))
        (_quote (rename 'quote))
        (_apply (rename 'apply))
        (_append (rename 'append))
        (_map (rename 'map))
        (_vector? (rename 'vector?))
        (_list? (rename 'proper-list?))
        (_len (rename 'len))
        (_length (rename 'length*))
        (_- (rename '-))
        (_>= (rename '>=))
        (_error (rename 'error))
        (_ls (rename 'ls))
        (_res (rename 'res))
        (_i (rename 'i))
        (_reverse (rename 'reverse))
        (_vector->list (rename 'vector->list))
        (_list->vector (rename 'list->vector))
        (_cons3 (rename 'cons-source))
        (_underscore (rename '_))
       ) ;
    (define ellipsis (if ellipsis-specified? (cadr expr) (rename '...)))
    (define lits (if ellipsis-specified? (car (cddr expr)) (cadr expr)))
    (define forms (if ellipsis-specified? (cdr (cddr expr)) (cddr expr)))
    (define full-match? (any (lambda (x) (not (pair? (car x)))) forms))
    (define (next-symbol s)
      (set! count (+ count 1))
      (rename (string->symbol (string-append s (number->string count))))
    ) ;define
    (define (expand-pattern pat tmpl)
      (let lp
        ((p (if full-match? pat (cdr pat)))
         (x (if full-match? _expr (list _cdr _expr)))
         (dim 0)
         (vars '())
         (k (lambda (vars) (list _cons (expand-template tmpl vars) #f)))
        ) ;
        (let ((v (next-symbol "v.")))
          (list _let
            (list (list v x))
            (cond
             ((identifier? p)
              (cond ((ellipsis-mark? p) (error "bad ellipsis" p))
                    ((memq (identifier->symbol p) lits)
                     (list _and (list _compare v (list _rename (list _quote p))) (k vars))
                    ) ;
                    ((compare p _underscore) (k vars))
                    (else
                      (list _let (list (list p v)) (k (cons (cons p dim) vars)))
                    ) ;else
              ) ;cond
             ) ;
             ((ellipsis? p)
              (cond
               ((not (null? (cdr (cdr p))))
                (cond
                 ((any (lambda (x) (and (identifier? x) (ellipsis-mark? x))) (cddr p))
                  (error "multiple ellipses" p)
                 ) ;
                 (else
                   (let ((len (length* (cdr (cdr p)))) (_lp (next-symbol "lp.")))
                     `(,_let
                       ((,_len (,_length ,v)))
                       (,_and
                        (,_>= ,_len ,len)
                        (,_let
                         ,_lp
                         ((,_ls ,v)
                          (,_i (,_- ,_len ,len))
                          (,_res (,_quote ,())))
                         (,_if
                          (,_>= ,0 ,_i)
                          ,(lp `(,(cddr p) (,(car p) ,(car (cdr p))))
                             `(,_cons
                               ,_ls
                               (,_cons (,_reverse ,_res) (,_quote ,())))
                             dim
                             vars
                             k)
                          (,_lp
                           (,_cdr ,_ls)
                           (,_- ,_i ,1)
                           (,_cons (,_car ,_ls) ,_res))))))
                   ) ;let
                 ) ;else
                ) ;cond
               ) ;
               ((identifier? (car p))
                (list _and
                  (list _list? v)
                  (list _let (list (list (car p) v)) (k (cons (cons (car p) (+ 1 dim)) vars)))
                ) ;list
               ) ;
               (else
                 (let* ((w (next-symbol "w."))
                        (_lp (next-symbol "lp."))
                        (new-vars (all-vars (car p) (+ dim 1)))
                        (ls-vars
                          (map
                            (lambda (x)
                              (next-symbol
                                (string-append (symbol->string (identifier->symbol (car x))) "-ls")
                              ) ;next-symbol
                            ) ;lambda
                            new-vars
                          ) ;map
                        ) ;ls-vars
                        (once
                          (lp (car p)
                            (list _car w)
                            (+ dim 1)
                            '()
                            (lambda (_)
                              (cons _lp
                                (cons (list _cdr w)
                                  (map (lambda (x l) (list _cons (car x) l)) new-vars ls-vars)
                                ) ;cons
                              ) ;cons
                            ) ;lambda
                          ) ;lp
                        ) ;once
                       ) ;
                   (list _let
                     _lp
                     (cons (list w v) (map (lambda (x) (list x (list _quote '()))) ls-vars))
                     (list _if
                       (list _null? w)
                       (list _let
                         (map (lambda (x l) (list (car x) (list _reverse l))) new-vars ls-vars)
                         (k (append new-vars vars))
                       ) ;list
                       (list _and (list _pair? w) once)
                     ) ;list
                   ) ;list
                 ) ;let*
               ) ;else
              ) ;cond
             ) ;
             ((pair? p)
              (list _and
                (list _pair? v)
                (lp (car p)
                  (list _car v)
                  dim
                  vars
                  (lambda (vars) (lp (cdr p) (list _cdr v) dim vars k))
                ) ;lp
              ) ;list
             ) ;
             ((vector? p)
              (list _and
                (list _vector? v)
                (lp (vector->list p) (list _vector->list v) dim vars k)
              ) ;list
             ) ;
             ((null? p) (list _and (list _null? v) (k vars)))
             (else (list _and (list _equal? v p) (k vars)))
            ) ;cond
          ) ;list
        ) ;let
      ) ;let
    ) ;define
    (define ellipsis-mark?
      (if
        (if ellipsis-specified?
          (memq ellipsis lits)
          (any (lambda (x) (compare ellipsis x)) lits)
        ) ;if
        (lambda (x) #f)
        (if ellipsis-specified?
          (lambda (x) (eq? ellipsis x))
          (lambda (x) (compare ellipsis x))
        ) ;if
      ) ;if
    ) ;define
    (define (ellipsis-escape? x)
      (and (pair? x) (ellipsis-mark? (car x)))
    ) ;define
    (define (ellipsis? x)
      (and (pair? x) (pair? (cdr x)) (ellipsis-mark? (cadr x)))
    ) ;define
    (define (ellipsis-depth x)
      (if (ellipsis? x) (+ 1 (ellipsis-depth (cdr x))) 0)
    ) ;define
    (define (ellipsis-tail x)
      (if (ellipsis? x) (ellipsis-tail (cdr x)) (cdr x))
    ) ;define
    (define (all-vars x dim)
      (let lp
        ((x x) (dim dim) (vars '()))
        (cond
         ((identifier? x)
          (if (or (memq (identifier->symbol x) lits) (compare x _underscore))
            vars
            (cons (cons x dim) vars)
          ) ;if
         ) ;
         ((ellipsis? x) (lp (car x) (+ dim 1) (lp (cddr x) dim vars)))
         ((pair? x) (lp (car x) dim (lp (cdr x) dim vars)))
         ((vector? x) (lp (vector->list x) dim vars))
         (else vars)
        ) ;cond
      ) ;let
    ) ;define
    (define (free-vars x vars dim)
      (let lp
        ((x x) (free '()))
        (cond
         ((identifier? x)
          (if
            (and (not (memq x free))
              (cond
               ((assq x vars) => (lambda (cell) (>= (cdr cell) dim)))
               (else #f)
              ) ;cond
            ) ;and
            (cons x free)
            free
          ) ;if
         ) ;
         ((pair? x) (lp (car x) (lp (cdr x) free)))
         ((vector? x) (lp (vector->list x) free))
         (else free)
        ) ;cond
      ) ;let
    ) ;define
    (define (expand-template tmpl vars)
      (let lp
        ((t tmpl) (dim 0) (ell-esc #f))
        (cond
         ((identifier? t)
          (cond
           ((find (lambda (v) (eq? t (car v))) vars)
            =>
            (lambda (cell) (if (<= (cdr cell) dim) t (error "too few ...'s")))
           ) ;
           (else (list _rename (list _quote t)))
          ) ;cond
         ) ;
         ((pair? t)
          (cond
           ((and (ellipsis-escape? t) (not ell-esc))
            (lp
              (if (and (pair? (cdr t)) (null? (cddr t))) (cadr t) (cdr t))
              dim
              #t
            ) ;lp
           ) ;
           ((and (ellipsis? t) (not ell-esc))
            (let* ((depth (ellipsis-depth t))
                   (ell-dim (+ dim depth))
                   (ell-vars (free-vars (car t) vars ell-dim))
                  ) ;
              (cond ((null? ell-vars) (error "too many ...'s"))
                    ((and (null? (cdr (cdr t))) (identifier? (car t))) (lp (car t) ell-dim ell-esc))
                    (else
                      (let* ((once (lp (car t) ell-dim ell-esc))
                             (nest
                               (if (and (null? (cdr ell-vars)) (identifier? once) (eq? once (car ell-vars)))
                                 once
                                 (cons _map (cons (list _lambda ell-vars once) ell-vars))
                               ) ;if
                             ) ;nest
                             (many
                               (do ((d depth (- d 1)) (many nest (list _apply _append many)))
                                 ((= d 1) many)
                               ) ;do
                             ) ;many
                            ) ;
                        (if (null? (ellipsis-tail t))
                          many
                          (list _append many (lp (ellipsis-tail t) dim ell-esc))
                        ) ;if
                      ) ;let*
                    ) ;else
              ) ;cond
            ) ;let*
           ) ;
           (else (list _cons (lp (car t) dim ell-esc) (lp (cdr t) dim ell-esc)))
          ) ;cond
         ) ;
         ((vector? t) (list _list->vector (lp (vector->list t) dim ell-esc)))
         ((null? t) (list _quote '()))
         (else t)
        ) ;cond
      ) ;let
    ) ;define
    (list _er-macro-transformer
      (list _lambda
        (list _expr _rename _compare)
        (list _car
          (cons _or
            (append
              (map
                (lambda (clause)
                  (if (and (list? clause) (= (length clause) 2))
                    (expand-pattern (car clause) (cadr clause))
                    (error "invalid syntax-rules clause, which must be of the form (pattern template) (note fenders are not supported)"
                      clause
                    ) ;error
                  ) ;if
                ) ;lambda
                forms
              ) ;map
              (list
                (list _cons
                  (list _error "no expansion for" (list (rename 'strip-syntactic-closures) _expr))
                  #f
                ) ;list
              ) ;list
            ) ;append
          ) ;cons
        ) ;list
      ) ;list
    ) ;list
  ) ;let
) ;define

(define-macro (syntax-rules . args)
  (let* ((expr (cons 'syntax-rules args))
         (mac-env (curlet))
         (ren (make-renamer mac-env))
         (cmp (lambda (x y) (identifier=? mac-env x mac-env y)))
         (ast (syntax-rules-transformer expr ren cmp))
        ) ;
    (strip-syntactic-closures ast)
  ) ;let*
) ;define-macro

(define-bacro (%define-syntax-macro name t def-env)
  (let ((ar (car (arity (eval t def-env)))))
    (if (= ar 1)
      `(define-macro (,name . args)
         (resolve-syntactic-closures (,t (cons (quote ,name) args)) ,def-env))
      `(define-macro (,name . args)
         (resolve-syntactic-closures (,t
                                      (cons (quote ,name) args)
                                      (curlet)
                                      ,def-env)
           ,def-env))
    ) ;if
  ) ;let
) ;define-bacro

(define-bacro (define-syntax name
                transformer-spec
              ) ;define-syntax
  (let ((t (gensym "trans_")) (def-env (curlet)))
    `(begin
       (define ,t ,transformer-spec)
       (%define-syntax-macro ,name ,t ,def-env))
  ) ;let
) ;define-bacro

(define-macro (syntax-error message . args) (apply error message args))

(define-macro (let-syntax bindings . body)
  (let ((specs (map (lambda (b) (gensym "spec_")) bindings)))
    `(let ,()
       ,@(map (lambda (s b) `(define ,s ,(cadr b))) specs bindings)
       ,@(map (lambda (s b) `(define-syntax ,(car b) ,s)) specs bindings)
       (let ,() ,@body))
  ) ;let
) ;define-macro

(define-macro (letrec-syntax bindings . body)
  `(let ,()
     ,@(map (lambda (b) `(define-syntax ,(car b) ,(cadr b))) bindings)
     (let ,() ,@body))
) ;define-macro
