
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

(define-public *telemetry-event-queue* '())

;; Safeguards: file size limit and event aging
(define telemetry-max-file-size-bytes 10485760)  ; 10 MB
(define telemetry-max-event-age-ms 604800000)    ; 7 days in ms

(define-public (track-event event-type properties)
  (if (not (telemetry-enabled?))
    #f
    (if (and (string? event-type) (not (string-null? event-type)))
      (begin
        (set! *telemetry-event-queue*
          (cons (telemetry-make-event event-type properties)
                *telemetry-event-queue*))
        (let ((len (length *telemetry-event-queue*)))
          (display (string-append "[telemetry] track: " event-type
                                  " (queue: " (number->string len)
                                  "/" (number->string (telemetry-get-buffer-size)) ")\n"))
          (if (> len telemetry-max-queue-size)
            (set! *telemetry-event-queue*
              (list-head *telemetry-event-queue* telemetry-max-queue-size)))
          (if (>= len (telemetry-get-buffer-size))
            (telemetry-flush)))
        #t)
      #f)))

(define-public (telemetry-queue-length)
  (length *telemetry-event-queue*))

(define-public (telemetry-flush-if-needed)
  (if (not (telemetry-enabled?))
    #t
    (if (not (null? *telemetry-event-queue*))
      (telemetry-flush)
      #t)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Event aging: drop events older than 7 days before flush
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (telemetry-filter-stale-events events)
  (let ((now (telemetry-now))
        (stale-count 0))
    (define (loop evts acc)
      (if (null? evts)
        (begin
          (if (> stale-count 0)
            (display (string-append "[telemetry] warn: dropped "
                                    (number->string stale-count)
                                    " stale event(s)\n")))
          (reverse acc))
        (let ((ev (car evts)))
          (let ((ts (assoc-ref ev "timestamp_ms")))
            (if (and (number? ts) (> (- now ts) telemetry-max-event-age-ms))
              (begin
                (set! stale-count (+ stale-count 1))
                (loop (cdr evts) acc))
              (loop (cdr evts) (cons ev acc)))))))
    (loop events '())))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Flush implementation (merged from telemetry-flush to share mutable state)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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
      (begin
        (display (string-append "[telemetry] warn: lock owner mismatch, skipping release "
                                "(expected " owner ", got "
                                (if info (json-ref-string info "owner" "") "none") ")\n"))
        #f))))

(define (telemetry-write-pending events)
  (if (null? events)
    #t
    (let ((path (telemetry-pending-path))
          (lines (map json->string events)))
      (catch #t
        (lambda ()
          (let* ((text (string-join lines "\n"))
                 (new-size (string-length text))
                 (existing-raw (if (path-exists? path)
                                 (string-trim-right (string-load path))
                                 ""))
                 (existing-events
                   (if (and (string? existing-raw) (> (string-length existing-raw) 0))
                     (catch #t
                       (lambda ()
                         (telemetry-filter-stale-events
                           (map string->json (string-split existing-raw #\newline))))
                       (lambda args '()))
                     '()))
                 (existing-text (if (null? existing-events)
                                  ""
                                  (string-join (map json->string existing-events) "\n")))
                 (existing-size (string-length existing-text)))
            ;; File size safeguard: if total would exceed 10MB, drop old content
            (if (> (+ existing-size new-size 1) telemetry-max-file-size-bytes)
              (begin
                (display (string-append "[telemetry] warn: file size limit (10MB) reached, "
                                        "dropping old events, keeping "
                                        (number->string (length events))
                                        " new event(s)\n"))
                (string-save text path))
              (if (> (string-length existing-text) 0)
                (string-save
                  (string-append existing-text "\n" text)
                  path)
                (string-save text path)))
            (display (string-append "[telemetry] flush: "
                                    (number->string (length events))
                                    " events -> "
                                    path "\n"))
            #t))
        (lambda args
          (display (string-append "[telemetry] error: write failed: "
                                  (object->string args) "\n"))
          #f)))))

(define-public (telemetry-flush)
  (if (null? *telemetry-event-queue*)
    #t
    (let ((owner (telemetry-acquire-lock)))
      (if owner
        (let* ((fresh-events (telemetry-filter-stale-events *telemetry-event-queue*))
               (ok? (telemetry-write-pending fresh-events)))
          (if ok?
            (begin
              (set! *telemetry-event-queue* '())
              (telemetry-release-lock owner)
              #t)
            (begin
              (display (string-append "[telemetry] error: flush failed, keeping "
                                      (number->string (length *telemetry-event-queue*))
                                      " events in memory queue\n"))
              (telemetry-release-lock owner)
              #f)))
        #f))))
