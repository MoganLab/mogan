;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-llm.scm
;; DESCRIPTION : Initialize fake llm plugin (echo functionality with llm style)
;; COPYRIGHT   : (C) 2025 Darcy Shen
;;
;; MIT License
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (dynamic session-edit) (binary goldfish))

(import (liii path))

(define (llm-serialize lan t)
  (string-append (object->string t) "\n<EOF>\n")
) ;define

(define (llm-launcher)
  (let* ((home (path-from-env "TEXMACS_HOME_PATH"))
         (sys (path-from-env "TEXMACS_PATH"))
         (user (path-join home "plugins" "llm" "goldfish" "tm-llm.scm"))
         (sys-path (path-join sys "plugins" "llm" "goldfish" "tm-llm.scm"))
         (entry (if (url-exists? (path->string user))
                  (path->string user)
                  (path->string sys-path)
                ) ;if
         ) ;entry
        ) ;
    (string-append (string-quote (url->system (find-binary-goldfish)))
      " load "
      (string-quote (url->system entry))
    ) ;string-append
  ) ;let*
) ;define

(define (init-llm)
  (plugin-configure llm
    (:require (has-binary-goldfish?))
    (:launch ,(llm-launcher))
    (:serializer ,llm-serialize)
    (:session "LLM")
  ) ;plugin-configure

  (when (supports-llm?)
    (session-enable-text-input "llm" "default")
  ) ;when
) ;define

(init-llm)
