;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-tree-ops-test.scm
;; DESCRIPTION : 纯逻辑单元测试：chat-tab-source-doc-info 来源文档身份解析
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r chat-tree-ops-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/plugins/llm/progs/llm/chat-tree-ops.scm")

;;; ---------- chat-tab-source-doc-info ----------

;; 与 chat-tab-source-doc-info 同规则的文件名：url 末段去扩展名

(define (expected-name u)
  (let* ((tail (url->system (url-tail u))) (suffix (url-suffix u)))
    (if (== suffix "")
      tail
      (substring tail 0 (- (string-length tail) (string-length suffix) 1))
    ) ;if
  ) ;let*
) ;define

;; 普通文档：master 是其自身，直接读到 init-env 绑定的 stem-doc-id

(define (test-plain-document)
  (let ((doc (buffer-new)))
    (switch-to-buffer doc)
    (with-buffer doc (init-env "stem-doc-id" "DOC-ID-PLAIN"))
    (check (chat-tab-source-doc-info) => (cons "DOC-ID-PLAIN" (expected-name doc)))
  ) ;let
) ;define

;; 焦点在聊天 buffer（master 指向来源文档）：解析到文档身份而非聊天 buffer
;; 尾段（回归：曾返回 ("" . "message")，按钮永远找不到文档的专属会话）。
;; doc-id 走 initial collection 写入：headless 下文档 buffer 无视图，
;; init-env 路径依赖社区 with-buffer 聚焦成功（真实 GUI 中文档必有视图）

(define (test-from-chat-buffer)
  (let* ((doc (buffer-new)) (chat (buffer-new)))
    (buffer-set doc
      `(document (TeXmacs ,(texmacs-version))
         (style (tuple "generic"))
         (body (document ""))
         (initial (collection (associate "stem-doc-id" "DOC-ID-CHAT"))))
    ) ;buffer-set
    (buffer-set-master chat doc)
    (switch-to-buffer chat)
    (check (chat-tab-source-doc-info) => (cons "DOC-ID-CHAT" (expected-name doc)))
  ) ;let*
) ;define

;; 文档未绑定 stem-doc-id：doc-id 为 ""，不生成新 id

(define (test-unbound-doc-id)
  (let ((doc (buffer-new)))
    (switch-to-buffer doc)
    (check (car (chat-tab-source-doc-info)) => "")
  ) ;let
) ;define

;;; ---------- 测试套件入口 ----------

(tm-define (regtest-chat-tree-ops)
  (test-plain-document)
  (test-from-chat-buffer)
  (test-unbound-doc-id)
  (check-report)
) ;tm-define
