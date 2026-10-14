;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-protocol-test.scm
;; DESCRIPTION : 纯逻辑单元测试：%chat 协议 v2（images/attachments 字段、
;;               图片剥离序列化、v1 入口委托）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r chat-protocol-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(import (liii json))

(check-set-mode! 'report-failed)

;;; ---------- connection-defined? 间谍 ----------

;; 测试环境装有 llm 插件（连接已定义），会走真实 feed 路径并拉起 goldfish
;; 子进程。必须在模块加载前定义间谍，使 chat-tab-send 走「连接未定义」
;; 回显分支（s7 对 glue 函数的引用在模块加载时静态绑定，见 chat-list-test）

(tm-define (connection-defined? name) #f)

(load "./TeXmacs/plugins/llm/progs/llm/chat-protocol.scm")

;;; ---------- chat-input->json：images/attachments 编码 ----------

(define (make-ctx input images attachments)
  (make-chat-input input "sid-json" "model-x" "" "disabled" "disabled" "medium"
    images attachments
  ) ;make-chat-input
) ;define

;; 空 images/attachments：字段恒在，输出空数组

(define (test-json-empty-arrays)
  (let* ((ctx (make-ctx '(document "hello") '() '()))
         (parsed (string->json (json->string (chat-input->json ctx))))
         (images (json-ref parsed "images"))
         (attachments (json-ref parsed "attachments"))
        ) ;
    (check (json-ref-string parsed "sessionId" #f) => "sid-json")
    (check (vector? images) => #t)
    (check (vector-length images) => 0)
    (check (vector? attachments) => #t)
    (check (vector-length attachments) => 0)
  ) ;let*
) ;define

;; 非空 images/attachments：每项按键编码为 JSON 对象

(define (test-json-nonempty-arrays)
  (let* ((images-in (list (list (cons "path" "/tmp/a.png") (cons "mime" "image/png"))))
         (attachments-in (list (list (cons "path" "/tmp/b.pdf") (cons "filename" "b.pdf") (cons "size" 1024))
                         ) ;list
         ) ;attachments-in
         (ctx (make-ctx '(document "hi") images-in attachments-in))
         (parsed (string->json (json->string (chat-input->json ctx))))
         (images (json-ref parsed "images"))
         (attachments (json-ref parsed "attachments"))
        ) ;
    (check (vector-length images) => 1)
    (let ((img0 (vector-ref images 0)))
      (check (json-ref-string img0 "path" #f) => "/tmp/a.png")
      (check (json-ref-string img0 "mime" #f) => "image/png")
    ) ;let
    (check (vector-length attachments) => 1)
    (let ((att0 (vector-ref attachments 0)))
      (check (json-ref-string att0 "path" #f) => "/tmp/b.pdf")
      (check (json-ref-string att0 "filename" #f) => "b.pdf")
      (check (json-ref-integer att0 "size" 0) => 1024)
    ) ;let
  ) ;let*
) ;define

;;; ---------- chat-tab-tree-strip-images ----------

;; 纯文本原样保留

(define (test-strip-plain-text)
  (check (chat-tab-tree-strip-images '(document "hello" "world"))
    =>
    '(document "hello" "world")
  ) ;check
) ;define

;; 含图：图片替换为空串，文本与段落数保持

(define (test-strip-image-document)
  (let ((stripped (chat-tab-tree-strip-images '(document "before"
                                                 (image "x.png" "1cm" "1cm" ""
                                                   "")
                                                 "after")
                  ) ;chat-tab-tree-strip-images
        ) ;stripped
       ) ;
    (check stripped => '(document "before" "" "after"))
    (check (length stripped) => 4)
  ) ;let
) ;define

;; with 壳内嵌图同样剥离，壳结构保留

(define (test-strip-image-in-with)
  (check
    (chat-tab-tree-strip-images '(document (with "font-series"
                                             "bold"
                                             "a"
                                             (image "y.png" "1cm" "1cm" "" "")
                                             "b"))
    ) ;chat-tab-tree-strip-images
    =>
    '(document (with "font-series" "bold" "a" "" "b"))
  ) ;check
) ;define

;;; ---------- chat-tab-tree->plain-text：含图输入不出怪文本 ----------

(define (test-plain-text-with-image)
  (let ((text (chat-tab-tree->plain-text '(document "see "
                                            (image "x.png" "1cm" "1cm" "" "")
                                            " done")
              ) ;chat-tab-tree->plain-text
        ) ;text
       ) ;
    (check (string-contains? text "includegraphics") => #f)
    (check (string-contains? text "see") => #t)
    (check (string-contains? text "done") => #t)
  ) ;let
) ;define

;;; ---------- chat-tab-send（连接未定义分支回显） ----------

;; 递归查 stree 中是否出现指定文本（chat-tab-flatten-stree 为模块私有）

(define (stree-contains-text? t sub)
  (cond ((string? t) (string-contains? t sub))
        ((pair? t)
         (let loop
           ((rest (cdr t)))
           (if (null? rest)
             #f
             (or (stree-contains-text? (car rest) sub) (loop (cdr rest)))
           ) ;if
         ) ;let
        ) ;
        (else #f)
  ) ;cond
) ;define

;; 8 参入口携带 images/attachments：connection-defined? 为假时走回显分支
;; （追加一轮并回显输入、返回 #t）。输入区清空依赖 buffer 视图，headless
;; 无视图属既有行为，不在本测试断言

(define (test-chat-tab-send-with-attachments)
  (let* ((sid "test-send-v2")
         (in-buf (chat-tab-session->input-buffer sid))
         (msg-buf (chat-tab-session->message-buffer sid))
        ) ;
    (buffer-set-body in-buf '(document "hello v2"))
    (check
      (chat-tab-send sid
        "model-x"
        ""
        "disabled"
        "disabled"
        "medium"
        (list (list (cons "path" "/tmp/a.png") (cons "mime" "image/png")))
        (list (list (cons "path" "/tmp/b.pdf") (cons "filename" "b.pdf") (cons "size" 1024))
        ) ;list
      ) ;chat-tab-send
      =>
      #t
    ) ;check
    (check (stree-contains-text? (tm->stree (buffer-get-body msg-buf)) "hello v2")
      =>
      #t
    ) ;check
  ) ;let*
) ;define

;;; ---------- 测试套件入口 ----------

(tm-define (regtest-chat-protocol)
  (test-json-empty-arrays)
  (test-json-nonempty-arrays)
  (test-strip-plain-text)
  (test-strip-image-document)
  (test-strip-image-in-with)
  (test-plain-text-with-image)
  (test-chat-tab-send-with-attachments)
  (check-report)
) ;tm-define
