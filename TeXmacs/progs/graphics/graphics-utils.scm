
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : graphics-utils.scm
;; DESCRIPTION : utilities routines for graphics mode
;; COPYRIGHT   : (C) 2001  Joris van der Hoeven
;;               (C) 2004, 2005, 2006  Joris van der Hoeven and Henri Lesourd
;;               (C) 2011  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (graphics graphics-utils) (:use (graphics graphics-drd)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic scheme processing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; NOTE: This section is OK.
(define-macro (define-export-macro head . body)
  `(begin (define-macro ,head ,@body) (export ,(car head)))
) ;define-macro
(export define-export-macro)
;; NOTE: It seems that as soon as macros become a little bit complex,
;;   the Guile macroexpander interacts poorly with the memoizing stuff
;;   in (define-public-macro), and then it becomes unstable. This is
;;   the reason why (define-export-macro) exists : it is *strictly*
;;   equivalent to (define-macro) + (export), and never raises problems.

;; Conversions
(tm-define (tree->object t) (if (or (symbol? t) (number? t)) t (tm->stree t)))

(tm-define (object->tree o) (tm->tree o))

;; These abbreviations are very convenient
;; to use. A nice naming scheme is :
;;
;;  -> b=bool ;
;;  -> i=integer ;
;;  -> f=float ;
;;  -> sy=symbol ;
;;  -> s=string ;
;;  -> o=Scheme object ;
;;  -> p=path.
;;  -> t=tree.
;;
;;  One can add the missing ones on demand.
(tm-define f2s float->string)
(tm-define s2f string->float)
(tm-define sy2s symbol->string)
(tm-define s2sy string->symbol)
(tm-define o2s object->string)
(tm-define s2o string->object)
(tm-define t2o tree->object)
(tm-define o2t object->tree)

;; Error
(tm-define (graphics-error . msg) (for (e msg) (display e)) (newline))

;; Lists as bags
;; FIXME: One more time, due to an incomplete implementation
;;   of some very basic functionnality, we had to do our own
;;   hack...

(tm-define (complete-eq? x y)
  (if (and (tree? x) (tree? y)) (tree-eq? x y) (eq? x y))
) ;tm-define

(tm-define (seek-eq? x l)
  (if (pair? l) (if (complete-eq? x (car l)) l (seek-eq? x (cdr l))) #f)
) ;tm-define

(tm-define (remove-eq0? x l)
  (define l0 (cons 'X l))
  (define (seek l prec)
    (if (pair? l)
      (if (complete-eq? x (car l)) (set-cdr! prec (cdr l)) (seek (cdr l) (cdr prec)))
    ) ;if
  ) ;define
  (seek l l0)
  (cdr l0)
) ;tm-define

(define-export-macro (remove-eq? x l)
  (if (symbol? l) `(begin (set! ,l (remove-eq0? ,x ,l)) ,l) `(remove-eq0? ,x ,l))
) ;define-export-macro

;; Iterators

(tm-define-macro (foreach-number what . body)
  (let ((n (length what)))
    (cond ((== n 3)
           ;; (foreach-number (i i0 iN) body[i])
           `(for (,(car what) ,(cadr what) ,(caddr what)) ,@body)
          ) ;
          ((== n 4)
           ;; (foreach-number (i i0 [< <= > >=] iN) body[i])
           (if (in? (caddr what) '(< <=))
             `(for (,(car what) ,(cadr what) ,(cadddr what) ,1 ,(caddr what))
                ,@body)
             `(for (,(car what) ,(cadr what) ,(cadddr what) ,-1 ,(caddr what))
                ,@body)
           ) ;if
          ) ;
          ((== n 5)
           ;; (foreach-number (i i0 [< <= > >=] iN step) body[i])
           (if (in? (caddr what) '(< <=))
             `(for (,(car what)
                    ,(cadr what)
                    ,(cadddr what)
                    ,(car (cddddr what))
                    ,(caddr what))
                ,@body)
             `(for (,(car what)
                    ,(cadr what)
                    ,(cadddr what)
                    ,(- 0 (car (cddddr what)))
                    ,(caddr what))
                ,@body)
           ) ;if
          ) ;
          (else '(noop))
    ) ;cond
  ) ;let
) ;tm-define-macro

(tm-define-macro (foreach-cons i . b)
  ;; (foreach-cons (e l) i   body[elt]) -> for each cons e of the list l
  ;; (foreach-cons (e l1 l2) body[elt]) -> for each cons e in [l1...l2]
  (if (null? (cddr i))
    `(do ((,(car i) ,(cadr i) (cdr ,(car i))))
       ((null? ,(car i)) ,(cadr i))
       ,(cons 'begin b))
    `(do ((,(car i) ,(cadr i) (cdr ,(car i))))
       ((or (null? ,(car i)) (eq? ,(car i) ,(caddr i))) ,(cadr i))
       ,(cons 'begin b))
  ) ;if
) ;tm-define-macro
;; TODO: Extend (foreach-cons) for recursively traversing tree nodes.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Other utility routines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (string-number=? s1 s2)
  (and (string? s1)
    (string? s2)
    (let* ((i1 (string->float s1)) (i2 (string->float s2)))
      (and i1 i2 (== i1 i2))
    ) ;let*
  ) ;and
) ;tm-define

(tm-define (string-symbol=? s1 s2)
  (if (symbol? s1) (set! s1 (symbol->string s1)))
  (if (symbol? s2) (set! s2 (symbol->string s2)))
  (== s1 s2)
) ;tm-define

(tm-define (maprec func o)
  (if (pair? o) (map (lambda (x) (maprec func x)) o) (func o))
) ;tm-define

(tm-define (tm-copy o)
  (define (copy x)
    (if (tree? x) (tree-copy x) (copy-tree x))
  ) ;define
  (maprec copy o)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines for accessing trees & managing listprops
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; FIXME: Should use (tm-adjust-path), otherwise, crashes in some cases
(tm-define (tm-upwards-path p tags nottags)
  (if (in? (tree-label (path->tree p)) tags)
    p
    (if (in? (tree-label (path->tree p)) nottags)
      #f
      (if (> (length p) 2) (tm-upwards-path (cDr p) tags nottags) #f)
    ) ;if
  ) ;if
) ;tm-define
;; TODO: Put this one in kernel/library/tree.scm

;; NOTE: This section is OK.
(tm-define nothing (gensym))

(define list-find-prop-cons #f)
(tm-define (list-find-prop l var)
  (define (find l)
    (if (or (null? l) (null? (cdr l)))
      nothing
      (if (== (car l) var)
        (begin
          (set! list-find-prop-cons (cdr l))
          (cadr l)
        ) ;begin
        (if (null? (cdr l)) nothing (find (cddr l)))
      ) ;if
    ) ;if
  ) ;define
  (set! list-find-prop-cons #f)
  (if (null? l) nothing (find (cdr l)))
) ;tm-define

(tm-define (list-find&set-prop l var val)
  (list-find-prop l var)
  (if list-find-prop-cons
    (begin
      (set-car! list-find-prop-cons val)
      l
    ) ;begin
    `(with ,var ,val unquote (if (eq? (car l) 'with) (cdr l) l))
  ) ;if
) ;tm-define

(tm-define (tm-find-prop p var)
  ;; (tm-find-prop '<tree <with|a|1|...>> "a")              -> <tree 1>
  ;; (tm-find-prop (tree->path '<tree <with|a|1|...>>) "a") -> <tree 1>
  (if (null? p)
    nothing
    (let* ((t (if (tree? p) p (path->tree p))) (n (tree-arity t)))
      (if (> n 2)
        (with res
          nothing
          (foreach-number (i 0 < (- (/ n 2) 1))
            (if (== (tm->stree (tree-ref t (* 2 i))) var)
              (set! res (tree-ref t (+ (* 2 i) 1)))
            ) ;if
          ) ;foreach-number
          res
        ) ;with
        nothing
      ) ;if
    ) ;let*
  ) ;if
) ;tm-define

(tm-define (find-prop l var . default)
  ;; (find-prop '(with "a" 1) "a")             -> 1
  ;; (find-prop '(with "a" 1) "b" "not found") -> "not found"
  ;; (find-prop '<tree <with|a|1|...>> "a")    -> 1
  (set! default (if (pair? default) (car default) #f))
  (with val
   ((if (tree? l) tm-find-prop list-find-prop) l var)
   (if (== val nothing) default (if (tree? val) (tm->stree val) val))
  ) ;with
) ;tm-define
;; TODO : Put this in utils/library/tree.scm

(tm-define (get-upwards-tree-property p var)
  (if (null? p)
    nothing
    (with q
      (tm-upwards-path p '(with) '())
      (if (not q)
        nothing
        (with val
          (tm-find-prop q var)
          (if (== val nothing) (get-upwards-property (cDr q) var) val)
        ) ;with
      ) ;if
    ) ;with
  ) ;if
) ;tm-define

(tm-define (get-upwards-property p var) (t2o (get-upwards-tree-property p var)))

(tm-define (get-upwards-property-1 p var)
  (if (null? p)
    nothing
    (with q
      (tm-upwards-path p '(with) '())
      (if (equal? q (cDr p)) (get-upwards-property p var) nothing)
    ) ;with
  ) ;if
) ;tm-define
;; TODO : Put this in utils/library/tree.scm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines for accessing the graphics
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (stree-at p) (tm->stree (path->tree p)))
;; TODO: Put this in kernel/library/tree.scm

(tm-define (graphics-graphics-path)
  ;; path to innermost graphics tag
  (let* ((p (cDr (cursor-path))) (t (path->tree p)))
    (if (eq? (tree-label t) 'graphics)
      p
      (with u (tree-innermost 'graphics) (and u (tree->path u)))
    ) ;if
  ) ;let*
) ;tm-define

(tm-define (graphics-path path)
  (if (or (null? path) (null? (cdr path)))
    #f
    (with p
      (cDr path)
      (with o
        (path->tree p)
        (if (and (tree? o)
              ;; (in? (tree-label o) gr-tags-all)
              (not (eq? (tree-label o) 'string))
              (tm-upwards-path (cDr p) '(graphics) (graphical-text-tag-list))
            ) ;and
          (begin
            ;; (display* "gp=" (path->tree (cDr path)) "\n")
            (if (eq? (tree-label o) 'graphics) #f p)
          ) ;begin
          (graphics-path (cDr path))
        ) ;if
      ) ;with
    ) ;with
  ) ;if
) ;tm-define

(tm-define (graphics-active-path)
  ;; path to active tag
  (graphics-path (cursor-path))
) ;tm-define

(tm-define (graphics-group-path)
  ;; path to innermost group
  (graphics-graphics-path)
) ;tm-define

(tm-define (graphics-default-unit) (if (in-poster?) "0.1par" "1cm"))

(tm-define (make-graphics . init)
  (when (null? init)
    (set! init
      `(,"gr-mode"
        ,"point"
        ,"gr-frame"
        (tuple ,"scale" ,(graphics-default-unit) (tuple "0.5gw" "0.5gh"))
        ,"gr-geometry"
        (tuple "geometry" "1par" "0.6par")
        ,"gr-grid"
        (tuple "cartesian" (point "0" "0") "1")
        ,"gr-edit-grid-aspect"
        (tuple (tuple "axes" "none") (tuple "1" "none") (tuple "10" "none"))
        ,"gr-edit-grid"
        (tuple "cartesian" (point "0" "0") "1"))
    ) ;set!
  ) ;when
  (graphics-reset-context 'begin)
  (insert-raw-go-to `(with ,@init (graphics "")) `(,(length init) ,1))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines for accessing the properties of the graphics
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; NOTE: This section is OK.

(define (graphics-get-raw-property var)
  (with val
    (get-upwards-tree-property (graphics-graphics-path) var)
    (if (eq? val nothing)
      (get-default-tree-val var)
      (if (eq? (tm-car val) 'quote) (tree-ref val 0) val)
    ) ;if
  ) ;with
) ;define

(tm-define (graphics-get-property var)
  (with val (graphics-get-raw-property var) (tm->stree val))
) ;tm-define

(tm-define (graphics-set-property var val)
  (with p
    (graphics-graphics-path)
    (cond ((tree? val) (graphics-set-property var (tm->stree val)))
          ((== val "default") (graphics-remove-property var))
          ((== val (graphics-attribute-default var)) (graphics-remove-property var))
          (p (path-insert-with p var val))
    ) ;cond
    (when (not (tree? val)) (graphics-sync-type-config var val))
  ) ;with
) ;tm-define

(tm-define (graphics-remove-property var)
  (with p (graphics-graphics-path) (if p (path-remove-with p var)))
) ;tm-define

;; Magnification
(tm-define (graphics-eval-magnify)
  (graphical-get-attribute (path->tree (graphics-graphics-path)) "magnify")
) ;tm-define

(tm-define (graphics-eval-magnify-at path)
  (graphical-get-attribute (path->tree (cDr path)) "magnify")
) ;tm-define

(tm-define (magnify->number m)
  (cond ((number? m) m)
        ((== m "default") 1)
        ((== m #f) 1)
        ((== m "mixed") 1)
        (else (string->number m))
  ) ;cond
) ;tm-define

(tm-define (number->magnify m)
  (cond ((and (> m 0.999999) (< m 1.000001)) "default")
        (else (number->string m))
  ) ;cond
) ;tm-define

(tm-define (multiply-magnify m1 m2)
  (number->magnify (* (magnify->number m1) (magnify->number m2)))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Per-type property configuration (gr-props-<type>)
;;
;; 为每种图形类型各存一份独立的属性配置，随文档持久化于 <graphics> 外层
;; with 变量 gr-props-<type>（成对 属性名 值 的 tuple，仅存相关且非默认项）。
;; 改属性时实时写入当前类型存储；切换类型时把目标存储恢复到全局 gr-*。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; per-type 配置对应的 <graphics> with 变量名
(tm-define (graphics-props-var tag)
  (string-append "gr-props-" (symbol->string tag))
) ;tm-define

;; 单条 gr-* 写入时，实时并入当前绘制类型的配置
;; 排除 gr-mode 与 gr-props-* 自身（避免递归）
(tm-define (graphics-sync-type-config var val)
  (let* ((m (graphics-mode))
         (tag (and (func? m 'edit 1) (cadr m)))
         (attr (and (gr-prefixed? var) (gr-unprefix var))))
    (when (and tag attr (!= var "gr-mode")
               (not (string-starts? var "gr-props-"))
               (graphics-mode-attribute? `(edit ,tag) attr))
      (with tab (graphics-get-type-config tag)
        (if (and val (!= val "default") (!= val (graphics-attribute-default attr)))
          (ahash-set! tab attr val)
          (ahash-remove! tab attr)
        ) ;if
        (graphics-set-type-config tag tab)
      ) ;with
    ) ;when
  ) ;let*
) ;tm-define

;; 读取当前类型的配置（属性名 -> 值），未存则空表
(tm-define (graphics-get-type-config tag)
  (let* ((tab (make-ahash-table))
         (raw (graphics-get-property (graphics-props-var tag))))
    (when (and (pair? raw) (== (car raw) 'tuple))
      (with l (cdr raw)
        (while (nnull? l)
          (ahash-set! tab (car l) (cadr l))
          (set! l (cddr l))
        ) ;while
      ) ;with
    ) ;when
    tab
  ) ;let*
) ;tm-define

;; 把配置写回 gr-props-<type>
(tm-define (graphics-set-type-config tag tab)
  (with p (graphics-graphics-path)
    (when p
      (with pairs (append-map (lambda (x) (list (car x) (cdr x))) (ahash-table->list tab))
        (if (null? pairs)
          (path-remove-with p (graphics-props-var tag))
          (path-insert-with p (graphics-props-var tag) `(tuple ,@pairs))
        ) ;if
      ) ;with
    ) ;when
  ) ;with
) ;tm-define

;; 把该类型的配置回填到全局 gr-*
(tm-define (graphics-restore-type-config tag)
  (let* ((attrs (graphics-mode-attributes `(edit ,tag)))
         (tab (graphics-get-type-config tag))
         (p (graphics-graphics-path)))
    (when (and (nnull? attrs) p)
      (for (attr attrs)
        (let* ((var (gr-prefix attr))
               (val (or (ahash-ref tab attr) "default")))
          (if (or (== val "default") (== val (graphics-attribute-default attr)))
            (path-remove-with p var)
            (path-insert-with p var val)
          ) ;if
        ) ;let*
      ) ;for
    ) ;when
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Enriching graphics with properties like color, line width, etc.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (graphics-enrich-filter t l)
  (if (null? l)
    l
    (let* ((head (car l)) (tail (graphics-enrich-filter t (cdr l))))
      (if (or (not (cadr head))
            (== (cadr head) "default")
            (== (cadr head) (get-default-val (car head)))
            (not (graphics-attribute? t (car head)))
          ) ;or
        tail
        (cons* (car head) (cadr head) tail)
      ) ;if
    ) ;let*
  ) ;if
) ;tm-define

(tm-define (graphics-enrich-sub t l)
  (with f (graphics-enrich-filter (car t) l) (if (null? f) t `(with ,@f ,t)))
) ;tm-define

(tm-define (graphics-enrich-bis t id tab)
  (set! tab (list->ahash-table (ahash-table->list tab)))
  (ahash-remove! tab "gid")
  (let* ((attrs (graphical-relevant-attributes t))
         (sel (ahash-table-select tab attrs))
         (l1 (cons (cons "gid" id) (ahash-table->list sel)))
         (l2 (map (lambda (x) (list (car x) (cdr x))) l1))
        ) ;
    ;; (display* "l= " l2 "\n")
    (graphics-enrich-sub t l2)
  ) ;let*
) ;tm-define

(tm-define (graphics-enrich t)
  (let* ((l1 (graphics-all-attributes))
         (l2 (map gr-prefix l1))
         (l3 (map graphics-get-property l2))
         (tab (list->ahash-table (map cons l1 l3)))
        ) ;
    (graphics-enrich-bis t "default" tab)
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines for modifying the innermost group of graphics
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (graphics-group-insert-bis t go-into)
  (let* ((p (graphics-group-path)) (p2 #f))
    (if (null? layer-of-last-removed-object) (set! layer-of-last-removed-object #f))
    (if p
      (with n
        (if layer-of-last-removed-object
          (if (pair? layer-of-last-removed-object)
            (with val
              (car layer-of-last-removed-object)
              (set! layer-of-last-removed-object (cdr layer-of-last-removed-object))
              val
            ) ;with
            layer-of-last-removed-object
          ) ;if
          (tree-arity (path->tree p))
        ) ;if
        (tree-insert (path->tree p) n (list t))
        (if (func? t 'with)
          (if (and go-into (graphical-text-context? (cAr t)))
            (set! p2 (append p (list n (- (length t) 2) 0 0)))
            (set! p2 (append p (list n (- (length t) 2) 1)))
          ) ;if
          (if (and go-into (graphical-text-context? t))
            (set! p2 (append p (list n 0 0)))
            (set! p2 (append p (list n 0)))
          ) ;if
        ) ;if
        (go-to p2)
        (graphics-path p2)
      ) ;with
      #f
    ) ;if
  ) ;let*
) ;tm-define

(tm-define (graphics-group-insert t) (graphics-group-insert-bis t #t))

(tm-define (graphics-group-enrich-insert t)
  (graphics-group-insert (graphics-enrich t))
) ;tm-define

(tm-define (graphics-group-start)
  (graphics-finish)
  (with p (graphics-group-path) (if p (go-to (append p '(0 0)))))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines for modifying the active tag
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; NOTE: This section is OK.
(tm-define (graphics-tree path)
  (with p (graphics-path path) (if p (path->tree p) #f))
) ;tm-define

(tm-define (graphics-object path)
  (with p (graphics-path path) (if p (tm->stree (path->tree p)) #f))
) ;tm-define

(tm-define (graphics-active-object)
  (with p (graphics-active-path) (if p (tm->stree (path->tree p)) #f))
) ;tm-define

(tm-define (get-default-tree-val var) (get-init-tree var))

(tm-define (get-default-val var) (tm->stree (get-init-tree var)))

(tm-define (graphics-path-property-bis p var default-val)
  (with c (get-upwards-property p var) (if (== c nothing) default-val c))
) ;tm-define

(tm-define (graphics-path-property p var)
  (graphics-path-property-bis p var "default")
) ;tm-define

(tm-define (graphics-path-property-bis-1 p var default-val)
  (with c (get-upwards-property-1 p var) (if (== c nothing) default-val c))
) ;tm-define

(tm-define (graphics-path-property-1 p var)
  (graphics-path-property-bis-1 p var "default")
) ;tm-define

;; (tm-define (graphics-object-root-path p)
;;  (let* ((q (tm-upwards-path p '(with) '()))
;;         (path (if (and q (== (+ (length q) 1) (length p))) q p)))
;;    path))

(tm-define (graphics-object-root-path p)
  (with t
    (path->tree p)
    (cond ((tree-in? t :up '(with anim-edit)) (graphics-object-root-path (cDr p)))
          (else p)
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (graphics-remove p . parms)
  (when p
    (with p0
      (graphics-object-root-path p)
      (set! layer-of-last-removed-object
        (if (and (pair? parms) (eq? (car parms) 'memoize-layer))
          (if (list? layer-of-last-removed-object)
            (cons (cAr p0) layer-of-last-removed-object)
            (cAr p0)
          ) ;if
          #f
        ) ;if
      ) ;set!
      (when (list-starts? (cursor-path) p0)
        (with-innermost t 'graphics (tree-go-to t 0 :start))
      ) ;when
      (tree-remove (path->tree (cDr p0)) (cAr p0) 1)
    ) ;with
  ) ;when
) ;tm-define

(tm-define (graphics-assign p t)
  (when p
    (tree-assign (path->tree p) t)
    (go-to (rcons p 1))
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Box info & frame
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (box-info t cmd) (tm->stree (texmacs-exec* `(box-info ,t ,cmd))))

(tm-define (frame-direct p) (tm->stree (texmacs-exec* `(frame-direct ,p))))

(tm-define (frame-inverse p) (tm->stree (texmacs-exec* `(frame-inverse ,p))))

(tm-define (interval-intersects i1 i2)
  (let* ((i1a (car i1)) (i1b (cadr i1)) (i2a (car i2)) (i2b (cadr i2)))
    (or (and (<= i1a i2a) (>= i1b i2b))
      (and (<= i2a i1a) (>= i2b i1b))
      (and (>= i1a i2a) (<= i1a i2b))
      (and (>= i2a i1a) (<= i2a i1b))
    ) ;or
  ) ;let*
) ;tm-define

(tm-define (box-intersects t1 t2)
  (define (max-box t)
    (let* ((bx1 (box-info t "lbrt")) (bx2 (box-info t "LBRT")))
      (set! bx1 (map s2f (cdr bx1)))
      (set! bx2 (map s2f (cdr bx2)))
      `(,(min (car bx1) (car bx2))
        ,(min (cadr bx1) (cadr bx2))
        ,(max (caddr bx1) (caddr bx2))
        ,(max (cadddr bx1) (cadddr bx2)))
    ) ;let*
  ) ;define
  (let* ((box1 (max-box t1)) (box2 (max-box t2)))
    (and (interval-intersects `(,(car box1) ,(caddr box1)) `(,(car box2)
                                                             ,(caddr box2)))
      (interval-intersects `(,(cadr box1) ,(cadddr box1))
        `(,(cadr box2) ,(cadddr box2))
      ) ;interval-intersects
    ) ;and
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Enhanced trees
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (enhanced-tree? t) (tree-in? t '(with anim-edit)))

(tm-define (enhanced-tree->radical t)
  (cond ((tree-is? t 'with) (enhanced-tree->radical (tree-ref t :last)))
        ((tree-is? t 'anim-edit) (enhanced-tree->radical (tree-ref t 1)))
        (else t)
  ) ;cond
) ;tm-define

(tm-define (radical->enhanced-tree r)
  (with t (tree-up r) (if (enhanced-tree? t) (radical->enhanced-tree t) r))
) ;tm-define

(tm-define (stree-radical t)
  (cond ((tm-is? t 'with) (stree-radical (tm-ref t :last)))
        ((tm-is? t 'anim-edit) (stree-radical (tm-ref t 1)))
        (else t)
  ) ;cond
) ;tm-define

(tm-define (stree-radical* t anim?)
  (cond ((and (tm-is? t 'with) (not anim?)) (stree-radical* (tm-ref t :last) anim?))
        ((tm-is? t 'anim-edit) (stree-radical* (tm-ref t 1) #t))
        (else t)
  ) ;cond
) ;tm-define

(tm-define (graphics-re-enhance obj compl anim?)
  (cond ((tm-is? compl 'anim-edit)
         `(anim-edit ,(tm-ref compl 0)
            ,(graphics-re-enhance obj (tm-ref compl 1) #t)
            ,@(cddr (tm-children compl)))
        ) ;
        ((and (tm-is? compl 'with) (or anim? (tm-is? (tm-ref compl :last) 'anim-edit)))
         `(with ,@(cDr (tm-children compl))
            ,(graphics-re-enhance obj (tm-ref compl :last) anim?))
        ) ;
        (else obj)
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Animations
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (graphics-anim-frames t)
  (and (tm-in? t '(anim-static anim-dynamic))
    (tm-is? (tm-ref t 0) 'morph)
    (with c
      (tm-children (tm-ref t 0))
      (and (list-and (map (lambda (x) (tm-func? x 'tuple 2)) c))
        (map (lambda (x) (tm-ref x 1)) c)
      ) ;and
    ) ;with
  ) ;and
) ;tm-define

(tm-define (graphics-anim-radicals t)
  (and-with l (graphics-anim-frames t) (map stree-radical l))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; New style graphical attributes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (group-pairs l)
  (if (or (null? l) (null? (cdr l)))
    '()
    (cons (cons (car l) (cadr l)) (group-pairs (cddr l)))
  ) ;if
) ;define

(tm-define (with-get-attributes t)
  (let* ((a1 (cDr (tree-children t))) (a2 (map tm->stree a1)) (a3 (group-pairs a2)))
    (list->ahash-table a3)
  ) ;let*
) ;tm-define

(define (ungroup-pairs l)
  (if (null? l) l (cons (caar l) (cons (cdar l) (ungroup-pairs (cdr l)))))
) ;define

(tm-define (with-set-attributes t tab)
  (let* ((a1 (ahash-table->list tab)) (a2 (ungroup-pairs a1)) (a3 (map tm->tree a2)))
    (tree-remove! t 0 (- (tree-arity t) 1))
    (tree-insert! t 0 a3)
    t
  ) ;let*
) ;tm-define

(tm-define (graphical-get-attributes t)
  (cond ((not (tree? t)) (graphical-get-attributes (tm->tree t)))
        ((not (tree-is? t 'with))
         (if (tree-is? t :up 'with)
           (graphical-get-attributes (tree-ref t :up))
           (make-ahash-table)
         ) ;if
        ) ;
        (else (with-get-attributes t))
  ) ;cond
) ;tm-define

(tm-define (graphical-set-attributes t tab)
  (cond ((not (tree? t)) (tm->stree (graphical-set-attributes (tm->tree t) tab)))
        ((not (tree-is? t 'with))
         (if (tree-is? t :up 'with)
           (graphical-set-attributes (tree-ref t :up) tab)
           (begin
             (tree-insert-node! t 0 '(with))
             (graphical-set-attributes t tab)
           ) ;begin
         ) ;if
        ) ;
        (else (set! t (with-set-attributes t tab))
          (if (and (tree-is? t 'with) (== (tree-arity t) 1)) (tree-remove-node! t 0))
          t
        ) ;else
  ) ;cond
) ;tm-define

(tm-define (graphical-get-attribute* t var)
  (ahash-ref (graphical-get-attributes t) var)
) ;tm-define

(tm-define (graphical-get-attribute t var)
  (or (graphical-get-attribute* t var) (graphics-attribute-default var))
) ;tm-define

(tm-define (graphical-set-attribute t var val)
  (with new
    (graphical-get-attributes t)
    (if val (ahash-set! new var val) (ahash-remove! new var))
    (graphical-set-attributes t new)
  ) ;with
) ;tm-define

(tm-define (graphical-get-selected-attributes* t filter-list)
  (let* ((old (graphical-get-attributes t)) (new (make-ahash-table)))
    (for (var filter-list)
      (if (ahash-ref old var) (ahash-set! new var (ahash-ref old var)))
    ) ;for
    new
  ) ;let*
) ;tm-define

(tm-define (graphical-get-selected-attributes t filter-list)
  (with tab
    (graphical-get-selected-attributes t filter-list)
    (for (var filter-list)
      (if (and (not (ahash-ref tab var)) (graphics-attribute-default var))
        (ahash-set! tab var (graphics-attribute-default var))
      ) ;if
    ) ;for
    tab
  ) ;with
) ;tm-define

(tm-define (graphical-set-selected-attributes t filter-list tab)
  (with new
    (graphical-get-attributes t)
    (for (var filter-list)
      (if (ahash-ref tab var)
        (ahash-set! new var (ahash-ref tab var))
        (ahash-remove! new var)
      ) ;if
    ) ;for
    (graphical-set-attributes t new)
  ) ;with
) ;tm-define

(tm-define (graphical-relevant-attributes t)
  (cond ((tm-is? t 'with) (graphical-relevant-attributes (tm-ref t :last)))
        ((tm-compound? t) (graphical-relevant-attributes (tm-car t)))
        (else (graphics-attributes t))
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Useful routines for processing points
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (point-get-x pt) (s2f (cadr pt)))

(tm-define (point-get-y pt) (s2f (caddr pt)))

(tm-define (points-add pt1 pt2)
  `(point ,(f2s (+ (point-get-x pt1) (point-get-x pt2)))
     ,(f2s (+ (point-get-y pt1) (point-get-y pt2))))
) ;tm-define

(tm-define (points-sub pt1 pt2)
  `(point ,(f2s (- (point-get-x pt1) (point-get-x pt2)))
     ,(f2s (- (point-get-y pt1) (point-get-y pt2))))
) ;tm-define

(tm-define (point-times pt f)
  `(point ,(f2s (* (point-get-x pt) f)) ,(f2s (* (point-get-y pt) f)))
) ;tm-define

(tm-define (point-div pt f)
  `(point ,(f2s (/ (point-get-x pt) f)) ,(f2s (/ (point-get-y pt) f)))
) ;tm-define

(tm-define (point-rotate-90 pt)
  `(point ,(f2s (- (point-get-y pt))) ,(f2s (point-get-x pt)))
) ;tm-define

(tm-define (points-mid P1 P2) (point-div (points-add P1 P2) 2))

(define (square x)
  (* x x)
) ;define

(tm-define (point-norm P)
  (sqrt (+ (square (point-get-x P)) (square (point-get-y P))))
) ;tm-define

(tm-define (point-get-unit P) (point-div P (point-norm P)))

(tm-define (points-cross-product-k P1 P2)
  (let* ((p1x (point-get-x P1))
         (p1y (point-get-y P1))
         (p2x (point-get-x P2))
         (p2y (point-get-y P2))
        ) ;
    (- (* p1x p2y) (* p1y p2x))
  ) ;let*
) ;tm-define

(tm-define (points-distance P1 P2) (point-norm (points-sub P1 P2)))

(tm-define (points-sum obj)
  (if (pair? obj)
    (if (match? obj '(point :%2))
      obj
      (points-add (points-sum (car obj)) (points-sum (cdr obj)))
    ) ;if
    '(point "0" "0")
  ) ;if
) ;tm-define
