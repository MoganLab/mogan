
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

(define telemetry-scheduled? #f)

(define (telemetry-scheduler-step)
  (when (telemetry-enabled?)
    (telemetry-flush-if-needed))
  (telemetry-delayed))

(define (telemetry-delayed)
  (delayed
    (:pause (telemetry-get-flush-interval))
    (telemetry-scheduler-step)))

(define-public (init-telemetry)
  (if telemetry-scheduled?
    (display "[telemetry] init: already initialized\n")
    (begin
      (let ((loaded (telemetry-load-config)))
        (if (not (null? loaded))
          (display (string-append "[telemetry] init: loaded config "
                                  (object->string loaded) "\n"))))
      (if (telemetry-enabled?)
        (begin
          (set! telemetry-scheduled? #t)
          (display (string-append "[telemetry] init: enabled, buffer="
                                  (number->string (telemetry-get-buffer-size))
                                  ", interval="
                                  (number->string (telemetry-get-flush-interval))
                                  "ms\n"))
          (on-exit
            (catch #t
              (lambda () (telemetry-flush-if-needed))
              (lambda args
                (display (string-append "[telemetry] error: exit flush failed: "
                                        (object->string args) "\n")))))
          (telemetry-delayed))
        (display "[telemetry] init: disabled\n")))))
