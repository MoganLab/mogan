;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-ai-test.scm
;; DESCRIPTION : Initialize AI Assistant test module
;; COPYRIGHT   : (C) 2026 Mogan Team
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (ai init-ai-test)
  (:use (ai ai-widget-test)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Module initialization code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Debug output for module loading
(display "[AI-TEST] Loading AI Test module...\n")

;; Register widget type on startup
(display "[AI-TEST] Registering AI test panel widget type...\n")
(register-auxiliary-widget-type 'ai-simple-panel 
  (list open-ai-simple-panel-widget close-ai-simple-panel-widget))

(display "[AI-TEST] AI Test module loaded successfully.\n")
