;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : claw-ai-widget.scm
;; DESCRIPTION : Claw AI panel widget for Mogan
;; COPYRIGHT   : (C) 2026  Gatsby
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (claw claw-ai-widget)
  (:use (kernel gui menu-define)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Chat history storage
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define claw-ai-chat-history '())
(define claw-ai-current-input "")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Helper functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (claw-ai-add-message role content)
  ;; Add a message to chat history
  ;; role: 'user or 'assistant
  (set! claw-ai-chat-history 
    (append claw-ai-chat-history (list (list role content)))))

(tm-define (claw-ai-clear-history)
  ;; Clear chat history
  (set! claw-ai-chat-history '()))

(tm-define (claw-ai-format-history)
  ;; Format history for display
  (with result ""
    (for-each 
      (lambda (msg)
        (with (role content) msg
          (set! result 
            (string-append result 
              (if (eq? role 'user) "You: " "AI: ")
              content "\n\n"))))
      claw-ai-chat-history)
    result))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Send message handler
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (claw-ai-send-message)
  ;; Send current input to AI
  (when (not (string-null? claw-ai-current-input))
    ;; Add user message to history
    (claw-ai-add-message 'user claw-ai-current-input)
    
    ;; Send to AI (sync for now)
    (with response (claw-ai-send claw-ai-current-input)
      ;; Add AI response to history
      (claw-ai-add-message 'assistant response))
    
    ;; Clear input
    (set! claw-ai-current-input "")
    
    ;; Refresh UI
    (refresh-now "claw-ai-chat")))

(tm-define (claw-ai-update-input text)
  ;; Update current input
  (set! claw-ai-current-input text))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Widget definition
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (claw-ai-panel)
  (padded
    (vlist
      ;; Title
      (bold (text "Claw AI Assistant"))
      ===
      ;; Chat history display
      (refreshable "claw-ai-chat"
        (scrollable
          (resize "280px" "400px"
            (if (null? claw-ai-chat-history)
                (grey (text "Start a conversation with Claw AI..."))
                (verbatim (text (claw-ai-format-history)))))))
      ===
      ;; Input area
      (hlist
        (resize "200px" "30px"
          (input (claw-ai-update-input answer) "string" 
                 (list claw-ai-current-input) "1w"))
        >>
        (explicit-buttons 
          ("Send" (claw-ai-send-message))))
      ===
      ;; Action buttons
      (hlist
        (explicit-buttons 
          ("Clear" (begin 
                     (claw-ai-clear-history)
                     (refresh-now "claw-ai-chat"))))
        //
        (explicit-buttons 
          ("Close" (show-claw-ai-panel #f)))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Show/hide panel
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (toggle-claw-ai-panel)
  ;; Toggle panel visibility
  (show-claw-ai-panel (not (claw-ai-panel-visible?))))

(tm-define (open-claw-ai-panel)
  ;; Open the Claw AI panel
  (show-claw-ai-panel #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keyboard shortcut
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map
  ("C-`" (toggle-claw-ai-panel)))
