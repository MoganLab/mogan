;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-ai.scm
;; DESCRIPTION : Initialize AI Assistant module
;; COPYRIGHT   : (C) 2026 Mogan Team
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (ai init-ai)
  (:use (ai ai-widget)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Module initialization code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Debug output for module loading
(display "[AI] Loading AI Assistant module (simplified version)...\n")

;; Register widget type on startup
(register-auxiliary-widget-type 'ai-panel 
  (list open-ai-panel-widget close-ai-panel-widget))

(display "[AI] AI Assistant module loaded successfully.\n")
