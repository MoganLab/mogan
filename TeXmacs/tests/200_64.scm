
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
(import (liii path))
(import (liii string))

(check-set-mode! 'report-failed)

(use-modules (telemetry telemetry-utils))
(use-modules (telemetry telemetry-track))

(define (string-contains-digit? s)
  (let loop ((chars (string->list s)))
    (and (not (null? chars))
         (or (and (char>=? (car chars) #\0) (char<=? (car chars) #\9))
             (loop (cdr chars))))))

(when (not (community-stem?))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-make-event：验证事件结构
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  (let ((ev (telemetry-make-event "TEST_EVENT" '(("foo" . "bar")))))
    (check (assoc-ref ev "eventType") => "TEST_EVENT")
    (check (number? (assoc-ref ev "timestamp")) => #t)
    (check (string? (assoc-ref ev "distinctId")) => #t)
    (check (string? (assoc-ref ev "sessionId")) => #t)
    (check (string? (assoc-ref ev "eventId")) => #t)
    (check (string? (assoc-ref ev "appVersion")) => #t)
    (check (string? (assoc-ref ev "deviceId")) => #t)
    (check (string? (assoc-ref ev "platform")) => #t)
    (check (string? (assoc-ref ev "language")) => #t)
    (check (string? (assoc-ref ev "timezone")) => #t)
    (check (assoc-ref ev "properties") => '(("foo" . "bar")))
  ) ;let

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; sessionId / eventId 语义验证
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  (let* ((ev1 (telemetry-make-event "TEST_SESSION" '()))
         (ev2 (telemetry-make-event "TEST_SESSION" '()))
         (sid1 (assoc-ref ev1 "sessionId"))
         (sid2 (assoc-ref ev2 "sessionId"))
         (eid1 (assoc-ref ev1 "eventId"))
         (eid2 (assoc-ref ev2 "eventId"))
        )
    ;; 同一次运行 sessionId 保持一致
    (check sid1 => sid2)
    ;; 每个事件 eventId 独立
    (check (not (equal? eid1 eid2)) => #t)
  ) ;let*

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-platform：返回具体系统版本（而非笼统的 macos/windows/linux）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  (let ((platform (telemetry-platform)))
    (check (string? platform) => #t)
    ;; 应包含版本号或具体发行版名，不再只是三选一
    (check (string-contains-digit? platform) => #t)
  ) ;let

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-language：跨平台能拿到有效语言标识
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  (let ((language (telemetry-language)))
    (check (string? language) => #t)
    ;; 应为类似 en_US、zh_CN 的 locale 格式，含下划线
    (check (string-contains? language "_") => #t)
  ) ;let

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-enabled?：开关控制
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  (let ((old-pref (get-preference "telemetry")))
    ;; 默认开启（无偏好或不是 "0"/"off"）
    (when old-pref
      (set-preference "telemetry" "1")
    ) ;when
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
      (reset-preference "telemetry")
    ) ;if
  ) ;let

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; track-event：开关控制
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  (let ((old-pref (get-preference "telemetry")))
    (set-preference "telemetry" "1")
    ;; 开启时入队
    (let ((before (telemetry-queue-length)))
      (track-event "TEST_ENABLED" '())
      (check (> (telemetry-queue-length) before) => #t)
    ) ;let
    ;; 禁用时忽略
    (set-preference "telemetry" "0")
    (let ((before (telemetry-queue-length)))
      (track-event "TEST_DISABLED" '())
      (check (<= (telemetry-queue-length) before) => #t)
    ) ;let
    ;; 恢复
    (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")
    ) ;if
  ) ;let

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-flush：空队列返回 #t
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  (let ((old-pref (get-preference "telemetry")))
    (set-preference "telemetry" "1")
    (set! *telemetry-event-queue* '())
    (check (telemetry-flush) => #t)
    (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")
    ) ;if
  ) ;let

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-flush：独立 jsonl 文件写入与 meta 更新
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  (let ((old-pref (get-preference "telemetry")))
    (set-preference "telemetry" "1")
    ;; 清理：删除已有 jsonl，清空 meta
    (let ((old-meta (telemetry-read-meta)))
      (for-each (lambda (entry)
                  (let ((f (assoc-ref entry "filename")))
                    (when f
                      (let ((p (telemetry-full-path f)))
                        (when (path-exists? p)
                          (path-unlink p)
                        ) ;when
                      ) ;let
                    ) ;when
                  ) ;let
                ) ;lambda
        old-meta
      ) ;for-each
    ) ;let
    (telemetry-write-meta '())
    (set! *telemetry-event-queue* '())
    (track-event "FLUSH_TEST_A" '())
    (track-event "FLUSH_TEST_B" '())
    (telemetry-flush)
    ;; 验证 meta 包含 1 条记录
    (let ((meta1 (telemetry-read-meta)))
      (check (length meta1) => 1)
      (let* ((filename1 (assoc-ref (car meta1) "filename"))
             (path1 (telemetry-full-path filename1))
            ) ;
        ;; 验证文件存在且包含 2 行
        (check (path-exists? path1) => #t)
        (let ((raw (string-load (system->url path1))))
          (let ((lines (filter (lambda (s) (> (string-length s) 0)) (string-split raw #\newline))
                ) ;lines
               ) ;
            (check (length lines) => 2)
            (check (string-contains? (car lines) "\"eventType\":\"FLUSH_TEST_A\"") => #t)
            (check (string-contains? (cadr lines) "\"eventType\":\"FLUSH_TEST_B\"") => #t)
          ) ;let
        ) ;let
        ;; 再次 flush，应生成新文件
        (track-event "FLUSH_TEST_C" '())
        (telemetry-flush)
        (let ((meta2 (telemetry-read-meta)))
          (check (length meta2) => 2)
          (let* ((filename2 (assoc-ref (car meta2) "filename"))
                 (path2 (telemetry-full-path filename2))
                ) ;
            (check (path-exists? path2) => #t)
            (let ((raw2 (string-load (system->url path2))))
              (let ((lines2 (filter (lambda (s) (> (string-length s) 0)) (string-split raw2 #\newline))
                    ) ;lines2
                   ) ;
                (check (length lines2) => 1)
                (check (string-contains? (car lines2) "\"eventType\":\"FLUSH_TEST_C\"") => #t)
              ) ;let
            ) ;let
            ;; 清理
            (path-unlink path1)
            (path-unlink path2)
            (telemetry-write-meta '())
          ) ;let*
        ) ;let
      ) ;let*
    ) ;let
    (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")
    ) ;if
  ) ;let

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; track-event：达到 buffer-size 自动 flush
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  (let ((old-pref (get-preference "telemetry"))
        (old-buffer-size (telemetry-get-buffer-size))
       ) ;
    (set-preference "telemetry" "1")
    ;; 清理
    (let ((old-meta (telemetry-read-meta)))
      (for-each (lambda (entry)
                  (let ((f (assoc-ref entry "filename")))
                    (when f
                      (let ((p (telemetry-full-path f)))
                        (when (path-exists? p)
                          (path-unlink p)
                        ) ;when
                      ) ;let
                    ) ;when
                  ) ;let
                ) ;lambda
        old-meta
      ) ;for-each
    ) ;let
    (telemetry-write-meta '())
    (telemetry-set-buffer-size! 2)
    (set! *telemetry-event-queue* '())
    (track-event "AUTO_1" '())
    (check (telemetry-queue-length) => 1)
    (track-event "AUTO_2" '())
    ;; buffer-size 为 2，自动 flush 后队列清空
    (check (telemetry-queue-length) => 0)
    (let ((meta (telemetry-read-meta)))
      (check (> (length meta) 0) => #t)
      (let* ((filename (assoc-ref (car meta) "filename"))
             (path (telemetry-full-path filename))
            ) ;
        (check (path-exists? path) => #t)
        ;; 清理
        (path-unlink path)
        (telemetry-write-meta '())
      ) ;let*
    ) ;let
    ;; 恢复
    (telemetry-set-buffer-size! old-buffer-size)
    (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")
    ) ;if
  ) ;let

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
      (reset-preference "telemetry")
    ) ;if
  ) ;let

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; telemetry-meta-add-entry：滚动保留 200 条，溢出移除最旧
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  (let ((old-pref (get-preference "telemetry"))
        (old-buffer-size (telemetry-get-buffer-size))
       ) ;
    (set-preference "telemetry" "1")
    ;; 清理旧数据
    (let ((old-meta (telemetry-read-meta)))
      (for-each (lambda (entry)
                  (let ((f (assoc-ref entry "filename")))
                    (when f
                      (let ((p (telemetry-full-path f)))
                        (when (path-exists? p)
                          (path-unlink p)
                        ) ;when
                      ) ;let
                    ) ;when
                  ) ;let
                ) ;lambda
        old-meta
      ) ;for-each
    ) ;let
    (telemetry-write-meta '())
    (telemetry-set-buffer-size! 1)
    (set! *telemetry-event-queue* '())
    ;; 连续 flush 205 次
    (let loop
      ((i 0))
      (when (< i 205)
        (track-event (string-append "OVERFLOW_" (number->string i)) '())
        (loop (+ i 1))
      ) ;when
    ) ;let
    ;; 验证 meta 只有 200 条
    (let ((meta (telemetry-read-meta)))
      (check (length meta) => 200)
      ;; meta 新在前旧在后：读第一个 jsonl 验证 eventType 是 OVERFLOW_204
      (let* ((first-file (assoc-ref (car meta) "filename"))
             (first-path (telemetry-full-path first-file))
             (first-line (car (filter (lambda (s) (> (string-length s) 0))
                                (string-split (string-load (system->url first-path)) #\newline)
                              ) ;filter
                         ) ;car
             ) ;first-line
            ) ;
        (check (string-contains? first-line "\"eventType\":\"OVERFLOW_204\"") => #t)
      ) ;let*
      ;; 读最后一个 jsonl 验证 eventType 是 OVERFLOW_5
      (let* ((last-file (assoc-ref (car (reverse meta)) "filename"))
             (last-path (telemetry-full-path last-file))
             (last-line (car (filter (lambda (s) (> (string-length s) 0))
                               (string-split (string-load (system->url last-path)) #\newline)
                             ) ;filter
                        ) ;car
             ) ;last-line
            ) ;
        (check (string-contains? last-line "\"eventType\":\"OVERFLOW_5\"") => #t)
      ) ;let*
    ) ;let
    ;; 清理
    (let ((meta (telemetry-read-meta)))
      (for-each (lambda (entry)
                  (let ((f (assoc-ref entry "filename")))
                    (when f
                      (let ((p (telemetry-full-path f)))
                        (when (path-exists? p)
                          (path-unlink p)
                        ) ;when
                      ) ;let
                    ) ;when
                  ) ;let
                ) ;lambda
        meta
      ) ;for-each
    ) ;let
    (telemetry-write-meta '())
    ;; 恢复
    (telemetry-set-buffer-size! old-buffer-size)
    (if old-pref
      (set-preference "telemetry" old-pref)
      (reset-preference "telemetry")
    ) ;if
  ) ;let

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
      (reset-preference "telemetry")
    ) ;if
  ) ;let

) ;when
;; end when (skip tests on community build)

(define (test_200_64)
  (check-report)
) ;define
