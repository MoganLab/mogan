
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : telemetry-utils.scm
;; DESCRIPTION : Telemetry utilities for paths, config, and device info
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (telemetry telemetry-utils))

(import (scheme base)
  (liii base)
  (liii json)
  (liii os)
  (liii path)
  (liii uuid)
)

(define telemetry-buffer-size 10)
(define telemetry-flush-interval-ms 60000)
(define-public telemetry-max-queue-size 1000)

(define-public (telemetry-get-buffer-size)
  telemetry-buffer-size)

(define-public (telemetry-get-flush-interval)
  telemetry-flush-interval-ms)

(define-public (telemetry-enabled?)
  (let ((pref (get-preference "telemetry")))
    (not (or (== pref "off") (== pref "0")))))

(define (telemetry-home-path)
  (url->system (get-texmacs-home-path)))

(define-public (telemetry-dir)
  (let ((dir (string-append (telemetry-home-path) "/system/telemetry")))
    (if (not (path-exists? dir))
      (mkdir dir))
    dir))

(define-public (telemetry-pending-path)
  (string-append (telemetry-dir) "/telemetry-pending.jsonl"))

(define-public (telemetry-flags-cache-path)
  (string-append (telemetry-dir) "/telemetry-flags-cache.json"))

(define-public (telemetry-lock-path)
  (string-append (telemetry-dir) "/.lock"))

(define-public (telemetry-lock-info-path)
  (string-append (telemetry-lock-path) "/owner.json"))

(define-public (telemetry-ensure-dir)
  (let ((dir (telemetry-dir)))
    (if (not (path-exists? dir))
      (mkdir dir))))

(define-public (telemetry-lock-owner)
  (string-append "telemetry-" (number->string (getpid))))

(define-public (telemetry-device-id)
  (let ((id (stem-device-id)))
    (if (string? id) id "unknown")))

(define-public (telemetry-session-id)
  (uuid4))

(define-public (telemetry-app-version)
  (xmacs-version))

(define-public (telemetry-platform)
  (cond ((os-macos?) "macos")
        ((or (os-win32?) (os-mingw?)) "windows")
        (else "linux")))

(define-public (telemetry-language)
  (let ((lang (or (system-getenv "LANG") "en_US")))
    (if (string-contains? lang ".")
      (car (string-split lang #\.))
      lang)))

(define-public (telemetry-timezone)
  (catch #t
    (lambda ()
      (let ((date (current-date)))
        (if (and date (date? date))
          (date-zone date)
          "UTC")))
    (lambda args "UTC")))

(define-public (telemetry-now)
  (inexact->exact (truncate (* 1000 (current-time)))))

(define-public *telemetry-session-id* (telemetry-session-id))

(define-public (telemetry-parse-config buffer-str interval-str)
  (let ((result '()))
    (if (and (string? buffer-str) (> (string-length buffer-str) 0))
      (let ((val (string->number buffer-str)))
        (if (and (number? val) (> val 0))
          (let ((clamped (min (inexact->exact (truncate val)) telemetry-max-queue-size)))
            (if (< clamped (inexact->exact (truncate val)))
              (display (string-append "[telemetry] warn: MOGAN_TELEMETRY_BUFFER_SIZE exceeds max queue size ("
                                      (number->string telemetry-max-queue-size) "), clamped to "
                                      (number->string clamped) "\n")))
            (set! telemetry-buffer-size clamped)
            (set! result (acons 'buffer-size telemetry-buffer-size result)))
          (display (string-append "[telemetry] warn: invalid MOGAN_TELEMETRY_BUFFER_SIZE="
                                  buffer-str ", using default "
                                  (number->string telemetry-buffer-size) "\n")))))
    (if (and (string? interval-str) (> (string-length interval-str) 0))
      (let ((val (string->number interval-str)))
        (if (and (number? val) (> val 0))
          (begin
            (set! telemetry-flush-interval-ms (inexact->exact (truncate val)))
            (set! result (acons 'flush-interval telemetry-flush-interval-ms result)))
          (display (string-append "[telemetry] warn: invalid MOGAN_TELEMETRY_FLUSH_INTERVAL="
                                  interval-str ", using default "
                                  (number->string telemetry-flush-interval-ms) "ms\n")))))
    result))

(define-public (telemetry-load-config)
  (telemetry-parse-config
    (system-getenv "MOGAN_TELEMETRY_BUFFER_SIZE")
    (system-getenv "MOGAN_TELEMETRY_FLUSH_INTERVAL")))

(define-public (telemetry-make-event event-type properties)
  `(("event_type" . ,event-type)
    ("timestamp_ms" . ,(telemetry-now))
    ("distinct_id" . ,(telemetry-device-id))
    ("session_id" . ,*telemetry-session-id*)
    ("event_id" . ,(uuid4))
    ("app_version" . ,(telemetry-app-version))
    ("device_id" . ,(telemetry-device-id))
    ("platform" . ,(telemetry-platform))
    ("language" . ,(telemetry-language))
    ("timezone" . ,(telemetry-timezone))
    ("properties" . ,(if (list? properties) properties '()))))
