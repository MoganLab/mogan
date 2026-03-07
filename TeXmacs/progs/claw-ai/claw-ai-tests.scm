;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : claw-ai-tests.scm
;; DESCRIPTION : Unit tests for Claw AI module
;; COPYRIGHT   : (C) 2026 Liii Network
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (claw-ai-tests)
  (:use (claw-ai)))

;; ========== 测试框架 ==========

(define test-passed 0)
(define test-failed 0)
(define test-current-name "")

(define (test-begin name)
  (set! test-current-name name)
  (display* "\n=== Test: " name " ===\n"))

(define (test-end name)
  (display* "=== End: " name " ===\n")
  (display* "Passed: " test-passed " Failed: " test-failed "\n\n"))

(define (test-assert name condition)
  (if condition
      (begin
        (set! test-passed (+ test-passed 1))
        (display* "  ✓ " name "\n"))
      (begin
        (set! test-failed (+ test-failed 1))
        (display* "  ✗ " name " FAILED\n"))))

(define (test-equal name expected actual)
  (if (equal? expected actual)
      (begin
        (set! test-passed (+ test-passed 1))
        (display* "  ✓ " name "\n"))
      (begin
        (set! test-failed (+ test-failed 1))
        (display* "  ✗ " name " FAILED\n")
        (display* "    Expected: " expected "\n")
        (display* "    Actual: " actual "\n"))))

;; ========== 缓冲区管理测试 ==========

(define (test-buffer-management)
  (test-begin "Buffer Management")
  
  ;; 测试获取缓冲区
  (let ((buf (claw-ai-buffer-get)))
    (test-assert "claw-ai-buffer-get returns a buffer"
                 (url? buf)))
  
  ;; 测试缓冲区缓存
  (let ((buf1 (claw-ai-buffer-get))
        (buf2 (claw-ai-buffer-get)))
    (test-equal "claw-ai-buffer-get returns same buffer"
                buf1 buf2))
  
  (test-end "Buffer Management"))

;; ========== 历史管理测试 ==========

(define (test-history-management)
  (test-begin "History Management")
  
  ;; 清空历史
  (claw-ai-clear-history)
  (test-equal "History is empty after clear"
              '() (claw-ai-get-history))
  
  ;; 添加消息到历史（模拟）
  (set! claw-ai-message-history
        (list (list "user" "Hello")
              (list "assistant" "Hi there!")))
  
  (test-equal "History has 2 messages"
              2 (length (claw-ai-get-history)))
  
  ;; 再次清空
  (claw-ai-clear-history)
  (test-equal "History is empty after second clear"
              '() (claw-ai-get-history))
  
  (test-end "History Management"))

;; ========== 上下文获取测试 ==========

(define (test-context-retrieval)
  (test-begin "Context Retrieval")
  
  (let ((context (claw-ai-get-context)))
    (test-assert "Context is a list"
                 (list? context))
    
    (test-assert "Context has doc-type"
                 (assoc "doc-type" context))
    
    (test-assert "Context has buffer"
                 (assoc "buffer" context)))
  
  (test-end "Context Retrieval"))

;; ========== 消息发送测试 ==========

(define (test-message-sending)
  (test-begin "Message Sending")
  
  ;; 清空历史
  (claw-ai-clear-history)
  
  ;; 测试空消息不发送
  (claw-ai-send "")
  (test-equal "Empty message not added to history"
              '() (claw-ai-get-history))
  
  ;; 测试有效消息
  (claw-ai-send "Test message")
  ;; 注意：实际发送会触发异步操作，这里只测试历史记录
  ;; 在真实测试中需要等待异步完成
  
  (test-end "Message Sending"))

;; ========== 窗口状态测试 ==========

(define (test-window-state)
  (test-begin "Window State")
  
  ;; 初始状态
  (test-equal "Initial visibility is false"
              #f claw-ai-widget-visible?)
  
  ;; 注意：实际显示/隐藏窗口需要 GUI，这里只测试状态变量
  ;; (claw-ai-show)
  ;; (test-equal "Visibility is true after show"
  ;;             #t claw-ai-widget-visible?)
  
  ;; (claw-ai-hide)
  ;; (test-equal "Visibility is false after hide"
  ;;             #f claw-ai-widget-visible?)
  
  (test-end "Window State"))

;; ========== 运行所有测试 ==========

(tm-define (run-claw-ai-tests)
  (:synopsis "运行所有 Claw AI 单元测试")
  (:interactive #t)
  
  (display* "\n========== Claw AI Unit Tests ==========\n")
  
  (set! test-passed 0)
  (set! test-failed 0)
  
  (test-buffer-management)
  (test-history-management)
  (test-context-retrieval)
  (test-message-sending)
  (test-window-state)
  
  (display* "========================================\n")
  (display* "Total Passed: " test-passed "\n")
  (display* "Total Failed: " test-failed "\n")
  (display* "========================================\n")
  
  (if (= test-failed 0)
      (display* "\n✓ All tests passed!\n")
      (display* "\n✗ Some tests failed.\n")))

;; ========== 快捷命令 ==========

(tm-define (claw-ai-test)
  (:synopsis "快捷命令：运行 Claw AI 测试")
  (:interactive #t)
  (run-claw-ai-tests))
