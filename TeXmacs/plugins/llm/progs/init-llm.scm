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

(import (liii path))

(define (llm-serialize lan t)
  ;; connection-write 调用前已做 tree_herk_to_utf8，传入的 stree 字符串是
  ;; UTF-8。% 开头的单字符串文档（%chat 协议行）须原样透传：object->string
  ;; 会给字符串加引号/转义并包 (document ...) 外壳，子进程将无法识别协议行
  (if (and (pair? t)
        (>= (length t) 2)
        (eq? (car t) 'document)
        (string? (cadr t))
        (string-starts? (cadr t) "%")
      ) ;and
    (string-append (cadr t) "\n<EOF>\n")
    (string-append (object->string t) "\n<EOF>\n")
  ) ;if
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
