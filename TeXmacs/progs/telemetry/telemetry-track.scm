
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : telemetry-track.scm
;; DESCRIPTION : Telemetry event tracking with memory queue
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (telemetry telemetry-track)
  (:use (telemetry telemetry-utils)
        (telemetry telemetry-flush)))

(import (scheme base))

(define-public telemetry-buffer-size 10)

(define-public (track-event event-type properties)
  (if (and (string? event-type) (not (string-null? event-type)))
    (begin
      (display (string-append "[telemetry] scheme track: " event-type "\n"))
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
