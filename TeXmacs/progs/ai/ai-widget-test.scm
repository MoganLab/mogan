;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ai-widget-test.scm
;; DESCRIPTION : Simplified AI Assistant widget for testing
;; COPYRIGHT   : (C) 2026 Mogan Team
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (ai ai-widget-test)
  (:use (kernel gui menu-define)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; State variables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define ai-test-visible? #f)
(define ai-test-message "Hello from AI!")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Simple Widget definition (without input for now)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (ai-simple-panel)
  (padded
    (vlist
      ;; Title bar
      (hlist
        (bold (text "AI Assistant (Test)"))
        >>
        (explicit-buttons 
          ("Close" (show-ai-simple-panel #f))))
      ===
      ;; Simple message display
      (scrollable
        (resize "300px" "200px"
          (vlist
            (text "This is a test panel.")
            ===
            (text "If you see this, the widget loads correctly!")))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Show/hide panel
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (show-ai-simple-panel flag)
  (set! ai-test-visible? flag)
  (if flag
      (begin
        (display* "[AI-TEST] Opening simple panel\n")
        (set-auxiliary-widget-state #t 'ai-simple-panel)
        (auxiliary-widget 
          (lambda (cmd) (ai-simple-panel))
          (lambda () (show-ai-simple-panel #f))
          "AI Test"))
      (begin
        (display* "[AI-TEST] Closing simple panel\n")
        (set-auxiliary-widget-state #f 'ai-simple-panel)
        (show-auxiliary-widget #f))))

(tm-define (toggle-ai-simple-panel)
  (show-ai-simple-panel (not ai-test-visible?)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Widget handlers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-ai-simple-panel-widget)
  (show-ai-simple-panel #t))

(tm-define (close-ai-simple-panel-widget)
  (show-ai-simple-panel #f))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keyboard shortcut
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map
  ("C-S-t" (toggle-ai-simple-panel)))  ; Ctrl+Shift+T
