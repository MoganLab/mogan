
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : macro-search.scm
;; DESCRIPTION : searching properties of macros
;; COPYRIGHT   : (C) 2013  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (source macro-search) (:use (utils edit variants)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Routines for subsequent customization
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define tag-options-table (make-ahash-table))

(define tag-parameters-table (make-ahash-table))

(tm-define (standard-options l) #f)

(tm-define (standard-parameters l) #f)

(tm-define (tree-parameter? t) (tree-label-parameter? (tree-label t)))

(tm-define (parameter-choice-list l) #f)

(tm-define inhibit-local-table (make-ahash-table))
(tm-define inhibit-global-table (make-ahash-table))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Collecting style options for a tag
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (collect-options-sub def t)
  ;; (display* "Collect sub " def "\n")
  (cond ((tree-atomic? def) (list))
        ((and (tree-is? def 'compound) (tree-atomic? (tree-ref def 0)))
         (let* ((c (tree-children def))
                (l (string->symbol (tree->string (car c))))
                (u (tm->tree (cons l (cdr c))))
               ) ;
           (collect-options-sub u t)
         ) ;let*
        ) ;
        (else (let* ((head (collect-options (symbol->string (tree-label def)) t))
                     (tail (map (cut collect-options-sub <> t) (tree-children def)))
                    ) ;
                (list-remove-duplicates (apply append (cons head tail)))
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (append-options l1 l2)
  (cond ((not l1) l2)
        ((and (pair? l1) (== (car l1) :recurse)) (list-union (cdr l1) l2))
        (else l1)
  ) ;cond
) ;define

(define (collect-options l t)
  (when (not (ahash-ref t l))
    ;; (display* "collect-options " l "\n")
    (ahash-set! t l '())
    (ahash-set! t
      l
      (with std
        (standard-options (string->symbol l))
        (append-options std
          (with def
            (get-init-tree l)
            (if (tree-in? def '(macro xmacro)) (collect-options-sub def t) (list))
          ) ;with
        ) ;append-options
      ) ;with
    ) ;ahash-set!
  ) ;when
  (ahash-ref t l)
) ;define

(tm-define (search-options l)
  (with label
    (if (symbol? l) (symbol->string l) l)
    (collect-options label tag-options-table)
    (ahash-ref tag-options-table label)
  ) ;with
) ;tm-define

(tm-define (search-tag-options t) (search-options (tree-label t)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Determine which parameters are set by a tag
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (assigned-parameters-with l v t)
  (if (null? (cdr l))
    (assigned-parameters-sub (car l) v t)
    (let* ((v* (make-ahash-table)) (t* (make-ahash-table)))
      (collect-parameters-sub (car l) v* t*)
      (collect-parameters-sub (cadr l) v* t*)
      (assigned-parameters-with (cddr l) v t)
      (when (tree-atomic? (car l))
        (ahash-set! v (tree->string (car l)) #t)
      ) ;when
      (for (x (ahash-set->list v*)) (ahash-remove! v x))
    ) ;let*
  ) ;if
) ;define

(define (assigned-parameters-sub def v t)
  (cond ((tree-atomic? def) (noop))
        ((tree-is? def 'with) (assigned-parameters-with (tree-children def) v t))
        ((and (tree-is? def 'compound) (tree-atomic? (tree-ref def 0)))
         (let* ((c (tree-children def))
                (l (string->symbol (tree->string (car c))))
                (u (tm->tree (cons l (cdr c))))
               ) ;
           (assigned-parameters-sub u v t)
         ) ;let*
        ) ;
        ((== (tree-arity def) 1)
         (assigned-parameters (symbol->string (tree-label def)) v t)
        ) ;
  ) ;cond
) ;define

(define (assigned-parameters l v t)
  (when (not (ahash-ref t l))
    ;; (display* "Assigned by " l "\n")
    (ahash-set! t l #t)
    (with def
      (get-init-tree l)
      ;; (display* "  Def= " def "\n")
      (cond ((tree-is? def 'uninit) (noop))
            ((tree-in? def '(macro xmacro))
             (assigned-parameters-sub (cAr (tree-children def)) v t)
            ) ;
      ) ;cond
    ) ;with
  ) ;when
) ;define

(tm-define (assigned-parameters l)
  (if (symbol? l) (set! l (symbol->string l)))
  (let* ((v (make-ahash-table)) (t (make-ahash-table)))
    (assigned-parameters l v t)
    (sort (ahash-set->list v) string<=?)
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Collecting environment variables which are parameters for a tag
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (get-with-vars l)
  (cond ((or (null? l) (null? (cdr l))) '())
        ((tree-atomic? (car l)) (cons (tree->string (car l)) (get-with-vars (cddr l))))
        (else (get-with-vars (cddr l)))
  ) ;cond
) ;define

(define (collect-parameters-sub def v t)
  ;; (display* "Collect sub " def "\n")
  (cond ((tree-atomic? def) (noop))
        ((and (tree-in? def '(value quote-value)) (tree-atomic? (tree-ref def 0)))
         (ahash-set! v (tree->string (tree-ref def 0)) #t)
        ) ;
        ((tree-is? def 'with)
         (let* ((v* (make-ahash-table))
                (t* (make-ahash-table))
                (vs (get-with-vars (cDr (tree-children def))))
               ) ;
           (for-each (cut collect-parameters-sub <> v* t) (tree-children def))
           (for (x vs) (ahash-remove! v* x))
           (for (x (ahash-set->list v*)) (ahash-set! v x #t))
         ) ;let*
        ) ;
        ((and (tree-is? def 'compound) (tree-atomic? (tree-ref def 0)))
         (let* ((c (tree-children def))
                (l (string->symbol (tree->string (car c))))
                (u (tm->tree (cons l (cdr c))))
               ) ;
           (collect-parameters-sub u v t)
         ) ;let*
        ) ;
        ((tree-label-extension? (tree-label def))
         (when (tree-parameter? def)
           (ahash-set! v (symbol->string (tree-label def)) #t)
         ) ;when
         (let* ((v* (make-ahash-table)) (t* (make-ahash-table)))
           (assigned-parameters (symbol->string (tree-label def)) v* t*)
           (with al
             (filter (lambda (x) (not (ahash-ref v x))) (ahash-set->list v*))
             (collect-parameters (symbol->string (tree-label def)) v t)
             (for-each (cut collect-parameters-sub <> v t) (tree-children def))
             (for (x al) (ahash-remove! v x))
           ) ;with
         ) ;let*
        ) ;
        (else (when (tree-parameter? def)
                (ahash-set! v (symbol->string (tree-label def)) #t)
              ) ;when
          (collect-parameters (symbol->string (tree-label def)) v t)
          (for-each (cut collect-parameters-sub <> v t) (tree-children def))
        ) ;else
  ) ;cond
) ;define

(define (collect-parameters l v t)
  (when (not (ahash-ref t l))
    ;; (display* "collect-parameters " l "\n")
    (ahash-set! t l #t)
    (with std
      (standard-parameters l)
      (if std
        (begin
          ;; (display* "Std= " std "\n")
          (for (x std) (ahash-set! v x #t))
        ) ;begin
        (with def
          (get-init-tree l)
          ;; (display* "  Def= " def "\n")
          (cond ((tree-is? def 'uninit) (noop))
                ((tree-in? def '(macro xmacro)) (collect-parameters-sub def v t))
                (else (ahash-set! v l #t))
          ) ;cond
        ) ;with
      ) ;if
    ) ;with
  ) ;when
) ;define

(tm-define (search-parameters l)
  (let* ((label (if (symbol? l) (symbol->string l) l)) (v (make-ahash-table)))
    (collect-parameters label v tag-parameters-table)
    (sort (ahash-set->list v) string<=?)
  ) ;let*
) ;tm-define

(tm-define (search-tag-parameters t) (search-parameters (tree-label t)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Theme analysis
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (theme-guess var)
  (with def
    (get-init-tree var)
    (and (tree-is? def 'macro)
      (== (tree-arity def) 2)
      (tree-is? (tree-ref def 1) 'with)
      (with l
        (cDr (tm-children (tree->stree (tm-ref def 1))))
        (and (>= (length l) 2)
          (let* ((var (car l)) (val (cadr l)))
            (and (string? var)
              (tm-is? val 'value)
              (== (tm-arity val) 1)
              (string? (tm-ref val 0))
              (string-ends? (tm-ref val 0) (string-append "-" var))
              (string-drop-right (tm-ref val 0) (+ (string-length var) 1))
            ) ;and
          ) ;let*
        ) ;and
      ) ;with
    ) ;and
  ) ;with
) ;tm-define

(define (theme-read-members th l)
  (if (or (null? l) (null? (cdr l)))
    l
    (and-with r
      (theme-read-members th (cddr l))
      (let* ((var (car l)) (val (cadr l)))
        (and (string? var)
          (tm-is? val 'value)
          (== (tm-arity val) 1)
          (== (tm-ref val 0) (string-append th "-" var))
          (cons var r)
        ) ;and
      ) ;let*
    ) ;and-with
  ) ;if
) ;define

(tm-define (theme->members th)
  ;; FIXME: does not handle subthemes yet
  (with def
    (get-init-tree (string-append "with-" th))
    (and (tree-is? def 'macro)
      (== (tree-arity def) 2)
      (tree-is? (tree-ref def 1) 'with)
      (with l
        (cDr (tm-children (tree->stree (tm-ref def 1))))
        (theme-read-members th l)
      ) ;with
    ) ;and
  ) ;with
) ;tm-define

(define (member->theme-at var at)
  (with pos
    (string-search-backwards "-" at var)
    (and (>= pos 0)
      (or (and-with mems
            (theme->members (substring var 0 pos))
            (and (in? (substring var (+ pos 1) (string-length var)) mems)
              (substring var 0 pos)
            ) ;and
          ) ;and-with
        (member->theme-at var (- pos 1))
      ) ;or
    ) ;and
  ) ;with
) ;define

(tm-define (member->theme var) (member->theme-at var (- (string-length var) 1)))

(tm-define (search-themes l)
  (let* ((l1 (search-parameters l)) (l2 (list-filter (map member->theme l1) identity)))
    (list-remove-duplicates l2)
  ) ;let*
) ;tm-define

(tm-define (search-tag-themes l) (search-themes (tree-label l)))
