;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-ghost.scm
;; DESCRIPTION : 初始化 Ghost 自动补全插件
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (binary goldfish))
(import (liii os))
(import (liii json))

(set! *load-path*
  (let ((user-path (string-append (getenv "TEXMACS_HOME_PATH") "/plugins/ghost/goldfish"))
        (sys-path (string-append (getenv "TEXMACS_PATH") "/plugins/ghost/goldfish")))
    (if (file-exists? user-path)
        (cons user-path *load-path*)
        (cons sys-path *load-path*))
  ) ;let
) ;set!

(import (liii ghost-context))
(import (liii ghost-complete))

;; 启动后台 Goldfish 守护进程的命令
(define (ghost-launcher)
  (let* ((user "$TEXMACS_HOME_PATH/plugins/ghost/goldfish/tm-ghost.scm")
         (sys "$TEXMACS_PATH/plugins/ghost/goldfish/tm-ghost.scm")
         (entry (if (url-exists? user) user sys))
        ) ;
    (string-append (string-quote (url->system (find-binary-goldfish)))
      " "
      (string-quote (url->system entry))
    ) ;string-append
  ) ;let*
) ;define

;; 序列化输入数据供守护进程读取
(define (ghost-serialize lan t)
  (with u
    (pre-serialize lan t)
    (string-append (texmacs->utf8raw (stree->tree u)) "\n<EOF>\n"))
) ;define

(plugin-configure ghost
  (:require (has-binary-goldfish?))
  (:launch ,(ghost-launcher))
  (:serializer ,ghost-serialize)
) ;plugin-configure

;; 暴露给 Mogan 前端的异步自动补全接口，完全非阻塞
(tm-define (ghost-cloud-predict prefix suffix callback)
  (let* ((payload (json->string `((,"prefix" unquote prefix)
                                  (,"suffix" unquote suffix)))))
    (silent-feed* "ghost" "default" `(document ,payload)
      (lambda (result)
        (let* ((res-str (if (tree? result)
                          (if (tree-atomic? result)
                            (tree->string result)
                            (tree->string (tree-ref result 0)))
                          result))
               (j (catch #t (lambda () (string->json res-str)) (lambda args #f))))
          (callback (if (and j (json-object? j) (json-ref j "tokens"))
                      (cons (json-ref j "tokens") (json-ref j "logprobs"))
                      #f))))
      '()))
) ;tm-define
