;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-llm.scm
;; DESCRIPTION : Initialize fake llm plugin (echo functionality with llm style)
;; COPYRIGHT   : (C) 2025 Darcy Shen
;;
;; MIT License
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (llm chat-loader))
(use-modules (dynamic session-edit) (binary goldfish))

(define (init-llm)
  (plugin-configure llm
    (:require (has-binary-goldfish?))
    (:launch ,(goldfish-plugin-launcher "llm"))
    (:serializer ,goldfish-plugin-serializer)
    (:session "LLM")
  ) ;plugin-configure

  (when (supports-llm?)
    (session-enable-text-input "llm" "default")
  ) ;when
) ;define

(init-llm)
