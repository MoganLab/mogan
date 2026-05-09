
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
  (liii os)
  (liii path)
  (liii string)
  (liii uuid)
)

(import (only (srfi srfi-19)
  current-date date-zone-offset))

(define telemetry-buffer-size 30)
(define telemetry-flush-interval-ms 60000)
(define-public telemetry-max-queue-size 1000)

(define-public (telemetry-get-buffer-size)
  telemetry-buffer-size)

(define-public (telemetry-set-buffer-size! size)
  (set! telemetry-buffer-size size))

(define-public (telemetry-get-flush-interval)
  telemetry-flush-interval-ms)

(define-public (telemetry-set-flush-interval! interval)
  (set! telemetry-flush-interval-ms interval))

(define-public (telemetry-enabled?)
  (if (community-stem?) #f
    (let ((pref (get-preference "telemetry")))
      (not (or (== pref "off") (== pref "0"))))))

(define (telemetry-home-path)
  (url->system (get-texmacs-home-path)))

(define-public (telemetry-dir)
  (let ((dir (string-append (telemetry-home-path) "/system/telemetry")))
    (if (not (path-exists? dir))
      (mkdir dir))
    dir))

(define-public (telemetry-pending-path)
  (string-append (telemetry-dir) "/telemetry-pending.jsonl"))

(define-public (telemetry-lock-path)
  (string-append (telemetry-dir) "/.lock"))

(define-public (telemetry-lock-info-path)
  (string-append (telemetry-lock-path) "/owner.json"))

(define-public (telemetry-ensure-dir)
  (let ((dir (telemetry-dir)))
    (if (not (path-exists? dir))
      (mkdir dir))))

(define-public (telemetry-lock-owner)
  "telemetry-main")

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
      (let ((offset (date-zone-offset (current-date))))
        (if (zero? offset)
            "UTC"
            (let* ((sign (if (>= offset 0) "+" "-"))
                   (abs-offset (abs offset))
                   (hours (quotient abs-offset 3600))
                   (minutes (quotient (remainder abs-offset 3600) 60)))
              (string-append sign
                             (if (< hours 10) "0" "")
                             (number->string hours)
                             ":"
                             (if (< minutes 10) "0" "")
                             (number->string minutes))))))
    (lambda args "UTC")))

(define-public (telemetry-now)
  (inexact->exact (truncate (current-time))))

(define-public *telemetry-session-id* (telemetry-session-id))

(define-public (telemetry-make-event event-type properties)
  `(("eventType" . ,event-type)
    ("timestamp" . ,(telemetry-now))
    ("distinctId" . ,(telemetry-device-id))
    ("sessionId" . ,*telemetry-session-id*)
    ("eventId" . ,(uuid4))
    ("appVersion" . ,(telemetry-app-version))
    ("deviceId" . ,(telemetry-device-id))
    ("platform" . ,(telemetry-platform))
    ("language" . ,(telemetry-language))
    ("timezone" . ,(telemetry-timezone))
    ("properties" . ,(if (null? properties) '(()) properties))))
