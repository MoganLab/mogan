(unless (defined? 'texmacs-module)
  (set! (*s7* 'scheme-version) 'r7rs)
) ;unless

(define (file-exists? path)
  (if (string? path)
    (if (not (g_access path 0))
      #f
      (if (g_access path 1)
        #t
        (error 'permission-error
          (string-append "No permission: " path)
        ) ;error
      ) ;if
    ) ;if
    (error 'type-error
      "(file-exists? path): path should be string"
    ) ;error
  ) ;if
) ;define

(define (delete-file path)
  (if (not (string? path))
    (error 'type-error
      "(delete-file path): path should be string"
    ) ;error
    (if (not (file-exists? path))
      (error 'read-error
        (string-append path " does not exist")
      ) ;error
      (g_delete-file path)
    ) ;if
  ) ;if
) ;define

(define-macro (define-library libname . body)
  `(define ,(symbol (object->string libname)) (with-let (sublet (unlet) (cons (#_quote import) import) (cons (#_quote *export*) ()) (cons (#_quote export) (define-macro (,(gensym) . names) (#_list-values (#_quote set!) (#_quote *export*) (#_list-values (#_quote append) (#_list-values #_quote names) (#_quote *export*)))))) ,@body (apply inlet (map (lambda (entry) (if (or (member (car entry) (#_quote (*export* export import))) (and (pair? *export*) (not (member (car entry) *export*)))) (values) entry)) (curlet)))))
) ;define-macro

(unless (defined? 'r7rs-import-library-filename)
  (define (r7rs-import-library-filename libs)
    (when (pair? libs)
      (let ((lib-filename (let loop
                            ((lib (if (memq (caar libs)
                                        '(only except prefix rename)
                                      ) ;memq
                                    (cadar libs)
                                    (car libs)
                                  ) ;if
                             ) ;lib
                             (name "")
                            ) ;
                            (set! name
                              (string-append name
                                (symbol->string (car lib))
                              ) ;string-append
                            ) ;set!
                            (if (null? (cdr lib))
                              (string-append name ".scm")
                              (begin
                                (set! name (string-append name "/"))
                                (loop (cdr lib) name)
                              ) ;begin
                            ) ;if
                          ) ;let
            ) ;lib-filename
           ) ;
        (when (not (defined? (symbol (object->string (car libs)))
                   ) ;defined?
              ) ;not
          (load lib-filename)
        ) ;when
        (r7rs-import-library-filename (cdr libs)
        ) ;r7rs-import-library-filename
      ) ;let
    ) ;when
  ) ;define
) ;unless

(define-macro (import . libs)
  `(begin (r7rs-import-library-filename (quote ,libs)) (varlet (curlet) ,@(map (lambda (lib) (case (car lib) ((only) `((lambda (e names) (apply inlet (map (lambda (name) (cons name (e name))) names))) (symbol->value (symbol (object->string (cadr (quote ,lib))))) (cddr (quote ,lib)))) ((except) `((lambda (e names) (apply inlet (map (lambda (entry) (if (member (car entry) names) (values) entry)) e))) (symbol->value (symbol (object->string (cadr (quote ,lib))))) (cddr (quote ,lib)))) ((prefix) `((lambda (e prefx) (apply inlet (map (lambda (entry) (cons (string->symbol (string-append (symbol->string prefx) (symbol->string (car entry)))) (cdr entry))) e))) (symbol->value (symbol (object->string (cadr (quote ,lib))))) (caddr (quote ,lib)))) ((rename) `((lambda (e names) (apply inlet (map (lambda (entry) (let ((info (assoc (car entry) names))) (if info (cons (cadr info) (cdr entry)) entry))) e))) (symbol->value (symbol (object->string (cadr (quote ,lib))))) (cddr (quote ,lib)))) (else `(let ((sym (symbol (object->string (quote ,lib))))) (if (not (defined? sym)) (format () "~A not loaded~%" sym) (symbol->value sym)))))) libs)))
) ;define-macro
