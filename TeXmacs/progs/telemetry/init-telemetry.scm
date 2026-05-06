
/******************************************************************************
 * MODULE     : init-telemetry.scm
 * DESCRIPTION: Telemetry initialization and periodic flush
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

(define-library (telemetry init-telemetry)
  (export init-telemetry
    telemetry-enabled?
  )
  (import (scheme base)
    (telemetry telemetry-track)
    (telemetry telemetry-flush)
  )
  (begin

    (define telemetry-flush-interval-ms 60000)
    (define telemetry-scheduled? #f)

    (define (telemetry-enabled?)
      (let ((autosave (get-preference "autosave")))
        (not (string=? autosave "0"))))

    (define (telemetry-now)
      (when (telemetry-enabled?)
        (telemetry-flush-if-needed)
        (telemetry-delayed)))

    (define (telemetry-delayed)
      (delayed
        (:pause telemetry-flush-interval-ms)
        (telemetry-now)))

    (define (init-telemetry)
      (if (telemetry-enabled?)
        (begin
          (display "Telemetry enabled\n")
          (telemetry-delayed))
        (display "Telemetry disabled\n")))

  ))
