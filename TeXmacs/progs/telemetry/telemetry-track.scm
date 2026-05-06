
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

(texmacs-module (telemetry telemetry-track)
  (:use (telemetry telemetry-utils)))

(import (scheme base)
  (liii base)
  (liii json)
  (liii os)
  (liii path)
  (liii string)
  (liii list)
)

(define-public telemetry-buffer-size 10)
(define-public telemetry-max-queue-size 1000)
(define-public *telemetry-event-queue* '())

(define-public (track-event event-type properties)
  (if (not (telemetry-enabled?))
    #f
    (if (and (string? event-type) (not (string-null? event-type)))
      (begin
        (display (string-append "[telemetry] scheme track: " event-type "\n"))
        (set! *telemetry-event-queue*
          (cons (telemetry-make-event event-type properties)
                *telemetry-event-queue*))
        (let ((len (length *telemetry-event-queue*)))
          (if (> len telemetry-max-queue-size)
            (set! *telemetry-event-queue*
              (list-head *telemetry-event-queue* telemetry-max-queue-size)))
          (if (>= len telemetry-buffer-size)
            (telemetry-flush)))
        #t)
      #f)))

(define-public (telemetry-flush-if-needed)
  (if (not (telemetry-enabled?))
    #t
    (if (not (null? *telemetry-event-queue*))
      (telemetry-flush)
      #t)))

;; ---------------------------------------------------------------------------
;; Flush implementation (merged from telemetry-flush to share mutable state)
;; ---------------------------------------------------------------------------

(define telemetry-lock-timeout-seconds 30)

(define (telemetry-lock-info owner now)
  `(("owner" . ,owner) ("created_at" . ,now)))

(define (telemetry-read-lock-info)
  (catch #t
    (lambda ()
      (let ((text (string-load (telemetry-lock-info-path))))
        (if (and (string? text) (> (string-length text) 0))
          (string->json text)
          #f)))
    (lambda args #f)))

(define (telemetry-lock-expired? now)
  (let ((info (telemetry-read-lock-info)))
    (if info
      (let ((created (json-ref-number info "created_at" 0)))
        (> (- now created) telemetry-lock-timeout-seconds))
      #t)))

(define (telemetry-remove-lock)
  (catch #t
    (lambda ()
      (path-unlink (telemetry-lock-info-path) #t)
      (rmdir (telemetry-lock-path)))
    (lambda args #f)))

(define (telemetry-acquire-lock)
  (telemetry-ensure-dir)
  (let ((owner (telemetry-lock-owner))
        (now (inexact->exact (truncate (current-time)))))
    (catch #t
      (lambda ()
        (mkdir (telemetry-lock-path))
        (string-save
          (json->string (telemetry-lock-info owner now))
          (telemetry-lock-info-path))
        owner)
      (lambda args
        (if (telemetry-lock-expired? now)
          (begin
            (telemetry-remove-lock)
            (catch #t
              (lambda ()
                (mkdir (telemetry-lock-path))
                (string-save
                  (json->string (telemetry-lock-info owner now))
                  (telemetry-lock-info-path))
                owner)
              (lambda args2 #f)))
          #f)))))

(define (telemetry-release-lock owner)
  (let ((info (telemetry-read-lock-info)))
    (if (and info
          (string=? (json-ref-string info "owner" "") owner))
      (telemetry-remove-lock)
      #f)))

(define (telemetry-write-pending events)
  (if (null? events)
    #t
    (let ((path (telemetry-pending-path))
          (lines (map json->string events)))
      (catch #t
        (lambda ()
          (let* ((text (string-join lines "\n"))
                 (existing (if (path-exists? path)
                             (string-trim-right (string-load path))
                             "")))
            (if (and (string? existing) (> (string-length existing) 0))
              (string-save
                (string-append existing "\n" text)
                path)
              (string-save text path))
            (display (string-append "[telemetry] flushed "
                                    (number->string (length events))
                                    " event(s) to "
                                    path "\n"))
            #t))
        (lambda args
          (display (string-append "[telemetry] write error: "
                                  (object->string args) "\n"))
          #f)))))

(define-public (telemetry-flush)
  (if (null? *telemetry-event-queue*)
    #t
    (let ((owner (telemetry-acquire-lock)))
      (if owner
        (begin
          (telemetry-write-pending *telemetry-event-queue*)
          (set! *telemetry-event-queue* '())
          (telemetry-release-lock owner)
          #t)
        #f))))
