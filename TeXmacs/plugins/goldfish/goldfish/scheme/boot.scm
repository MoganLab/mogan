(unless (defined? 'texmacs-module)
  (set! (*s7* 'scheme-version) 'r7rs))

(define (file-exists? path)
  (if (string? path)
    (if (not (g_access path 0)) ; F_OK
      #f
      (if (g_access path 1) ; R_OK
          #t
          (error 'permission-error (string-append "No permission: " path))))
    (error 'type-error "(file-exists? path): path should be string")))

(define (delete-file path)
  (if (not (string? path))
    (error 'type-error "(delete-file path): path should be string")
    (if (not (file-exists? path))
      (error 'read-error (string-append path " does not exist"))
      (g_delete-file path))))

; 0-clause BSD
; Adapted from S7 Scheme's r7rs.scm
(define-macro (define-library libname . body) ; |(lib name)| -> environment
  `(define ,(symbol (object->string libname))
     (with-let (sublet (unlet)
                 (cons 'import import)
                 (cons '*export* ())
                 (cons 'export (define-macro (,(gensym) . names)
                                 `(set! *export* (append ',names *export*)))))
       ,@body
       (apply inlet
              (map (lambda (entry)
                     (if (or (member (car entry) '(*export* export import))
                             (and (pair? *export*)
                                  (not (member (car entry) *export*))))
                         (values)
                         entry))
                   (curlet))))))

(unless (defined? 'r7rs-import-library-filename)
  (define (r7rs-import-library-filename libs)
    (when (pair? libs)
      (let ((lib-filename (let loop ((lib (if (memq (caar libs) '(only except prefix rename))
                                              (cadar libs)
                                              (car libs)))
                                     (name ""))
                            (set! name (string-append name (symbol->string (car lib))))
                            (if (null? (cdr lib))
                                (string-append name ".scm")
                                (begin
                                  (set! name (string-append name "/"))
                                  (loop (cdr lib) name))))))
        (when (not (defined? (symbol (object->string (car libs)))))
          ;(display "Loading ") (display lib-filename) (newline)
          (load lib-filename))
        (r7rs-import-library-filename (cdr libs)))))
  )

(define-macro (import . libs)
  `(begin
     (r7rs-import-library-filename ',libs)
     (varlet (curlet)
       ,@(map (lambda (lib)
                (case (car lib)
                  ((only)
                   `((lambda (e names)
                       (apply inlet
                              (map (lambda (name)
                                     (cons name (e name)))
                                   names)))
                     (symbol->value (symbol (object->string (cadr ',lib))))
                     (cddr ',lib)))
                  ((except)
                   `((lambda (e names)
                       (apply inlet
                              (map (lambda (entry)
                                     (if (member (car entry) names)
                                         (values)
                                         entry))
                                   e)))
                     (symbol->value (symbol (object->string (cadr ',lib))))
                     (cddr ',lib)))
                  ((prefix)
                   `((lambda (e prefx)
                       (apply inlet
                              (map (lambda (entry)
                                     (cons (string->symbol 
                                            (string-append (symbol->string prefx) 
                                                           (symbol->string (car entry)))) 
                                           (cdr entry)))
                                   e)))
                     (symbol->value (symbol (object->string (cadr ',lib))))
                     (caddr ',lib)))
                  ((rename)
                   `((lambda (e names)
                       (apply inlet
                              (map (lambda (entry)
                                     (let ((info (assoc (car entry) names)))
                                       (if info
                                           (cons (cadr info) (cdr entry))
                                           entry))) 
                                   e)))
                     (symbol->value (symbol (object->string (cadr ',lib))))
                     (cddr ',lib)))
                  (else
                   `(let ((sym (symbol (object->string ',lib))))
                      (if (not (defined? sym))
                          (format () "~A not loaded~%" sym)
                          (symbol->value sym))))))
              libs))))

