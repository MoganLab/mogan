(define-library (liii chez)
  (export atom?)
  (begin

    (define (atom? x)
      (not (pair? x))
    ) ;define
  ) ;begin
) ;define-library
