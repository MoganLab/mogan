
/******************************************************************************
 * MODULE     : telemetry-track.scm
 * DESCRIPTION: Telemetry event tracking with memory queue
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

(texmacs-module (telemetry telemetry-track)
  (:use (telemetry telemetry-utils)
        (telemetry telemetry-flush)))

(import (scheme base))

(define-public telemetry-buffer-size 10)

(define-public (track-event event-type properties)
  (if (and (string? event-type) (not (string-null? event-type)))
    (begin
      (set! *telemetry-event-queue*
        (cons (telemetry-make-event event-type properties)
              *telemetry-event-queue*))
      (if (>= (length *telemetry-event-queue*) telemetry-buffer-size)
        (telemetry-flush))
      #t)
    #f))

(define-public (telemetry-flush-if-needed)
  (if (not (null? *telemetry-event-queue*))
    (telemetry-flush)
    #t))
