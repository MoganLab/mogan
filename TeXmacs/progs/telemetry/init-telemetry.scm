
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-telemetry.scm
;; DESCRIPTION : Telemetry initialization and periodic flush
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (telemetry init-telemetry)
  (:use (telemetry telemetry-track)
        (telemetry telemetry-utils)))

(import (scheme base))

(define telemetry-flush-interval-ms 60000)
(define telemetry-scheduled? #f)

(define (telemetry-scheduler-step)
  (when (telemetry-enabled?)
    (telemetry-flush-if-needed)
    (telemetry-delayed)))

(define (telemetry-delayed)
  (delayed
    (:pause telemetry-flush-interval-ms)
    (telemetry-scheduler-step)))

(define-public (init-telemetry)
  (if telemetry-scheduled?
    (display "Telemetry already initialized\n")
    (if (telemetry-enabled?)
      (begin
        (set! telemetry-scheduled? #t)
        (display "Telemetry enabled\n")
        (on-exit (telemetry-flush-if-needed))
        (telemetry-delayed))
      (display "Telemetry disabled\n"))))
