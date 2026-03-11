; Test script for AI Chat Widget
; Run this in Mogan's Scheme console

; Test 1: Check if widget-chat-messages function exists
(display "Test 1: widget-chat-messages function exists? ")
(if (defined? 'widget-chat-messages)
    (display "YES\n")
    (display "NO\n"))

; Test 2: Try to create the widget
(display "Test 2: Creating widget... ")
(with-exception-handler
  (lambda (e)
    (display "FAILED: ")
    (display e)
    (newline))
  (lambda ()
    (let ((w (widget-chat-messages)))
      (display "SUCCESS\n")
      (display "Widget: ")
      (display w)
      (newline))))

; Test 3: Check ai-widget module
(display "Test 3: Loading ai-widget module... ")
(with-exception-handler
  (lambda (e)
    (display "FAILED: ")
    (display e)
    (newline))
  (lambda ()
    (use-modules (ai ai-widget))
    (display "SUCCESS\n")))

; Test 4: Try to open AI panel
(display "Test 4: Opening AI panel... ")
(with-exception-handler
  (lambda (e)
    (display "FAILED: ")
    (display e)
    (newline))
  (lambda ()
    (show-ai-panel #t)
    (display "SUCCESS\n")))
