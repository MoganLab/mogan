(define-library (liii sys)
  (export argv executable which)
  (import (scheme process-context))
  (begin
    (define (argv)
      (command-line)
    ) ;define

    (define (executable)
      (g_executable)
    ) ;define

    (define* (which cmd (path #f))
      (if path
        (g_which cmd path)
        (g_which cmd)
      ) ;if
    ) ;define

  ) ;begin
) ;define-library
