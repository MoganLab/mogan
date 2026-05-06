
/******************************************************************************
 * MODULE     : telemetry-utils.scm
 * DESCRIPTION: Telemetry utilities for paths, config, and device info
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

(define-library (telemetry telemetry-utils)
  (export telemetry-dir
    telemetry-pending-path
    telemetry-flags-cache-path
    telemetry-lock-path
    telemetry-lock-info-path
    telemetry-ensure-dir
    telemetry-lock-owner
    telemetry-device-id
    telemetry-session-id
    telemetry-app-version
    telemetry-platform
    telemetry-language
    telemetry-timezone
    telemetry-now
    telemetry-make-event
    *telemetry-session-id*
    *telemetry-event-queue*
  )
  (import (scheme base)
    (liii base)
    (liii json)
    (liii path)
    (liii uuid)
  )
  (begin

    (define (telemetry-home-path)
      (let ((home (getenv "TEXMACS_HOME_PATH")))
        (if (and home (> (string-length home) 0))
          home
          (string-append (getenv "HOME") "/.local/share/Mogan"))))

    (define (telemetry-dir)
      (let ((dir (string-append (telemetry-home-path) "/system/telemetry")))
        (if (not (path-exists? dir))
          (mkdir dir))
        dir))

    (define (telemetry-pending-path)
      (string-append (telemetry-dir) "/telemetry-pending.jsonl"))

    (define (telemetry-flags-cache-path)
      (string-append (telemetry-dir) "/telemetry-flags-cache.json"))

    (define (telemetry-lock-path)
      (string-append (telemetry-dir) "/.lock"))

    (define (telemetry-lock-info-path)
      (string-append (telemetry-lock-path) "/owner.json"))

    (define (telemetry-ensure-dir)
      (let ((dir (telemetry-dir)))
        (if (not (path-exists? dir))
          (mkdir dir))))

    (define (telemetry-lock-owner)
      (string-append "telemetry-" (number->string (getpid))))

    (define (telemetry-device-id)
      (let ((id (stem-device-id)))
        (if (string? id) id "unknown")))

    (define (telemetry-session-id)
      (uuid4))

    (define (telemetry-app-version)
      (xmacs-version))

    (define (telemetry-platform)
      (cond ((os-macos?) "macos")
            ((or (os-win32?) (os-mingw?)) "windows")
            (else "linux")))

    (define (telemetry-language)
      (or (getenv "LANG") "en_US"))

    (define (telemetry-timezone)
      (catch #t
        (lambda ()
          (let ((date (current-date)))
            (if (and date (date? date))
              (date-zone date)
              "UTC")))
        (lambda args "UTC")))

    (define (telemetry-now)
      (inexact->exact (truncate (* 1000 (current-time)))))

    (define *telemetry-session-id* (telemetry-session-id))
    (define *telemetry-event-queue* '())

    (define (telemetry-make-event event-type properties)
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

  ))
