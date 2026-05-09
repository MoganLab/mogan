
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
  (:use (telemetry telemetry-track) (telemetry telemetry-utils))
) ;texmacs-module

(import (scheme base))

(define telemetry-scheduled? #f)

(define (telemetry-scheduler-step)
  (when (telemetry-enabled?)
    (telemetry-flush-if-needed)
  ) ;when
  (telemetry-delayed)
) ;define

(define (telemetry-delayed)
  (delayed (:pause (telemetry-get-flush-interval)) (telemetry-scheduler-step))
) ;define

(define-public (init-telemetry)
  (if telemetry-scheduled?
    (display "[telemetry] init: already initialized\n")
    (if (telemetry-enabled?)
      (begin
        (set! telemetry-scheduled? #t)
        (display (string-append "[telemetry] init: enabled, buffer="
                   (number->string (telemetry-get-buffer-size))
                   ", interval="
                   (number->string (telemetry-get-flush-interval))
                   "ms\n"
                 ) ;string-append
        ) ;display
        (on-exit (catch #t
                   (lambda () (telemetry-flush-if-needed))
                   (lambda args
                     (display (string-append "[telemetry] error: exit flush failed: "
                                (object->string args)
                                "\n"
                              ) ;string-append
                     ) ;display
                   ) ;lambda
                 ) ;catch
        ) ;on-exit
        (telemetry-delayed)
      ) ;begin
      (display "[telemetry] init: disabled\n")
    ) ;if
  ) ;if
) ;define-public
