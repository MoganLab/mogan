;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ai-widget.scm
;; DESCRIPTION : AI Assistant panel widget for Mogan (MINIMAL VERSION)
;; COPYRIGHT   : (C) 2026 Mogan Team
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (ai ai-widget)
  (:use (kernel gui menu-define)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; State variables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define ai-panel-visible? #f)  ; Panel visibility state

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Minimal Widget definition (USING ONLY PROVEN COMPONENTS)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (ai-dialog-panel)
  (padded
    (vlist
      ;; Title
      (hlist
        (bold (text "AI Assistant"))
        >>
        (explicit-buttons 
          ("Close" (show-ai-panel #f))))
      ===
      ;; Content - using only basic components
      (scrollable
        (resize "300px" "400px"
          (vlist
            (text "AI Assistant Panel")
            ===
            (text "")
            ===
            (grey (text "This is a simple test panel."))
            ===
            (grey (text "If you can see this, the UI works!"))
            ===
            (text "")
            ===
            (text "Features to be added:")
            ===
            (text "  - Chat interface")
            ===
            (text "  - Message history")
            ===
            (text "  - AI integration")))))))

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
  ("std i" (toggle-ai-panel)))
