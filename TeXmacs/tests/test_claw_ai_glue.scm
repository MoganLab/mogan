;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : test-claw-ai-glue.scm
;; DESCRIPTION : Test Claw AI Glue functions
;; COPYRIGHT   : (C) 2026 Liii Network
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Test if Claw AI Glue functions are registered
(define (test-claw-ai-glue)
  (display "Testing Claw AI Glue functions...")
  (newline)
  
  ;; Test 1: Check if functions are defined
  (display "1. Checking function definitions...")
  (newline)
  
  (if (defined? 'claw-ai-widget-show)
      (display "   ✓ claw-ai-widget-show defined")
      (display "   ✗ claw-ai-widget-show NOT defined"))
  (newline)
  
  (if (defined? 'claw-ai-widget-hide)
      (display "   ✓ claw-ai-widget-hide defined")
      (display "   ✗ claw-ai-widget-hide NOT defined"))
  (newline)
  
  (if (defined? 'claw-ai-widget-append)
      (display "   ✓ claw-ai-widget-append defined")
      (display "   ✗ claw-ai-widget-append NOT defined"))
  (newline)
  
  (if (defined? 'claw-ai-widget-update-last)
      (display "   ✓ claw-ai-widget-update-last defined")
      (display "   ✗ claw-ai-widget-update-last NOT defined"))
  (newline)
  
  (if (defined? 'claw-ai-widget-clear)
      (display "   ✓ claw-ai-widget-clear defined")
      (display "   ✗ claw-ai-widget-clear NOT defined"))
  (newline)
  
  (if (defined? 'claw-ai-widget-set-streaming)
      (display "   ✓ claw-ai-widget-set-streaming defined")
      (display "   ✗ claw-ai-widget-set-streaming NOT defined"))
  (newline)
  
  (if (defined? 'claw-ai-widget-message-count)
      (display "   ✓ claw-ai-widget-message-count defined")
      (display "   ✗ claw-ai-widget-message-count NOT defined"))
  (newline)
  
  ;; Test 2: Try calling show/hide (should not crash)
  (display "2. Testing function calls...")
  (newline)
  
  (if (defined? 'claw-ai-widget-show)
      (begin
        (display "   Calling claw-ai-widget-show...")
        (newline)
        (catch #t
          (lambda ()
            (claw-ai-widget-show)
            (display "   ✓ claw-ai-widget-show executed successfully"))
          (lambda (key . args)
            (display "   ✗ Error: ")
            (display args)))
        (newline)))
  
  (if (defined? 'claw-ai-widget-hide)
      (begin
        (display "   Calling claw-ai-widget-hide...")
        (newline)
        (catch #t
          (lambda ()
            (claw-ai-widget-hide)
            (display "   ✓ claw-ai-widget-hide executed successfully"))
          (lambda (key . args)
            (display "   ✗ Error: ")
            (display args)))
        (newline)))
  
  (display "Done!")
  (newline))

;; Run the test
(test-claw-ai-glue)
