(define-library (liii uuid)
  (export uuid4)
  (begin

    (define (uuid4)
      (g_uuid4)
    ) ;define

  ) ;begin
) ;define-library
