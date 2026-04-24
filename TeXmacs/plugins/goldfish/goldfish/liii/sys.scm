(define-library (liii sys)
  (export argv executable)
  (import (scheme process-context))
  (begin
    (define (argv)
      (command-line)
    ) ;define

    (define (executable)
      (g_executable)
    ) ;define

  ) ;begin
) ;define-library
