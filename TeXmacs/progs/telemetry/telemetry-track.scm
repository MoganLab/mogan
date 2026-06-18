
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : telemetry-track.scm
;; DESCRIPTION : Telemetry event tracking with memory queue and flush
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (telemetry telemetry-track) (:use (telemetry telemetry-utils)))

(import (scheme base)
  (liii base)
  (liii os)
  (liii path)
  (liii string)
  (liii list)
) ;import

(define-public *telemetry-event-queue* '())

(define-public (track-event event-type properties)
  (if (not (telemetry-enabled?))
    #f
    (if (and (string? event-type) (not (string-null? event-type)))
      (begin
        (set! *telemetry-event-queue*
          (cons (telemetry-make-event event-type properties) *telemetry-event-queue*)
        ) ;set!
        (let ((len (length *telemetry-event-queue*)))
          (display (string-append "[telemetry] track: "
                     event-type
                     " (queue: "
                     (number->string len)
                     "/"
                     (number->string (telemetry-get-buffer-size))
                     ")\n"
                   ) ;string-append
          ) ;display
          (if (> len telemetry-max-queue-size)
            (begin
              (set! *telemetry-event-queue*
                (list-head *telemetry-event-queue* telemetry-max-queue-size)
              ) ;set!
              (display (string-append "[telemetry] warn: queue truncated to "
                         (number->string telemetry-max-queue-size)
                         "\n"
                       ) ;string-append
              ) ;display
            ) ;begin
          ) ;if
          (if (>= len (telemetry-get-buffer-size)) (telemetry-flush))
        ) ;let
        #t
      ) ;begin
      #f
    ) ;if
  ) ;if
) ;define-public

(define-public (telemetry-queue-length) (length *telemetry-event-queue*))

(define-public (telemetry-flush-if-needed)
  (if (not (telemetry-enabled?))
    #t
    (if (not (null? *telemetry-event-queue*)) (telemetry-flush) #t)
  ) ;if
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Flush implementation: independent jsonl files + atomic meta update
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (telemetry-write-pending events)
  (if (null? events)
    #t
    (let* ((filename (telemetry-generate-filename))
           (filepath (telemetry-full-path filename))
           (lines (map telemetry->json events))
          ) ;
      (catch #t
        (lambda ()
          (let ((text (string-append (string-join lines "\n") "\n")))
            (string-save text (system->url filepath))
            (if (telemetry-meta-add-entry filename)
              (begin
                (display (string-append "[telemetry] flush: "
                           (number->string (length events))
                           " events -> "
                           filename
                           "\n"
                         ) ;string-append
                ) ;display
                #t
              ) ;begin
              (begin
                (display (string-append "[telemetry] error: meta update failed for " filename "\n")
                ) ;display
                #f
              ) ;begin
            ) ;if
          ) ;let
        ) ;lambda
        (lambda args
          (display (string-append "[telemetry] error: write failed: " (object->string args) "\n")
          ) ;display
          #f
        ) ;lambda
      ) ;catch
    ) ;let*
  ) ;if
) ;define-public

(define-public (telemetry-flush)
  (if (null? *telemetry-event-queue*)
    #t
    (let ((ok? (telemetry-write-pending (reverse *telemetry-event-queue*))))
      (if ok?
        (begin
          (set! *telemetry-event-queue* '())
          #t
        ) ;begin
        (begin
          (display (string-append "[telemetry] error: flush failed, keeping "
                     (number->string (length *telemetry-event-queue*))
                     " events in memory queue\n"
                   ) ;string-append
          ) ;display
          #f
        ) ;begin
      ) ;if
    ) ;let
  ) ;if
) ;define-public
