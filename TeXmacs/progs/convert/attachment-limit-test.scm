;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : attachment-limit-test.scm
;; DESCRIPTION : Tests for the extractedChars parsing of attachment ready events
;; COPYRIGHT   : (C) 2026  Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes with NO WARRANTY whatsoever.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (lolly attachment-limit-test))

(import (liii check) (liii json))

(check-set-mode! 'report-failed)

;; 6229：宿主侧 attachment-event-dispatch 从 ready 事件取 extractedChars
;; 透传给 C++（qt-chat-attachment-ready 第三参），C++ 侧据其做提取后累计
;; 上限判定。本测试只覆盖本层的 JSON 取值，不断言 glue 调用形状。

(define attachment-max-total-extracted-chars (* 1 1024 1024))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for extracting extractedChars from a ready event
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-extracted-chars-present)
  ;; ready 事件六键契约中的 extractedChars：string-length 的结果
  (let ((j (string->json "{\"event\":\"ready\",\"taskId\":\"t1\",\"sha256\":\"abc\",\"filename\":\"a.pdf\",\"size\":10,\"extractedChars\":1234}"
           ) ;string->json
        ) ;j
       ) ;
    (check (json-ref-number j "extractedChars" 0) => 1234)
  ) ;let
) ;define

(define (test-extracted-chars-zero)
  ;; 合法的零值（空文档提取出空文本）不应被当作缺失
  (let ((j (string->json "{\"event\":\"ready\",\"taskId\":\"t2\",\"sha256\":\"abc\",\"filename\":\"a.pdf\",\"size\":0,\"extractedChars\":0}"
           ) ;string->json
        ) ;j
       ) ;
    (check (json-ref-number j "extractedChars" -1) => 0)
  ) ;let
) ;define

(define (test-extracted-chars-missing)
  ;; 缺键回落默认值：老对端不带该键时按 0 计入累计，不应误拒
  (let ((j (string->json "{\"event\":\"ready\",\"taskId\":\"t3\",\"sha256\":\"abc\",\"filename\":\"a.pdf\",\"size\":10}"
           ) ;string->json
        ) ;j
       ) ;
    (check (json-ref-number j "extractedChars" 0) => 0)
  ) ;let
) ;define

(define (test-extracted-chars-non-numeric)
  ;; 非数值（如误传字符串）同样回落默认值，不污染累计
  (let ((j (string->json "{\"event\":\"ready\",\"taskId\":\"t4\",\"sha256\":\"abc\",\"filename\":\"a.pdf\",\"size\":10,\"extractedChars\":\"oops\"}"
           ) ;string->json
        ) ;j
       ) ;
    (check (json-ref-number j "extractedChars" 0) => 0)
  ) ;let
) ;define

(define (test-extracted-chars-large)
  ;; 超大值按整数透传（glue 侧为 int，1 MiB 上限内安全）
  (let ((j (string->json (string-append "{\"event\":\"ready\",\"taskId\":\"t5\",\"sha256\":\"abc\",\"extractedChars\":"
                           (number->string attachment-max-total-extracted-chars)
                           "}"
                         ) ;string-append
           ) ;string->json
        ) ;j
       ) ;
    (check (json-ref-number j "extractedChars" 0)
      =>
      attachment-max-total-extracted-chars
    ) ;check
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for the accumulation threshold (mirrors C++ totalReadyExtractedChars)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (attachment-exceeds-limit? chars-list)
  ;; 与 C++ ChatController::totalReadyExtractedChars 同语义：只累加 ready
  ;; 条目（此处入参即已就绪项），总和「大于」上限才拒绝
  (> (apply + chars-list) attachment-max-total-extracted-chars)
) ;define

(define (test-limit-exact-boundary)
  (check (attachment-exceeds-limit? (list attachment-max-total-extracted-chars))
    =>
    #f
  ) ;check
) ;define

(define (test-limit-one-over)
  (check (attachment-exceeds-limit? (list (+ attachment-max-total-extracted-chars 1)))
    =>
    #t
  ) ;check
) ;define

(define (test-limit-accumulate)
  (let ((half (/ attachment-max-total-extracted-chars 2)))
    (check (attachment-exceeds-limit? (list half half)) => #f)
    (check (attachment-exceeds-limit? (list half half 1)) => #t)
  ) ;let
) ;define

(define (test-limit-empty)
  (check (attachment-exceeds-limit? '()) => #f)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-attachment-limit)
  (test-extracted-chars-present)
  (test-extracted-chars-zero)
  (test-extracted-chars-missing)
  (test-extracted-chars-non-numeric)
  (test-extracted-chars-large)
  (test-limit-exact-boundary)
  (test-limit-one-over)
  (test-limit-accumulate)
  (test-limit-empty)
  (check-report)
) ;tm-define
