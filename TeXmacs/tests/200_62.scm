
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
(import (liii json))
(import (liii path))
(import (liii string))

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
;; telemetry-flush writes events to file with trailing newline
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry")))
  (set-preference "telemetry" "1")
  (let ((path (telemetry-pending-path)))
    ;; clean up: empty the file
    (string-save "" (system->url path))
    (set! *telemetry-event-queue* '())
    (track-event "FLUSH_TEST_A" '())
    (track-event "FLUSH_TEST_B" '())
    (telemetry-flush)
    ;; verify file exists and contains 2 lines
    (check (path-exists? path) => #t)
    (let ((raw (string-load (system->url path))))
      (let ((lines (filter (lambda (s) (> (string-length s) 0))
                           (string-split raw #\newline))))
        (check (length lines) => 2)
        ;; verify each line is valid JSON with correct event_type
        (let ((ev1 (string->json (car lines)))
              (ev2 (string->json (cadr lines))))
          (check (assoc-ref ev1 "event_type") => "FLUSH_TEST_A")
          (check (assoc-ref ev2 "event_type") => "FLUSH_TEST_B"))))
    ;; verify append: flush again, file should have 4 lines
    (track-event "FLUSH_TEST_C" '())
    (telemetry-flush)
    (let ((raw2 (string-load (system->url path))))
      (let ((lines2 (filter (lambda (s) (> (string-length s) 0))
                            (string-split raw2 #\newline))))
        (check (length lines2) => 3)
        (check (assoc-ref (string->json (caddr lines2)) "event_type") => "FLUSH_TEST_C")))
    ;; clean up: empty the file
    (string-save "" (system->url path)))
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; track-event auto-flush when reaching buffer-size
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry")))
  (set-preference "telemetry" "1")
  (let ((path (telemetry-pending-path)))
    (string-save "" (system->url path))
    (telemetry-parse-config "2" "60000")
    (set! *telemetry-event-queue* '())
    (track-event "AUTO_1" '())
    (check (telemetry-queue-length) => 1)
    (track-event "AUTO_2" '())
    ;; buffer-size is 2, so auto-flush should clear the queue
    (check (telemetry-queue-length) => 0)
    (check (path-exists? path) => #t)
    ;; clean up: empty the file
    (string-save "" (system->url path)))
  ;; restore
  (telemetry-parse-config "30" "60000")
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

(define (test_200_62)
  (check-report))
