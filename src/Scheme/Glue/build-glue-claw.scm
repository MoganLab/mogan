;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : build-glue-claw.scm
;; DESCRIPTION : Building glue for Claw AI integration
;; COPYRIGHT   : (C) 2026  Gatsby
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(output-copyright "build-glue-claw.scm")

(build
  "get_server()->"
  "initialize_glue_claw"
  
  ;; Panel visibility control
  (show-claw-ai-panel show_claw_ai_panel (void bool))
  (claw-ai-panel-visible? claw_ai_panel_visible (bool))
  
  ;; AI communication
  (claw-ai-send claw_ai_send (string string)))
