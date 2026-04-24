(define-library (liii timeit)
  (export timeit)
  (import (liii base) (scheme time))
  (begin

    (define* (timeit stmt
               (setup '())
               (number 1000000)
             ) ;timeit
      (if (not (procedure? stmt))
        (error 'type-error
          "(timeit stmt setup number): stmt must be a procedure"
        ) ;error
      ) ;if
      (if (not (or (procedure? setup) (null? setup))
          ) ;not
        (error 'type-error
          "(timeit stmt setup number): setup must be a procedure or '()"
        ) ;error
      ) ;if
      (if (not (and (integer? number)
                 (positive? number)
               ) ;and
          ) ;not
        (error 'type-error
          "(timeit stmt setup number): number must be a positive integer"
        ) ;error
      ) ;if

      (unless (null? setup)
        (setup)
      ) ;unless

      (let ((start-time (current-second)))
        (do ((i 0 (+ i 1)))
          ((= i number))
          (stmt)
        ) ;do

        (- (current-second) start-time)
      ) ;let
    ) ;define*

  ) ;begin
) ;define-library
