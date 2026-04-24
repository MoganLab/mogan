;; 0-clause BSD
;; Bill Schottstaedt
;; from S7 source repo: r7rs.scm

(define-library (srfi srfi-39)
  (export make-parameter parameterize)
  (begin

    ;; parameters
    ;;   s7 has no built-in parameter objects
    (define* (make-parameter init converter)
      (let* ((convert (or converter (lambda (x) x)))
             (old-values ())
             (value (convert init))
            ) ;
        (lambda () value)
      ) ;let*
    ) ;define*

    (define-macro (parameterize vars . body)
      `(dynamic-wind (lambda ,() ,@(map (lambda (var) `(with-let (funclet ,(car var)) (set! old-values (cons value old-values)) (set! value (convert ,(cadr var))))) vars)) (lambda ,() ,@body) (lambda ,() ,@(map (lambda (var) `(with-let (funclet ,(car var)) (set! value (car old-values)) (set! old-values (cdr old-values)))) vars)))
    ) ;define-macro

  ) ;begin
) ;define-library
