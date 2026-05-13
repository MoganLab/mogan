;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : sidebar-session-test.scm
;; DESCRIPTION : tests for sidebar session
;; COPYRIGHT   : (C) 2025--2026  Mogan Contributors
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (sidebar-session-test)
  (:use (dynamic sidebar-session)
        (generic chat-sidebar-ui)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test helpers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define test-passed 0)
(define test-failed 0)

(define (assert-equal expected actual msg)
  (if (equal? expected actual)
      (begin
        (set! test-passed (+ test-passed 1))
        (display* "[PASS] " msg "\n"))
      (begin
        (set! test-failed (+ test-failed 1))
        (display* "[FAIL] " msg "\n")
        (display* "       expected: " (object->string expected) "\n")
        (display* "       actual:   " (object->string actual) "\n"))))

(define (assert-true val msg)
  (assert-equal #t val msg))

(define (assert-false val msg)
  (assert-equal #f val msg))

(define (test-summary)
  (display* "\n=== Test Summary ===\n")
  (display* "Passed: " test-passed "\n")
  (display* "Failed: " test-failed "\n")
  (if (> test-failed 0)
      (display* "RESULT: FAILURE\n")
      (display* "RESULT: SUCCESS\n")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (run-sidebar-session-tests)
  (:synopsis "Run sidebar session tests")
  (:interactive #t)

  (set! test-passed 0)
  (set! test-failed 0)

  (display* "\n=== Sidebar Session Tests ===\n\n")

  ;; Test 1: Buffer names exist
  (assert-true (string? (chat-sidebar-message-buffer-name))
               "message buffer name is a string")
  (assert-true (string? (chat-sidebar-input-buffer-name))
               "input buffer name is a string")

  ;; Test 2: Ensure buffer creates buffer
  (let ((msg (chat-sidebar-ensure-buffer! (chat-sidebar-message-buffer-name))))
    (assert-true (buffer-exists? (chat-sidebar-message-buffer-name))
                 "message buffer exists after ensure"))

  ;; Test 3: Clear input
  (chat-sidebar-clear-input!)
  (let ((body (buffer-get-body (chat-sidebar-input-buffer-name))))
    (assert-true (tree? body) "input buffer body is a tree after clear"))

  ;; Test 4: Clear message
  (chat-sidebar-clear-message!)
  (let ((body (buffer-get-body (chat-sidebar-message-buffer-name))))
    (assert-true (tree? body) "message buffer body is a tree after clear"))

  ;; Test 5: Reset clears both
  (chat-sidebar-reset!)
  (assert-true (buffer-exists? (chat-sidebar-message-buffer-name))
               "message buffer exists after reset")
  (assert-true (buffer-exists? (chat-sidebar-input-buffer-name))
               "input buffer exists after reset")

  ;; Test 6: Active session state
  (assert-false (sidebar-session-active?)
                "no active session initially")
  (assert-false (sidebar-session-id)
                "session id is #f initially")

  ;; Test 7: Debug log file path
  (assert-true (string? sidebar-session-debug-log-file)
               "debug log file path is a string")

  (test-summary))
