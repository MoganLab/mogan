;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : apidoc-funcs.scm
;; DESCRIPTION : Routines for documentation of the scheme api
;; COPYRIGHT   : (C) 2012 Miguel de Benito Delgado
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; This file contains procedures needed for the display of documentation
;; collected using module (doc apidoc-collect).
;; As usual, procedures prefixed with a dollar sign return strees for display.
;; Most of the time they have an unprefixed counterpart which does the work.
;;
;; TODO:
;;  - use the code indexer when it's ready and ditch ad-hoc parsing made here
;;  - fix the implementation of refresh-widget to fix the module browser
;;  - this list
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (doc apidoc-funcs)
  (:use (convert rewrite init-rewrite)
    (doc apidoc-collect)
    (kernel gui gui-markup)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Conversions related to modules:

(tm-define (string->module str)
  (:synopsis "Returns the list representation of a module given as a string")
  (if (or (not (string? str)) (== str ""))
    '()
    (map string->symbol (string-split str #\.))
  ) ;if
) ;tm-define

(tm-define (module->string module)
  (:synopsis "Formats a module in list format (some module) as some.module")
  (cond ((list? module) (string-join (map symbol->string module) "."))
        ((symbol? module) (symbol->string module))
        (else "")
  ) ;cond
) ;tm-define

(define (module->name module)
  "Retrieves the name of the file for @module, without extension"
  (symbol->string (cAr module))
) ;define

(define (module->path module)
  "Returns the full path of the given module, without extension"
  (url-concretize (string-append "$TEXMACS_PATH/progs/"
                    (cond ((list? module) (string-join (map symbol->string module) "/"))
                          ((symbol? module) (symbol->string module))
                          (else "")
                    ) ;cond
                  ) ;string-append
  ) ;url-concretize
) ;define

(define (symbol->tree s)
  (string->tree (symbol->string s))
) ;define

(define (module-leq? x y)
  (string<=? (module->string x) (module->string y))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Compute and display information related to a module
;; TODO: write abstract interface to decouple from TeXmacs/Guile/whatever
;; specifics.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (guile?)
  (with sd (scheme-dialect) (and (string? sd) (== "guile" (string-take sd 5))))
) ;define-public

(tm-define (is-real-module? module)
  (url-exists? (string->url (string-append (module->path module) ".scm")))
) ;tm-define

(define (module-source-path module full?)
  (string-concatenate (list (if full? (url-concretize "$TEXMACS_PATH/progs/") "")
                        (string-join (map symbol->string module) "/")
                        ".scm"
                      ) ;list
  ) ;string-concatenate
) ;define

(define (module-doc-path module)
  (string-append "tmfs://apidoc/type=module&what=" (module->string module))
) ;define

(tm-define ($module-source-link module)
  ($link (module-source-path module #t) (module-source-path module #f))
) ;tm-define

(tm-define ($module-doc-link module)
  ($link (module-doc-path module) (module->string module))
) ;tm-define

(tm-define (module-dependencies module) '())

(tm-define ($module-dependencies module)
  (cons 'concat
    (list-intersperse (map $module-doc-link (module-dependencies module)) ", ")
  ) ;cons
) ;tm-define

(define (module-description m)
  "Description TO-DO"
) ;define

(define module-exported-cache (make-ahash-table))

(define-public macro-keywords
  '(define-macro define-public-macro tm-define-macro)
) ;define-public

(define-public def-keywords
  `(define-public provide-public
     tm-define
     tm-menu
     menu-bind
     tm-widget
     ,@macro-keywords)
) ;define-public

(define (parse-form form f)
  "Set symbol properties and return the symbol."
  (and (pair? form)
    (member (car form) def-keywords)
    (let* ((l (source-property form 'line))
           (c (source-property form 'column))
           (sym (if (pair? (cadr form)) (caadr form) (cadr form)))
          ) ;
      (and (symbol? sym)
        (with old
          (or (symbol-property sym 'defs) '())
          (if (not (member `(,f ,l ,c) old))
            (set-symbol-property! sym 'defs (cons `(,f ,l ,c) old))
          ) ;if
          sym
        ) ;with
      ) ;and
    ) ;let*
  ) ;and
) ;define

(tm-define (module-exported module)
  (:synopsis "List of exported symbols in @module")
  (or (ahash-ref module-exported-cache module)
    (and (is-real-module? module)
      (let* ((fname (module-source-path module #t))
             (p (open-input-string (string-load fname)))
             (defs '())
             (add (lambda (f)
                    (with pf (parse-form f fname) (and (!= pf #f) (set! defs (rcons defs pf))))
                  ) ;lambda
             ) ;add
            ) ;
        (letrec ((r (lambda () (with form (read p) (or (eof-object? form) (begin (add form) (r)))))
                 ) ;r
                ) ;
          (r)
        ) ;letrec
        (ahash-set! module-exported-cache module defs)
      ) ;let*
    ) ;and
    '()
  ) ;or
) ;tm-define

(tm-define (module-count-exported module) (length (module-exported module)))

(tm-define (module-count-undocumented module)
  (with l
    (module-exported module)
    (- (length l)
      (length (list-filter l
                (lambda (x)
                  (and (symbol? x) (persistent-has? (doc-scm-cache) (symbol->string x)))
                ) ;lambda
              ) ;list-filter
      ) ;length
    ) ;-
  ) ;with
) ;tm-define

(tm-define ($doc-module-exported module)
  (with l
    (module-exported module)
    (with fun
      (lambda (sym)
        (if (symbol? sym) (list ($doc-explain-scm* (symbol->string sym))) '())
      ) ;lambda
      (if (null? l)
        `(document ,(replace "No symbols exported"))
        `(document (subsection ,(replace "Symbol documentation"))
           ,@(append-map fun l))
      ) ;if
    ) ;with
  ) ;with
) ;tm-define

(define (tm-exported? sym)
  (and (symbol? sym) (ahash-ref tm-defined-table sym))
) ;define

(define (dir-with-access? path)
  (url-test? path "dx")
) ;define

(define (list-submodules module)
  (with full
    (module->path module)
    (if (not (dir-with-access? full))
      '()
      (let* ((list-1 (url->list (url-expand (url-complete (url-append full (url-wildcard "*")) "r")))
             ) ;list-1
             (list-2 (map (lambda (u)
                            (cond ((string-ends? (url->system u) ".scm")
                                   (string->symbol (string-drop-right (url->system (url-tail u)) 4))
                                  ) ;
                                  ((dir-with-access? (url->system u)) (string->symbol (url->system (url-tail u))))
                                  (else '())
                            ) ;cond
                          ) ;lambda
                       list-1
                     ) ;map
             ) ;list-2
             (list-3 (filter (lambda (s) (nnull? s)) list-2))
             (list-4 (map (lambda (s) (rcons module s)) list-3))
            ) ;
        list-4
      ) ;let*
    ) ;if
  ) ;with
) ;define

(tm-define (list-submodules-recursive ml)
  (:synopsis "Return all submodules, recursively, for module list @ml")
  (cond ((null? ml) '())
        ((npair? ml)
         (if (is-real-module? ml)
           (list ml)
           (list-submodules-recursive (list-submodules ml))
         ) ;if
        ) ;
        ((null? (cdr ml))
         (if (is-real-module? (car ml))
           (list (car ml))
           (list-submodules-recursive (list-submodules (car ml)))
         ) ;if
        ) ;
        (else (if (is-real-module? (car ml))
                (cons (car ml) (list-submodules-recursive (cdr ml)))
                (append (list-submodules-recursive (list-submodules (car ml)))
                  (list-submodules-recursive (cdr ml))
                ) ;append
              ) ;if
        ) ;else
  ) ;cond
) ;tm-define

(define ($doc-module-branch m)
  `(branch ,(symbol->string (cAr m)) ,(module-doc-path m))
) ;define

(define ($doc-module-branches lst)
  (append-map (lambda (m) (list ($doc-module-branch m))) lst)
) ;define

(tm-define ($doc-module-traverse root)
  `(traverse (document ,@($doc-module-branches (list-submodules root))))
) ;tm-define

(tm-define ($submodules->gtree m)
  (with fun
    (lambda (x) (symbol->string (cAr x)))
    `(tree ,(if (null? m) "()" (fun m)) ,@(map fun (list-submodules m)))
  ) ;with
) ;tm-define

(tm-define ($doc-module-header m)
  `(doc-module-header ,(module->string m) ,(module-description m))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Symbols documentation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (build-link s def)
  (with (file line column)
    def
    (if (and file line column)
      (let ((lno (number->string line)) (cno (number->string column)))
        `(action ,(string-append (url->system (url-tail file)) ":" lno)
           ,(string-append "(show-def \""
              file
              "\" "
              lno
              " "
              cno
              " \""
              (symbol->string s)
              "\")"))
      ) ;let
      ""
    ) ;if
  ) ;with
) ;define

(tm-define (show-def file line col w)
  (:secure #t)
  (load-buffer-in-new-window file)
  (go-to-line line)
  (select-line)
  (select-word w (path->tree (selection-path)) col)
) ;tm-define


(tm-define ($doc-symbol-properties sym)
  (with defs
    (or (symbol-property sym 'defs) '((#f #f #f)))
    `(concat ,@(list-intersperse (map (lambda (x) (build-link sym x))
                                   (reverse (list-remove-duplicates defs)))
                 " | "))
  ) ;with
) ;tm-define

(tm-define (doc-symbol-synopsis* sym)
  (with prop
    (property sym :synopsis)
    (if (list? prop) (car prop) (replace "No synopsis available"))
  ) ;with
) ;tm-define

(tm-define ($doc-symbol-code sym)
  ;; Changed because of bug 61989
  ;; `(folded-explain
  ;;  (document (with "color" "dark green" (em ,(replace "Definition..."))))
  `(document (with ,"font-series"
               ,"bold"
               ,"color"
               ,"dark green"
               (em ,(replace "Definition:")))
     (scm-code (document ,(cond ((and (tm-exported? sym)
                                   (procedure? (eval sym)))
                                 (object->string (procedure-sources (eval sym))))
                                ((and (defined? sym)
                                   (procedure? (eval sym))
                                   (procedure-source (eval sym)))
                                 =>
                                 object->string)
                                (else (replace "Symbol not found or not a procedure"))))))
) ;tm-define

(tm-define ($doc-symbol-template sym code? message)
  (with contents
    (cons message (if code? (list ($doc-symbol-code sym)) '()))
    `(explain (document (concat (scm ,(symbol->string sym))
                          (explain-synopsis ,(doc-symbol-synopsis* sym))))
       (document ,@contents))
  ) ;with
) ;tm-define

(tm-define ($doc-symbol-extra sym . docurl)
  ($inline '(htab "")
    (if (nnull? docurl)
      ($inline ($ismall ($link (car docurl) (replace "Open doc."))) " | ")
      ""
    ) ;if
    ($ismall (replace "Go to") " " ($doc-symbol-properties sym))
  ) ;$inline
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Retrieval and display of documentation from the cache
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (docgrep-new-window what)
  (let* ((query (list->query (list (cons "type" "doc") (cons "what" what))))
         (name (string-append "tmfs://grep/" query))
        ) ;
    (buffer-load name)
    (open-buffer-in-window name (buffer-get name) "")
  ) ;let*
) ;define

(tm-define (docgrep-in-doc-secure what)
  (:synopsis "Search in documentation. Secure routine to use in 'action tags")
  (:secure #t)
  (docgrep-new-window what)
) ;tm-define

(define ($explain-scheme-not-found key)
  `(document ,($doc-symbol-template (string->symbol key)
                #t
                `(concat ,"Documentation unavailable. Search "
                   (action ,"the manual"
                     ,(string-append "(docgrep-in-doc-secure \"" key "\")"))
                   ,", or go to the definition in "
                   ,($doc-symbol-properties (string->symbol key)))))
) ;define

(define (doc-explain-sub entries scheme?)
  (if (or (nlist? entries) (null? entries) (not (func? (car entries) 'entry)))
    '()
    (with (key lan url doc)
      (cdar entries)
      (cons (if scheme?
              `(explain ,(tm-ref doc 0)
                 (document ,(tm-ref doc 1)
                   ,($doc-symbol-code (string->symbol key))
                   ,($doc-symbol-extra (string->symbol key) url)))
              `(explain ,(tm-ref doc 0) (document ,(tm-ref doc 1)))
            ) ;if
        (doc-explain-sub (cdr entries) scheme?)
      ) ;cons
    ) ;with
  ) ;if
) ;define

(tm-define ($doc-explain-scm* key)
  (with docs
    (doc-retrieve (doc-scm-cache) key (get-output-language))
    (if (null? docs)
      ($explain-scheme-not-found key)
      `(document ,@(doc-explain-sub docs #t))
    ) ;if
  ) ;with
) ;tm-define

(tm-define ($doc-explain-scm key)
  (:synopsis "Return a document with the scheme documentation for @key")
  `(document ,($doc-explain-scm* key)
     (freeze (concat (locus (id "__doc__popup__") ""))))
) ;tm-define

(define ($explain-macro-not-found key)
  `(document ,($doc-symbol-template (string->symbol key)
                #f
                `(concat ,"Documentation unavailable. You may search "
                   (action ,"the manual"
                     ,(string-append "(docgrep-in-doc-secure \"" key "\")"))
                   ,".")))
) ;define

(tm-define ($doc-explain-macro* key)
  (with docs
    (doc-retrieve (doc-macro-cache) key (get-output-language))
    (if (null? docs)
      ($explain-macro-not-found key)
      `(document ,@(doc-explain-sub docs #f))
    ) ;if
  ) ;with
) ;tm-define

(tm-define ($doc-explain-macro key)
  (:synopsis "Return a document with the documentation for macro @key")
  `(document ,($doc-explain-macro* key)
     (freeze (concat (locus (id "__doc__popup__") ""))))
) ;tm-define
