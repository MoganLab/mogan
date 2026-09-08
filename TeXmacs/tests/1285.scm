;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1285.scm
;; DESCRIPTION : Telemetry meta (liii json) refactor unit tests
;; COPYRIGHT   : (C) 2026  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT NO WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check) (liii json) (liii path))

(use-modules (telemetry telemetry-utils))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 1. 读写往返与标准 JSON 格式验证
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-meta-roundtrip)
  (display "Testing telemetry meta roundtrip and json structure...\n")
  (let ((meta-path (telemetry-meta-path)))
    ;; 写入空列表
    (check (telemetry-write-meta '()) => #t)
    (check (telemetry-read-meta) => '())
    (check (path-read-text meta-path) => "[]")

    ;; 写入多条元数据记录
    (let ((test-entries '((("filename" . "detail-1.jsonl") ("timestamp" . 1000))
                          (("filename" . "detail-2.jsonl") ("timestamp" . 2000)))
          ) ;test-entries
         ) ;
      (check (telemetry-write-meta test-entries) => #t)
      (check (telemetry-read-meta) => test-entries)

      ;; 验证磁盘实际存储为符合 (liii json) 规范的 JSON 数组
      (let* ((raw (path-read-text meta-path)) (data (string->json raw)))
        (check (vector? data) => #t)
        (check (vector-length data) => 2)
        (check (json-ref (vector-ref data 0) "filename") => "detail-1.jsonl")
        (check (json-ref (vector-ref data 0) "timestamp") => 1000)
        (check (json-ref (vector-ref data 1) "filename") => "detail-2.jsonl")
        (check (json-ref (vector-ref data 1) "timestamp") => 2000)
      ) ;let*
    ) ;let
  ) ;let
  (display "telemetry meta roundtrip passed!\n")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 2. 异常数据容错与安全回退测试
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-meta-fallback)
  (display "Testing telemetry meta error fallback...\n")
  (let ((meta-path (telemetry-meta-path)))
    ;; 文件不存在时回退为 '()
    (when (path-exists? meta-path)
      (path-unlink meta-path)
    ) ;when
    (check (telemetry-read-meta) => '())

    ;; 文件内容为空字符串时回退为 '()
    (path-write-text meta-path "")
    (check (telemetry-read-meta) => '())

    ;; 文件内容为损坏的畸形 JSON 时安全捕获并回退为 '()
    (path-write-text meta-path "{invalid json: ")
    (check (telemetry-read-meta) => '())

    ;; 文件内容为非数组 JSON（如单个对象或标量）时安全回退为 '()
    (path-write-text meta-path "{\"meta\":1}")
    (check (telemetry-read-meta) => '())

    (path-write-text meta-path "42")
    (check (telemetry-read-meta) => '())
  ) ;let
  (display "telemetry meta fallback passed!\n")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 3. telemetry-meta-add-entry 增量添加测试
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-meta-add-entry)
  (display "Testing telemetry-meta-add-entry...\n")
  (telemetry-write-meta '())
  (check (telemetry-read-meta) => '())

  ;; 添加第一项
  (telemetry-meta-add-entry "detail-test-a.jsonl")
  (let ((m1 (telemetry-read-meta)))
    (check (length m1) => 1)
    (check (assoc-ref (car m1) "filename") => "detail-test-a.jsonl")
    (check (number? (assoc-ref (car m1) "timestamp")) => #t)
  ) ;let

  ;; 添加第二项，最新项排在最前
  (telemetry-meta-add-entry "detail-test-b.jsonl")
  (let ((m2 (telemetry-read-meta)))
    (check (length m2) => 2)
    (check (assoc-ref (car m2) "filename") => "detail-test-b.jsonl")
    (check (assoc-ref (cadr m2) "filename") => "detail-test-a.jsonl")
  ) ;let

  (display "telemetry-meta-add-entry passed!\n")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 测试入口
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (test_1285)
  (display "Running test_1285...\n")
  (let ((orig-meta (telemetry-read-meta)))
    (test-meta-roundtrip)
    (test-meta-fallback)
    (test-meta-add-entry)
    ;; 恢复原有 meta
    (telemetry-write-meta orig-meta)
  ) ;let
  (check-report)
) ;tm-define
