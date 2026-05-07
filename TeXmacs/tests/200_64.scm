
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 200_64.scm
;; DESCRIPTION : Telemetry 核心功能测试
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

(when (not (community-stem?))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-make-event：验证事件结构
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((ev (telemetry-make-event "TEST_EVENT" '(("foo" . "bar")))))
  (check (assoc-ref ev "event_type") => "TEST_EVENT")
  (check (number? (assoc-ref ev "timestamp_s")) => #t)
  (check (string? (assoc-ref ev "distinct_id")) => #t)
  (check (string? (assoc-ref ev "session_id")) => #t)
  (check (string? (assoc-ref ev "event_id")) => #t)
  (check (string? (assoc-ref ev "app_version")) => #t)
  (check (string? (assoc-ref ev "platform")) => #t)
  (check (string? (assoc-ref ev "language")) => #t)
  (check (string? (assoc-ref ev "timezone")) => #t)
  (check (assoc-ref ev "properties") => '(("foo" . "bar"))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-enabled?：开关控制
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry")))
  ;; 默认开启（无偏好或不是 "0"/"off"）
  (when old-pref (set-preference "telemetry" "1"))
  (check (telemetry-enabled?) => #t)
  (set-preference "telemetry" "0")
  (check (telemetry-enabled?) => #f)
  (set-preference "telemetry" "off")
  (check (telemetry-enabled?) => #f)
  (set-preference "telemetry" "1")
  (check (telemetry-enabled?) => #t)
  ;; 恢复
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; track-event：开关控制
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry")))
  (set-preference "telemetry" "1")
  ;; 开启时入队
  (let ((before (telemetry-queue-length)))
    (track-event "TEST_ENABLED" '())
    (check (> (telemetry-queue-length) before) => #t))
  ;; 禁用时忽略
  (set-preference "telemetry" "0")
  (let ((before (telemetry-queue-length)))
    (track-event "TEST_DISABLED" '())
    (check (<= (telemetry-queue-length) before) => #t))
  ;; 恢复
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-flush：空队列返回 #t
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry")))
  (set-preference "telemetry" "1")
  (set! *telemetry-event-queue* '())
  (check (telemetry-flush) => #t)
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-flush：文件写入与追加
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry")))
  (set-preference "telemetry" "1")
  (let ((path (telemetry-pending-path)))
    ;; 清理：清空文件
    (string-save "" (system->url path))
    (set! *telemetry-event-queue* '())
    (track-event "FLUSH_TEST_A" '())
    (track-event "FLUSH_TEST_B" '())
    (telemetry-flush)
    ;; 验证文件存在且包含 2 行
    (check (path-exists? path) => #t)
    (let ((raw (string-load (system->url path))))
      (let ((lines (filter (lambda (s) (> (string-length s) 0))
                           (string-split raw #\newline))))
        (check (length lines) => 2)
        ;; 验证每行是合法 JSON 且 event_type 正确
        (let ((ev1 (string->json (car lines)))
              (ev2 (string->json (cadr lines))))
          (check (assoc-ref ev1 "event_type") => "FLUSH_TEST_A")
          (check (assoc-ref ev2 "event_type") => "FLUSH_TEST_B"))))
    ;; 验证追加：再次 flush，文件应有 3 行
    (track-event "FLUSH_TEST_C" '())
    (telemetry-flush)
    (let ((raw2 (string-load (system->url path))))
      (let ((lines2 (filter (lambda (s) (> (string-length s) 0))
                            (string-split raw2 #\newline))))
        (check (length lines2) => 3)
        (check (assoc-ref (string->json (caddr lines2)) "event_type") => "FLUSH_TEST_C")))
    ;; 清理：清空文件
    (string-save "" (system->url path)))
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; track-event：达到 buffer-size 自动 flush
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry"))
      (old-buffer-size (telemetry-get-buffer-size)))
  (set-preference "telemetry" "1")
  (let ((path (telemetry-pending-path)))
    (string-save "" (system->url path))
    (telemetry-set-buffer-size! 2)
    (set! *telemetry-event-queue* '())
    (track-event "AUTO_1" '())
    (check (telemetry-queue-length) => 1)
    (track-event "AUTO_2" '())
    ;; buffer-size 为 2，自动 flush 后队列清空
    (check (telemetry-queue-length) => 0)
    (check (path-exists? path) => #t)
    ;; 清理：清空文件
    (string-save "" (system->url path)))
  ;; 恢复
  (telemetry-set-buffer-size! old-buffer-size)
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-flush-if-needed
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry")))
  (set-preference "telemetry" "1")
  ;; 空队列 => 直接返回 #t
  (set! *telemetry-event-queue* '())
  (check (telemetry-flush-if-needed) => #t)
  ;; 非空队列 => flush 并返回 #t
  (track-event "NEEDED_1" '())
  (check (telemetry-flush-if-needed) => #t)
  (check (telemetry-queue-length) => 0)
  ;; 禁用时 => #t
  (set-preference "telemetry" "0")
  (track-event "IGNORED" '())
  (check (telemetry-flush-if-needed) => #t)
  ;; 恢复
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-flush-if-needed：禁用时返回 #t
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(let ((old-pref (get-preference "telemetry")))
  (set-preference "telemetry" "0")
  (set! *telemetry-event-queue* '())
  ;; 禁用时 track-event 不入队
  (track-event "DISABLED_EVENT" '())
  (check (telemetry-queue-length) => 0)
  (check (telemetry-flush-if-needed) => #t)
  ;; 恢复
  (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")))

) ;; end when (skip tests on community build)

(define (test_200_64)
  (check-report))
