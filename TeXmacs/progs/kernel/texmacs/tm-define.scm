
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-define.scm
;; DESCRIPTION : Macros for defining TeXmacs functions
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel texmacs tm-define))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Contextual overloading
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (ctx-add-condition l kind opt)
  ;; (display* "add condition " l ", " opt "\n")
  (append l (list opt))
) ;define-public

(define-public (ctx-insert ctx data conds)
  ;; (display* "insert " ctx ", " data ", " conds "\n")
  (cons (cons conds data) (or ctx '()))
) ;define-public

(define-public (ctx-find ctx conds)
  ;; (display* "find " ctx ", " conds "\n")
  (cond ((or (not ctx) (null? ctx)) #f)
        ((== (caar ctx) conds) (cdar ctx))
        (else (ctx-find (cdr ctx) conds))
  ) ;cond
) ;define-public

(define-public (ctx-remove ctx conds)
  ;; (display* "remove " ctx ", " conds "\n")
  (cond ((or (not ctx) (null? ctx)) '())
        ((== (caar ctx) conds) (ctx-remove (cdr ctx) conds))
        (else (cons (car ctx) (ctx-remove (cdr ctx) conds)))
  ) ;cond
) ;define-public

(define (and-apply l args)
  (or (null? l) (and (apply (car l) (or args '())) (and-apply (cdr l) args)))
) ;define

(define-public (ctx-resolve ctx args)
  ;; (display* "resolve " ctx ", " args "\n")
  (cond ((or (not ctx) (null? ctx)) #f)
        ((and-apply (caar ctx) args) (cdar ctx))
        (else (ctx-resolve (cdr ctx) args))
  ) ;cond
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Global variables and subroutines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public tm-defined-table (make-ahash-table))
(define-public tm-defined-name (make-ahash-table))
(define-public tm-defined-module (make-ahash-table))
(define-public define-option-table (make-hash-table 100))

(define-public cur-conds '())

(define cur-props-table (make-ahash-table))

(define cur-props '())

(define (ca*r x)
  (if (pair? x) (ca*r (car x)) x)
) ;define

(define (ca*adr x)
  (ca*r (cadr x))
) ;define

(define (lambda* head body)
  (if (pair? head) (lambda* (car head) `((lambda ,(cdr head) ,@body))) (car body))
) ;define

(define (listify args)
  (if (pair? args) (cons (car args) (listify (cdr args))) (list args))
) ;define

(define (apply* fun head)
  (cond ((list? head) `(,(apply* fun (car head)) ,@(cdr head)))
        ((pair? head) `(apply ,(apply* fun (car head))
                         (cons* ,@(listify (cdr head)))))
        (else fun)
  ) ;cond
) ;define

(define (and* conds)
  (if (list-1? conds) (car conds) `(and ,@conds))
) ;define

(define (begin* conds)
  (if (list-1? conds) (car conds) `(begin ,@conds))
) ;define

(define-public (procedure-name fun)
  (if (procedure? fun)
    (or (ahash-ref tm-defined-name fun)
      (let ((str (format #f "~A" fun)))
        (if (string-starts? str "#<lambda") #f (string->symbol str))
      ) ;let
    ) ;or
    #f
  ) ;if
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Overloading
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (ctx-add-condition! kind opt)
  (set! cur-conds (ctx-add-condition cur-conds kind opt))
) ;define

(define (define-option-mode opt decl)
  (ctx-add-condition! 0 (car opt))
  decl
) ;define

(define-public (predicate-option? x)
  (or (and (symbol? x) (string-ends? (symbol->string x) "?"))
    (and (pair? x) (== (car x) 'lambda))
  ) ;or
) ;define-public

(define (define-option-match opt decl)
  (cond ((predicate-option? opt) (ctx-add-condition! 3 opt))
        ((and (pair? opt)
           (null? (cdr opt))
           (predicate-option? (car opt))
           (list? (cadr decl))
           (= (length (cadr decl)) 3)
         ) ;and
         (ctx-add-condition! 3 (car opt))
        ) ;
        (else (ctx-add-condition! 3 `(lambda args (match? args (quote ,opt)))))
  ) ;cond
  decl
) ;define

(define (define-option-require opt decl)
  (define-option-match `(lambda ,(cdadr decl) ,(car opt)) decl)
) ;define

(define (define-option-applicable opt decl)
  (with prop
    `((quote ,(ca*adr decl)) ,:applicable (list (lambda args ,@opt)))
    (set! cur-props (cons prop cur-props))
    ;; (define-option-require opt decl)
    decl
  ) ;with
) ;define

(ahash-set! define-option-table :mode define-option-mode)
(ahash-set! define-option-table :require define-option-require)
(ahash-set! define-option-table :applicable define-option-applicable)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Properties of overloaded functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (filter-conds l)
  "Remove conditions which depend on arguments from list"
  (cond ((null? l) l)
        ((>= (car l) 2) (filter-conds (cddr l)))
        (else (cons (car l) (cons (cadr l) (filter-conds (cddr l)))))
  ) ;cond
) ;define

(define-public (property-set! var prop what conds*)
  "Associate a property to a function symbol under conditions"
  (let* ((key (cons var prop)) (conds (filter-conds conds*)))
    (ahash-set! cur-props-table
      key
      (ctx-insert (ahash-ref cur-props-table key) what conds)
    ) ;ahash-set!
  ) ;let*
) ;define-public

(define-public (property var prop)
  "Retrieve a property of a function symbol"
  (if (procedure? var) (set! var (procedure-name var)))
  (let* ((key (cons var prop)))
    (ctx-resolve (ahash-ref cur-props-table key) #f)
  ) ;let*
) ;define-public

(define (property-rewrite l)
  `(property-set! ,@l (list ,@cur-conds))
) ;define

(define (define-property which)
  (lambda (opt decl)
    (set! cur-props (cons `((quote ,(ca*adr decl)) ,which (quote ,opt)) cur-props))
    decl
  ) ;lambda
) ;define

(define (define-property . l)
  (lambda (opt decl)
    (for (which l)
      (set! cur-props (cons `((quote ,(ca*adr decl)) ,which (quote ,opt)) cur-props))
    ) ;for
    decl
  ) ;lambda
) ;define

(define (define-property* which)
  (lambda (opt decl)
    (set! cur-props (cons `((quote ,(ca*adr decl)) ,which (list ,@opt)) cur-props))
    decl
  ) ;lambda
) ;define

(define (compute-arguments decl)
  (cond ((pair? (cadr decl)) (cdadr decl))
        ((and (pair? (caddr decl)) (== (caaddr decl) 'lambda)) (cadr (caddr decl)))
        (else (texmacs-error "compute-arguments" "Bad argument documentation"))
  ) ;cond
) ;define

(define (define-option-argument opt decl)
  (let* ((var (ca*adr decl))
         (args (compute-arguments decl))
         (arg (list :argument (car opt)))
        ) ;
    (set! cur-props (cons `((quote ,var) ,:arguments (quote ,args)) cur-props))
    (set! cur-props
      (cons `((quote ,var) (quote ,arg) (quote ,(cdr opt))) cur-props)
    ) ;set!
    decl
  ) ;let*
) ;define

(define (define-option-default opt decl)
  (let* ((var (ca*adr decl)) (arg (list :default (car opt))))
    (set! cur-props
      (cons `((quote ,var) (quote ,arg) (lambda ,() ,@(cdr opt))) cur-props)
    ) ;set!
    decl
  ) ;let*
) ;define

(define (define-option-proposals opt decl)
  (let* ((var (ca*adr decl)) (arg (list :proposals (car opt))))
    (set! cur-props
      (cons `((quote ,var) (quote ,arg) (lambda ,() ,@(cdr opt))) cur-props)
    ) ;set!
    decl
  ) ;let*
) ;define

(ahash-set! define-option-table :type (define-property :type))
(ahash-set! define-option-table :synopsis (define-property :synopsis))
(ahash-set! define-option-table
  :synopsis*
  (define-property :synopsis :synopsis*)
) ;ahash-set!
(ahash-set! define-option-table :returns (define-property :returns))
(ahash-set! define-option-table :note (define-property :note))
(ahash-set! define-option-table :argument define-option-argument)
(ahash-set! define-option-table :default define-option-default)
(ahash-set! define-option-table :proposals define-option-proposals)
(ahash-set! define-option-table :secure (define-property* :secure))
(ahash-set! define-option-table :check-mark (define-property* :check-mark))
(ahash-set! define-option-table :interactive (define-property* :interactive))
(ahash-set! define-option-table :balloon (define-property* :balloon))

(define-public (procedure-sources about)
  (or (and (procedure? about) (ahash-ref tm-defined-table (procedure-name about)))
    (and (procedure-source about) (list (procedure-source about)))
  ) ;or
) ;define-public

(define-public (help about)
  ;; very provisional
  (cond ((property about :synopsis) (property about :synopsis))
        ((procedure-documentation about) (procedure-documentation about))
        (else #f)
  ) ;cond
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Overloaded functions with properties
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (unlambda pred?)
  (if (func? pred? 'lambda) (caddr pred?) (list pred?))
) ;define

(define-public (tm-add-condition var head body)
  (if (null? cur-conds)
    body
    `((if ,(and* (map unlambda cur-conds))
        ,(begin* body)
        ,(apply* 'former head)))
  ) ;if
) ;define-public

(define-public-macro (tm-define-overloaded head . body)
  (let* ((var (ca*r head))
         (nbody (tm-add-condition var head body))
         (nval (lambda* head nbody))
        ) ;
    (if (ahash-ref tm-defined-table var)
      `(let ((former ,var))
         ;; (if (== (length (ahash-ref tm-defined-table ',var)) 1)
         ;;    (display* "Overloaded " ',var "\n"))
         ;; (display* "Overloaded " ',var "\n")
         ;; (display* "   " ',nval "\n")
         (set! ,var ,nval)
         (ahash-set! tm-defined-table
           (quote ,var)
           (cons (quote ,nval) (ahash-ref tm-defined-table (quote ,var))))
         (ahash-set! tm-defined-name ,var (quote ,var))
         (ahash-set! tm-defined-module
           (quote ,var)
           (cons *module-name* (ahash-ref tm-defined-module (quote ,var))))
         ,@(map property-rewrite cur-props))
      `(begin
         (when (nnull? cur-conds)
           (display* ,"warning: conditional master routine " (quote ,var) ,"\n")
           (display* ,"   " (quote ,nval) ,"\n"))
         ;; (display* "Defined " ',var "\n")
         ;; (if (nnull? cur-conds) (display* "   " ',nval "\n"))
         (varlet (rootlet)
           (quote ,var)
           (if (null? cur-conds)
             ,nval
             ,(list 'let '((former (lambda args (noop)))) nval)))
         (ahash-set! tm-defined-table (quote ,var) (list (quote ,nval)))
         (ahash-set! tm-defined-name ,var (quote ,var))
         (ahash-set! tm-defined-module (quote ,var) (list *module-name*))
         ,@(map property-rewrite cur-props))
    ) ;if
  ) ;let*
) ;define-public-macro

(define-public (tm-define-sub head body)
  (if (and (pair? (car body)) (keyword? (caar body)))
    (let ((decl (tm-define-sub head (cdr body))))
      (if (not (ahash-ref define-option-table (caar body)))
        (texmacs-error "tm-define-sub" "unknown option ~S" (caar body))
      ) ;if
      ((ahash-ref define-option-table (caar body)) (cdar body) decl)
    ) ;let
    (cons 'tm-define-overloaded (cons head body))
  ) ;if
) ;define-public

(define-public-macro (tm-define head . body)
  (set! cur-conds '())
  (set! cur-props '())
  (tm-define-sub head body)
) ;define-public-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Overloaded macros with properties
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (tm-macroify head)
  (if (pair? head)
    (cons (tm-macroify (car head)) (cdr head))
    (string->symbol (string-append (symbol->string head) "$impl"))
  ) ;if
) ;define-public

(define-public-macro (tm-define-macro head . body)
  (with macro-head
    (tm-macroify head)
    ;; (display* (ca*r head) "\n")
    ;; (display* "   " `(tm-define ,macro-head ,@body) "\n")
    ;; (display* "   " `(define-public-macro ,head
    ;;                   ,(apply* (ca*r macro-head) head)) "\n")
    `(begin
       (tm-define ,macro-head ,@body)
       (with-module *texmacs-user-module*
         (define-public-macro ,head ,(apply* (ca*r macro-head) head))))
  ) ;with
) ;define-public-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Associating extra properties to existing function symbols
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (tm-property-sub head body)
  (if (null? body)
    (cons 'tm-property-overloaded (cons head body))
    (let ((decl (tm-property-sub head (cdr body))))
      ((ahash-ref define-option-table (caar body)) (cdar body) decl)
    ) ;let
  ) ;if
) ;define-public

(define-public-macro (tm-property head . body)
  (set! cur-conds '())
  (set! cur-props '())
  (tm-property-sub head body)
) ;define-public-macro

(define-public-macro (tm-property-overloaded head . body)
  `(begin ,@(map property-rewrite cur-props))
) ;define-public-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Lazy function declations
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define lazy-define-table (make-ahash-table))

(define-public (not-define-option? item)
  (not (and (pair? item) (keyword? (car item))))
) ;define-public

(define-public (lazy-define-one module opts name)
  (let* ((old (ahash-ref lazy-define-table name))
         (new (if old (cons module old) (list module)))
        ) ;
    (ahash-set! lazy-define-table name new)
  ) ;let*
  (with name-star
    (string->symbol (string-append (symbol->string name) "*"))
    `(when (not (defined? (quote ,name)))
       (tm-define (,name . args)
         ,@opts
         (let* ((m (resolve-module (quote ,module))) (r (m (quote ,name))))
           (if (not r)
             (texmacs-error ,"lazy-define"
               ,(string-append "Could not retrieve " (symbol->string name))))
           (apply r args))))
  ) ;with
) ;define-public

(define-public-macro (lazy-define module . names)
  (receive (opts real-names)
    (list-break names not-define-option?)
    `(begin ,@(map (lambda (name) (lazy-define-one module opts name)) names))
  ) ;receive
) ;define-public-macro

(define-public (lazy-define-force name)
  (if (procedure? name) (set! name (procedure-name name)))
  (let* ((im (ahash-ref lazy-define-table name)) (modules (if im im '())))
    (ahash-remove! lazy-define-table name)
    (for-each module-provide modules)
  ) ;let*
) ;define-public
