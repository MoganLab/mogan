;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : claw-ai-widget.scm
;; DESCRIPTION : Claw AI panel using auxiliary-widget
;; COPYRIGHT   : (C) 2026  Gatsby
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (claw claw-ai-widget)
  (:use (kernel gui menu-define)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Chat state
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define claw-ai-messages '())
(define claw-ai-input "")
(define claw-ai-panel-opened #f)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Message management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (claw-ai-add-message role content)
  (set! claw-ai-messages 
    (append claw-ai-messages (list (cons role content)))))

(tm-define (claw-ai-clear-messages)
  (set! claw-ai-messages '())
  (refresh-now "claw-ai-chat"))

(tm-define (claw-ai-get-messages)
  claw-ai-messages)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Input handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (claw-ai-set-input text)
  (set! claw-ai-input text))

(tm-define (claw-ai-get-input)
  claw-ai-input)

(tm-define (claw-ai-send)
  ;; Mock send - just echo for now
  (when (not (string-null? claw-ai-input))
    (claw-ai-add-message 'user claw-ai-input)
    (claw-ai-add-message 'assistant 
      (string-append "Echo: " claw-ai-input))
    (set! claw-ai-input "")
    (refresh-now "claw-ai-chat")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Widget definition
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (claw-ai-chat-display)
  (refreshable "claw-ai-chat"
    (scrollable
      (resize "300px" "400px"
        (if (null? claw-ai-messages)
            (centered
              (grey (text "Start chatting with Claw AI")))
            (vlist
              (for-each
                (lambda (msg)
                  (with (role . content) msg
                    (vlist
                      (if (eq? role 'user)
                          (bold (text "You:"))
                          (bold (text "Claw AI:")))
                      (text content)
                      ===)))
                claw-ai-messages)))))))

(tm-widget (claw-ai-input-area)
  (hlist
    (resize "220px" "30px"
      (input (claw-ai-set-input answer) "string" 
             (list claw-ai-input) "1w"))
    >>
    (explicit-buttons ("Send" (claw-ai-send)))))

(tm-widget (claw-ai-toolbar)
  (hlist
    (explicit-buttons ("Clear" (claw-ai-clear-messages)))
    //
    (explicit-buttons ("Close" (show-auxiliary-widget #f)))))

(tm-widget (claw-ai-panel)
  (padded
    (vlist
      (bold (text "Claw AI Assistant"))
      ===
      (claw-ai-chat-display)
      ===
      (claw-ai-input-area)
      ===
      (claw-ai-toolbar))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Panel control
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-claw-ai)
  (set! claw-ai-panel-opened #t)
  (set-auxiliary-widget (claw-ai-panel) "Claw AI")
  (show-auxiliary-widget #t))

(tm-define (close-claw-ai)
  (set! claw-ai-panel-opened #f)
  (show-auxiliary-widget #f))

(tm-define (toggle-claw-ai)
  (if (auxiliary-widget-visible?)
      (close-claw-ai)
      (open-claw-ai)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keyboard shortcut
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map
  ("C-`" (toggle-claw-ai)))
