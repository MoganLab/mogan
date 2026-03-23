; 0-clause BSD by Bill Schottstaedt from S7 source repo: s7test.scm
(define-library (srfi srfi-2)
  (export and-let*)
  (begin

    (define-macro (and-let* vars . body)
      `(let () (and ,@(map (lambda (v) `(define ,@v)) vars) (begin ,@body)))
    ) ;define-macro

  ) ;begin
) ;define-library

