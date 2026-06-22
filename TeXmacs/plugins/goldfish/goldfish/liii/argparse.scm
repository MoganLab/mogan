(define-library (liii argparse)
  (import (liii base)
    (liii error)
    (liii list)
    (liii string)
    (liii hash-table)
    (liii alist)
    (liii sys)
  ) ;import
  (export make-argument-parser)
  (begin

    (define (make-arg-record name type short-name default)
      (list name type short-name default default)
    ) ;define

    (define (convert-value value type)
      (case type
       ((number)
        (if (number? value)
          value
          (let ((num (string->number value)))
            (if num num (error "Invalid number format" value))
          ) ;let
        ) ;if
       ) ;
       ((string) (if (string? value) value (error "Value is not a string")))
       (else (error "Unsupported type" type))
      ) ;case
    ) ;define

    (define (arg-type? type)
      (unless (symbol? type)
        (type-error "type of the argument must be symbol")
      ) ;unless
      (member type '(string number))
    ) ;define

    (define (%add-argument args-ht args)
      (let* ((options (car args))
             (name (alist-ref options
                     'name
                     (lambda () (value-error "name is required for an option"))
                   ) ;alist-ref
             ) ;name
             (type (alist-ref/default options 'type 'string))
             (short-name (alist-ref/default options 'short #f))
             (default (alist-ref/default options 'default #f))
             (arg-record (make-arg-record name type short-name default))
            ) ;
        (unless (string? name)
          (type-error "name of the argument must be string")
        ) ;unless
        (unless (arg-type? type)
          (value-error "Invalid type of the argument" type)
        ) ;unless
        (unless (or (not short-name) (string? short-name))
          (type-error "short name of the argument must be string if given")
        ) ;unless
        (hash-table-set! args-ht name arg-record)
        (when short-name
          (hash-table-set! args-ht short-name arg-record)
        ) ;when
      ) ;let*
    ) ;define

    (define (%get-argument args-ht args)
      (let ((found (hash-table-ref/default args-ht (car args) #f)))
        (if found (fifth found) (error "Argument not found" (car args)))
      ) ;let
    ) ;define

    (define (long-form? arg)
      (and (string? arg) (>= (string-length arg) 3) (string-starts? arg "--"))
    ) ;define

    (define (short-form? arg)
      (and (string? arg) (>= (string-length arg) 2) (char=? (string-ref arg 0) #\-))
    ) ;define

    (define (retrieve-args args)
      (if (null? args) (cddr (argv)) (car args))
    ) ;define

    (define (%parse-args args-ht prog-args)
      (let loop
        ((args (retrieve-args prog-args)))
        (if (null? args)
          args-ht
          (let ((arg (car args)))
            (cond ((long-form? arg)
                   (let* ((name (substring arg 2)) (found (hash-table-ref args-ht name)))
                     (if found
                       (if (null? (cdr args))
                         (error "Missing value for argument" name)
                         (begin
                           (let ((value (convert-value (cadr args) (cadr found))))
                             (set-car! (cddddr found) value)
                           ) ;let
                           (loop (cddr args))
                         ) ;begin
                       ) ;if
                       (value-error (string-append "Unknown option: --" name))
                     ) ;if
                   ) ;let*
                  ) ;
                  ((short-form? arg)
                   (let* ((name (substring arg 1)) (found (hash-table-ref args-ht name)))
                     (if found
                       (if (null? (cdr args))
                         (error "Missing value for argument" name)
                         (begin
                           (let ((value (convert-value (cadr args) (cadr found))))
                             (set-car! (cddddr found) value)
                           ) ;let
                           (loop (cddr args))
                         ) ;begin
                       ) ;if
                       (value-error (string-append "Unknown option: -" name))
                     ) ;if
                   ) ;let*
                  ) ;
                  (else (loop (cdr args)))
            ) ;cond
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (make-argument-parser)
      (let ((args-ht (make-hash-table)))
        (lambda (command . args)
          (case command
           ((:add) (%add-argument args-ht args))
           ((:add-argument) (%add-argument args-ht args))
           ((:get) (%get-argument args-ht args))
           ((:get-argument) (%get-argument args-ht args))
           ((:parse) (%parse-args args-ht args))
           ((:parse-args) (%parse-args args-ht args))
           (else (if (and (null? args) (symbol? command))
                   (%get-argument args-ht (list (symbol->string command)))
                   (error "Unknown parser command" command)
                 ) ;if
           ) ;else
          ) ;case
        ) ;lambda
      ) ;let
    ) ;define
  ) ;begin
) ;define-library
