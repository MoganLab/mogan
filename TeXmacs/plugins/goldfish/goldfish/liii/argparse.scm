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

    (define (normalize-list value)
      (cond ((not value) '())
            ((list? value) value)
            (else (list value))
      ) ;cond
    ) ;define

    (define (config-ref options key default)
      (alist-ref/default options key default)
    ) ;define

    (define (make-arg-record name type short-name default action)
      (list name type short-name default default action)
    ) ;define

    (define (arg-action record)
      (cadr (cddddr record))
    ) ;define

    (define (set-arg-current! record value)
      (set-car! (cddddr record) value)
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

    (define (arg-action? action)
      (unless (symbol? action)
        (type-error "action of the argument must be symbol")
      ) ;unless
      (member action '(store store-true store-false))
    ) ;define

    (define (default-value options action)
      (let ((found (assoc 'default options)))
        (if found
          (cdr found)
          (case action
           ((store-true) #f)
           ((store-false) #t)
           (else #f)
          ) ;case
        ) ;if
      ) ;let
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
             (action (alist-ref/default options 'action 'store))
             (default (default-value options action))
             (arg-record (make-arg-record name type short-name default action))
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
        (unless (arg-action? action)
          (value-error "Invalid action of the argument" action)
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

    (define (string-index-of str ch start)
      (let loop
        ((i start))
        (cond ((>= i (string-length str)) #f)
              ((char=? (string-ref str i) ch) i)
              (else (loop (+ i 1)))
        ) ;cond
      ) ;let
    ) ;define

    (define (split-option arg prefix-length)
      (let ((eq-pos (string-index-of arg #\= prefix-length)))
        (if eq-pos
          (cons (substring arg prefix-length eq-pos)
            (substring arg (+ eq-pos 1) (string-length arg))
          ) ;cons
          (cons (substring arg prefix-length (string-length arg)) #f)
        ) ;if
      ) ;let
    ) ;define

    (define (retrieve-args args)
      (if (null? args) (cddr (argv)) (car args))
    ) ;define

    (define (retrieve-argv args)
      (if (null? args) (argv) (car args))
    ) ;define

    (define (command-arg? options arg)
      (member arg (normalize-list (config-ref options 'command '())))
    ) ;define

    (define (skip-value-option? options arg)
      (member arg (normalize-list (config-ref options 'skip-value-options '())))
    ) ;define

    (define (skip-prefix-option? options arg)
      (let loop
        ((prefixes (normalize-list (config-ref options 'skip-prefix-options '()))))
        (cond ((null? prefixes) #f)
              ((string-starts? arg (car prefixes)) #t)
              (else (loop (cdr prefixes)))
        ) ;cond
      ) ;let
    ) ;define

    (define (unknown-options options)
      (config-ref options 'unknown-options 'error)
    ) ;define

    (define (handle-unknown-option options arg positionals)
      (case (unknown-options options)
            ((ignore) positionals)
            ((positional) (cons arg positionals))
            (else (value-error (string-append "Unknown option: " arg)))
      ) ;case
    ) ;define

    (define (parse-option args-ht options args prefix-length positionals loop)
      (let* ((arg (car args))
             (split (split-option arg prefix-length))
             (name (car split))
             (inline-value (cdr split))
             (found (hash-table-ref/default args-ht name #f))
            ) ;
        (if found
          (case (arg-action found)
                ((store-true)
                 (when inline-value
                   (error "Flag does not take a value" name)
                 ) ;when
                 (set-arg-current! found #t)
                 (loop (cdr args) positionals)
                ) ;
                ((store-false)
                 (when inline-value
                   (error "Flag does not take a value" name)
                 ) ;when
                 (set-arg-current! found #f)
                 (loop (cdr args) positionals)
                ) ;
                (else (let ((value (if inline-value
                                     inline-value
                                     (if (null? (cdr args)) (error "Missing value for argument" name) (cadr args))
                                   ) ;if
                            ) ;value
                           ) ;
                        (set-arg-current! found (convert-value value (cadr found)))
                        (loop (if inline-value (cdr args) (cddr args)) positionals)
                      ) ;let
                ) ;else
          ) ;case
          (loop (cdr args) (handle-unknown-option options arg positionals))
        ) ;if
      ) ;let*
    ) ;define

    (define (%parse-args args-ht options raw-args)
      (let loop
        ((args raw-args) (positionals '()))
        (if (null? args)
          (reverse positionals)
          (let ((arg (car args)))
            (cond ((command-arg? options arg) (loop (cdr args) positionals))
                  ((skip-value-option? options arg)
                   (loop (if (null? (cdr args)) '() (cddr args)) positionals)
                  ) ;
                  ((skip-prefix-option? options arg) (loop (cdr args) positionals))
                  ((long-form? arg) (parse-option args-ht options args 2 positionals loop))
                  ((short-form? arg) (parse-option args-ht options args 1 positionals loop))
                  (else (loop (cdr args) (cons arg positionals)))
            ) ;cond
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (%parse-program-args args-ht options args)
      (%parse-args args-ht options (retrieve-args args))
    ) ;define

    (define (%parse-argv args-ht options args)
      (let ((full-argv (retrieve-argv args)))
        (%parse-args args-ht options (if (null? full-argv) '() (cdr full-argv)))
      ) ;let
    ) ;define

    (define (make-argument-parser . maybe-options)
      (let ((args-ht (make-hash-table))
            (options (if (null? maybe-options) '() (car maybe-options)))
            (positionals '())
           ) ;
        (lambda (command . args)
          (case command
           ((:add) (%add-argument args-ht args))
           ((:add-argument) (%add-argument args-ht args))
           ((:get) (%get-argument args-ht args))
           ((:get-argument) (%get-argument args-ht args))
           ((:parse) (set! positionals (%parse-program-args args-ht options args)) args-ht)
           ((:parse-args)
            (set! positionals (%parse-program-args args-ht options args))
            args-ht
           ) ;
           ((:parse-argv) (set! positionals (%parse-argv args-ht options args)) args-ht)
           ((:positionals) positionals)
           ((:get-positionals) positionals)
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
