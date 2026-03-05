;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : build-glue-claw-ai.scm
;; DESCRIPTION : Build glue for Claw AI integration
;; COPYRIGHT   : (C) 2026  Gatsby
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (kernel gui menu-define))

;; Load the glue builder
(load "build-glue.scm")

;; Build Claw AI glue
(build-glue
  "glue_claw_ai"
  "src/Scheme/Glue/glue_claw_ai.lua"
  "src/Scheme/Glue/glue_claw_ai.cpp")

(display "Claw AI glue built successfully!\n")
