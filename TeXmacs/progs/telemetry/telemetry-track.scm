
/******************************************************************************
 * MODULE     : telemetry-track.scm
 * DESCRIPTION: Telemetry event tracking with memory queue
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

(define-library (telemetry telemetry-track)
  (export track-event
    telemetry-buffer-size
    telemetry-flush-if-needed
  )
  (import (scheme base)
    (telemetry telemetry-utils)
    (telemetry telemetry-flush)
  )
  (begin

    (define telemetry-buffer-size 10)

    (define (track-event event-type properties)
      (if (and (string? event-type) (not (string-null? event-type)))
        (begin
          (set! *telemetry-event-queue*
            (cons (telemetry-make-event event-type properties)
                  *telemetry-event-queue*))
          (if (>= (length *telemetry-event-queue*) telemetry-buffer-size)
            (telemetry-flush))
          #t)
        #f))

    (define (telemetry-flush-if-needed)
      (if (not (null? *telemetry-event-queue*))
        (telemetry-flush)
        #t))

  ))
