
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 200_62.scm
;; DESCRIPTION : Tests for telemetry core functions
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(use-modules (telemetry telemetry-utils))
(use-modules (telemetry telemetry-track))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-make-event
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((ev (telemetry-make-event "TEST_EVENT" '(("foo" . "bar")))))
  (check (assoc-ref ev "event_type") => "TEST_EVENT")
  (check (number? (assoc-ref ev "timestamp_ms")) => #t)
  (check (string? (assoc-ref ev "distinct_id")) => #t)
  (check (string? (assoc-ref ev "session_id")) => #t)
  (check (string? (assoc-ref ev "event_id")) => #t)
  (check (string? (assoc-ref ev "app_version")) => #t)
  (check (string? (assoc-ref ev "device_id")) => #t)
  (check (string? (assoc-ref ev "platform")) => #t)
  (check (string? (assoc-ref ev "language")) => #t)
  (check (string? (assoc-ref ev "timezone")) => #t)
  (check (assoc-ref ev "properties") => '(("foo" . "bar"))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-filter-stale-events
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let* ((now (telemetry-now))
       (fresh (telemetry-make-event "FRESH" '()))
       (old (telemetry-make-event "OLD" '()))
       (stale-ts (- now 691200000)))  ; 8 days ago
  ;; Patch the old event's timestamp (cadr old) is ("timestamp_ms" . <value>)
  (set-cdr! (cadr old) stale-ts)
  (let ((filtered (telemetry-filter-stale-events (list old fresh))))
    (check (length filtered) => 1)
    (check (assoc-ref (car filtered) "event_type") => "FRESH")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-enabled?
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry")))
  ;; default (no preference or not "0"/"off") => enabled
  (when old-pref (set-preference "telemetry" "1"))
  (check (telemetry-enabled?) => #t)
  (set-preference "telemetry" "0")
  (check (telemetry-enabled?) => #f)
  (set-preference "telemetry" "off")
  (check (telemetry-enabled?) => #f)
  (set-preference "telemetry" "1")
  (check (telemetry-enabled?) => #t)
  ;; restore
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-parse-config
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(telemetry-parse-config "5000" "")
(check (telemetry-get-buffer-size) => telemetry-max-queue-size)
(telemetry-parse-config "5" "")
(check (telemetry-get-buffer-size) => 5)
;; invalid value => no change
(telemetry-parse-config "abc" "")
(check (telemetry-get-buffer-size) => 5)
(telemetry-parse-config "" "30000")
(check (telemetry-get-flush-interval) => 30000)
;; empty strings => no change
(let ((r5 (telemetry-parse-config "" "")))
  (check (null? r5) => #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; track-event respects enabled? flag
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry")))
  (set-preference "telemetry" "1")
  ;; enable first
  (let ((before (telemetry-queue-length)))
    (track-event "TEST_ENABLED" '())
    (check (> (telemetry-queue-length) before) => #t))
  ;; disable and try again
  (set-preference "telemetry" "0")
  (let ((before (telemetry-queue-length)))
    (track-event "TEST_DISABLED" '())
    (check (<= (telemetry-queue-length) before) => #t))
  ;; restore
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-flush empty queue returns #t
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry")))
  (set-preference "telemetry" "1")
  (set! *telemetry-event-queue* '())
  (check (telemetry-flush) => #t)
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-write-pending empty list returns #t
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check (telemetry-write-pending '()) => #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; restore defaults to avoid polluting other tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(telemetry-parse-config "10" "60000")

(define (test_200_62)
  (check-report))
