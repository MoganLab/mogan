;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ai-widget.scm
;; DESCRIPTION : AI Assistant panel widget for Mogan (WITH CHAT FUNCTIONALITY)
;; COPYRIGHT   : (C) 2026 Mogan Team
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (ai ai-widget)
  (:use (kernel gui menu-define)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; State variables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define ai-panel-visible? #f)      ; Panel visibility state
(define ai-current-input "")       ; Current input text
(define ai-message-count 0)        ; Simple counter for messages sent
(define ai-message-history         ; Message history: ((role content) ...)
  '((user "你好")
    (assistant "你好！有什么可以帮助你的？")
    (user "如何学习 Scheme？")
    (assistant "多练习，多写代码！")))
;; No global flag variable needed - using local variable in with binding

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Helper functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (ai-update-input text)
  ;; Update current input
  (set! ai-current-input text))

(tm-define (ai-send-message)
  ;; Send current input to AI (placeholder - just for testing input)
  (when (not (string-null? ai-current-input))
    ;; Add to history
    (set! ai-message-history 
      (append ai-message-history (list (list 'user ai-current-input))))
    
    ;; Increment message count
    (set! ai-message-count (+ ai-message-count 1))
    
    ;; Clear input
    (set! ai-current-input "")
    
    ;; Refresh UI
    (refresh-now "ai-chat")))

(tm-define (tm-chat-add-message role content)
  ;; Add message to C++ chat widget via widget slot
  ;; This is a placeholder - real implementation would send to widget
  (display* "[Scheme] Adding message: " role " -> " content "\n"))

(tm-define (tm-chat-clear)
  ;; Clear all messages in chat widget
  (display* "[Scheme] Clearing chat\n"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main Widget definition - Using new C++ component
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (ai-dialog-panel)
  (padded
    (vlist
      ;; Title bar
      (hlist
        (bold (text "AI Assistant"))
        >>
        (explicit-buttons 
          ("Close" (show-ai-panel #f))))
      ===
      ;; Message list using new C++ component (no more refreshable!)
      (resize "500px" "700px"
        (widget-chat-messages))
      ===
      ;; Input area
      (hlist
        (resize "300px" "50px"
          (input (ai-update-input answer) "string" 
                 (list ai-current-input) "300px"))
        >>
        (explicit-buttons 
          ("Send" 
           (when (not (string-null? ai-current-input))
             ;; Call C++ component to add message
             (tm-chat-add-message "user" ai-current-input)
             
             ;; Also keep local history for now (for debugging)
             (set! ai-message-history 
               (append ai-message-history 
                       (list (list 'user ai-current-input))))
             
             ;; Clear input
             (set! ai-current-input ""))))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Show/hide panel
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (show-ai-panel flag)
  ;; Show or hide the AI panel
  (set! ai-panel-visible? flag)
  (if flag
      (begin
        (display* "[AI] Opening AI panel\n")
        (set-auxiliary-widget-state #t 'ai-panel)
        (auxiliary-widget 
          (lambda (cmd) (ai-dialog-panel))
          (lambda () (show-ai-panel #f))
          "AI Assistant"))
      (begin
        (display* "[AI] Closing AI panel\n")
        (set-auxiliary-widget-state #f 'ai-panel)
        (show-auxiliary-widget #f))))

(tm-define (toggle-ai-panel)
  ;; Toggle panel visibility
  (show-ai-panel (not ai-panel-visible?)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Widget handlers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-ai-panel-widget)
  (show-ai-panel #t))

(tm-define (close-ai-panel-widget)
  (show-ai-panel #f))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keyboard shortcut
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map
  ("A-C-i" (toggle-ai-panel)))
