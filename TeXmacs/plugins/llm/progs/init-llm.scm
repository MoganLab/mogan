;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-llm.scm
;; DESCRIPTION : Initialize fake llm plugin (echo functionality with llm style)
;; COPYRIGHT   : (C) 2025 Darcy Shen
;;
;; MIT License
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (dynamic session-edit)
             (binary goldfish))

(import (liii path))

(define (llm-serialize lan t)
  (string-append (object->string t) "\n<EOF>\n"))

(define (llm-launcher)
  (let* ((home (path-from-env "TEXMACS_HOME_PATH"))
         (sys (path-from-env "TEXMACS_PATH"))
         (user (path-join home "plugins" "llm" "goldfish" "tm-llm.scm"))
         (sys-path (path-join sys "plugins" "llm" "goldfish" "tm-llm.scm"))
         (entry (if (url-exists? (path->string user))
                    (path->string user)
                    (path->string sys-path))))
    (string-append (string-quote (url->system (find-binary-goldfish)))
                   " -l "
                   (string-quote (url->system entry)))))

(plugin-configure llm
  (:require (has-binary-goldfish?))
  (:launch ,(llm-launcher))
  (:serializer ,llm-serialize)
  (:session "LLM"))

(when (supports-llm?)
  (session-enable-math-input "llm" "default"))
