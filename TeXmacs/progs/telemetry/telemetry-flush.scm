
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : telemetry-flush.scm
;; DESCRIPTION : Telemetry flush with directory-based lock
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(texmacs-module (telemetry telemetry-flush)
  (:use (telemetry telemetry-utils)))

(import (scheme base)
  (liii base)
  (liii json)
  (liii path)
  (liii string)
  (liii list)
)

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

(define-public (telemetry-acquire-lock)
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

(define-public (telemetry-release-lock owner)
  (let ((info (telemetry-read-lock-info)))
    (if (and info
          (string=? (json-ref-string info "owner" "") owner))
      (telemetry-remove-lock)
      #f)))

(define-public (telemetry-read-pending)
  (let ((path (telemetry-pending-path)))
    (if (path-exists? path)
      (catch #t
        (lambda ()
          (let ((text (string-load path)))
            (if (and (string? text) (> (string-length text) 0))
              (let ((lines (string-split text #\newline)))
                (filter
                  (lambda (line) (> (string-length line) 0))
                  lines))
              '())))
        (lambda args '()))
      '())))

(define-public (telemetry-write-pending events)
  (if (null? events)
    #t
    (let ((path (telemetry-pending-path))
          (lines (map json->string events)))
      (catch #t
        (lambda ()
          (let ((text (string-join lines "\n")))
            (if (path-exists? path)
              (string-save
                (string-append (string-load path) "\n" text)
                path)
              (string-save text path))
            #t))
        (lambda args #f)))))

(define-public (telemetry-truncate-pending)
  (let ((path (telemetry-pending-path)))
    (if (path-exists? path)
      (catch #t
        (lambda ()
          (string-save "" path)
          #t)
        (lambda args #f))
      #t)))

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
