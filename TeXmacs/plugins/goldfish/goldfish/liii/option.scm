(define-library (liii option)
  (import (liii base))
  (export none
    option
    option?
    option-map
    option-filter
    option-flat-map
    option-for-each
    option-get
    option-get-or-else
    option=?
    option-or-else
    option-defined?
    option-empty?
    option-every
    option-any
  ) ;export
  (begin

    (define (none)
      (cons #f 'N)
    ) ;define

    (define (option value)
      (cons value 'S)
    ) ;define

    (define (option? x)
      (and (pair? x)
        (or (eq? (cdr x) 'N) (eq? (cdr x) 'S))
      ) ;and
    ) ;define

    (define (option-empty? opt)
      (eq? (cdr opt) 'N)
    ) ;define

    (define (option-defined? opt)
      (eq? (cdr opt) 'S)
    ) ;define

    (define (option-map f opt)
      (if (option-empty? opt)
        (none)
        (option (f (car opt)))
      ) ;if
    ) ;define

    (define (option-filter pred opt)
      (if (or (option-empty? opt)
            (not (pred (car opt)))
          ) ;or
        (none)
        opt
      ) ;if
    ) ;define

    (define (option-flat-map f opt)
      (if (option-empty? opt)
        (none)
        (f (car opt))
      ) ;if
    ) ;define

    (define (option-for-each f opt)
      (when (option-defined? opt)
        (f (car opt))
      ) ;when
    ) ;define

    (define (option-get opt)
      (if (option-empty? opt)
        (error "option is empty, cannot get value"
        ) ;error
        (car opt)
      ) ;if
    ) ;define

    (define (option-get-or-else default opt)
      (if (option-empty? opt)
        (if (procedure? default)
          (default)
          default
        ) ;if
        (car opt)
      ) ;if
    ) ;define

    (define (option-or-else alt opt)
      (if (option-empty? opt) alt opt)
    ) ;define

    (define (option=? opt1 opt2)
      (cond ((and (option-empty? opt1)
               (option-empty? opt2)
             ) ;and
             #t
            ) ;
            ((or (option-empty? opt1)
               (option-empty? opt2)
             ) ;or
             #f
            ) ;
            (else (equal? (car opt1) (car opt2)))
      ) ;cond
    ) ;define

    (define (option-every pred opt)
      (if (option-empty? opt)
        #f
        (pred (car opt))
      ) ;if
    ) ;define

    (define (option-any pred opt)
      (if (option-empty? opt)
        #f
        (pred (car opt))
      ) ;if
    ) ;define

  ) ;begin
) ;define-library
