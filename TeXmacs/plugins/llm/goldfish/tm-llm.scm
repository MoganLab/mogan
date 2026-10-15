
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-llm.scm
;; DESCRIPTION : LLM plugin (eval-and-print with code environment support)
;; COPYRIGHT   : (C) 2025 Darcy Shen
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;     http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (texmacs protocol)
  (texmacs attachment-cache)
  (liii path)
  (liii os)
  (liii uuid)
  (liii json)
  (liii string)
) ;import

(define (welcome)
  (flush-prompt "llm> ")
  (flush-verbatim "LLM Plugin")
) ;define

(define *large-data-threshold* 1048576)

(define (localized key)
  `(localize ,key)
) ;define

(define (reasoning-title-tree)
  `(with ,"color"
     ,"dark grey"
     ,"font-size"
     ,"0.92"
     ,"font-series"
     ,"medium"
     (concat (math "<blacktriangledown>") ," " ,(localized "View reasoning")))
) ;define

(define (reasoning-body-tree content)
  `(with ,"color"
     ,"dark grey"
     ,"font-size"
     ,"0.92"
     ,"font-shape"
     ,"italic"
     (document ,content))
) ;define

(define *fake-reasoning-content*
  "这是一段用于测试样式问题的推理过程文本。我们正在验证大语言模型插件在输出长文本推理内容时的排版与折叠表现，包括文字换行、行高、缩进、折叠按钮以及字体颜色等渲染细节是否正常。通过不换行的连续长文本输入，可以充分测试视图边界处的自动折叠换行机制，确保在各种窗口尺寸和缩放比例下，推理过程块的边框、背景及文字内容均能正确自适应布局，既不会超出容器边界，也不会出现文字截断或排版错乱的问题，从而为用户提供稳定美观的阅读体验。"
) ;define

(define *llm-log-buf* "")

(define (llm-log . parts)
  (set! *llm-log-buf*
    (string-append *llm-log-buf* (apply string-append parts) "\n")
  ) ;set!
  (path-write-text (path-join (path->string (path-temp-dir)) "llm-trace.log")
    *llm-log-buf*
  ) ;path-write-text
) ;define

;; 验收用调试日志：设置环境变量 TM_LLM_CHAT_DEBUG 后，收到的 %chat 线格式
;; 原样追加写入 /tmp/ai-chat-debug.log（假插件专用，默认关闭避免常驻写盘；
;; /tmp 不存在时静默忽略）

(define (ai-chat-debug-log line)
  (when (getenv "TM_LLM_CHAT_DEBUG")
    (catch #t
      (lambda ()
        (path-append-text "/tmp/ai-chat-debug.log" (string-append line "\n"))
      ) ;lambda
      (lambda args #f)
    ) ;catch
  ) ;when
) ;define

;; 附件解耦闭环：%chat v2 的 attachments 只含 sha256（及展示元数据），
;; 假 llm 插件按 sha256 从附件抽取缓存（共享 (texmacs attachment-cache)
;; 库，布局与查找约定单点定义）取假文本，全程不接触本地路径

(define (fake-llm-attachment-injections attachments)
  ;; attachments 为 JSON 数组（vector）；每项注入一行回显
  (if (not (vector? attachments))
    ""
    (let loop
      ((i 0) (acc ""))
      (if (>= i (vector-length attachments))
        acc
        (let* ((item (vector-ref attachments i))
               (sha256 (if (json-object? item) (json-ref-string item "sha256" "") ""))
               (content (and (> (string-length sha256) 0) (attachment-cache-payload-content sha256))
               ) ;content
              ) ;
          (loop (+ i 1)
            (if content
              (string-append acc "\n[fake] attachment " sha256 " injected: " content)
              acc
            ) ;if
          ) ;loop
        ) ;let*
      ) ;if
    ) ;let
  ) ;if
) ;define


;; reasoning 主体必须内联在返回的 unfolded-explain 树里：reasoning-delta /
;; fold-explain-reasoning 是真实流式插件的边通道标记，需 mogan 侧拦截拼接，
;; 假插件经它们发出的 reasoning 不会进入最终返回的文档

(define (flush-fake-reasoning text)
  (llm-log "plugin: flush-fake-reasoning len="
    (number->string (string-length text))
  ) ;llm-log
  ;; 末尾空段：C++ 输入累积会把相邻非空节点并入同一 concat，
  ;; 留空段让后续输出（%chat 回显）另起一段
  (flush-scheme
    `(document (unfolded-explain ,(reasoning-title-tree)
                 ,(reasoning-body-tree text))
       ,"")
  ) ;flush-scheme
) ;define

(define (llm-write-temp-file data)
  (let* ((tmp-dir (path-temp-dir))
         (tmp-name (uuid4))
         (tmp-path (path-join (path->string tmp-dir) tmp-name))
        ) ;
    (path-write-text tmp-path data)
    tmp-path
  ) ;let*
) ;define

;; 假插件不联网：把收到的 "%chat {json}" 协议行原样回传——content 换成
;; 假回复文本，params/sessionId 原样保留，供无网络环境验证协议字段
;; 双向链路；解析失败返回 #f，由调用方回退原文回显

(define (fake-llm-chat-reply data)
  (let* ((payload (string-trim (string-drop data (string-length "%chat "))))
         (j (catch #t (lambda () (string->json payload)) (lambda args #f)))
        ) ;
    (if (not (json-object? j))
      #f
      (let ((content (json-ref-string j "content" ""))
            ;; 缺键时 json-ref 返回 ()，由下方 json-object? 判定兜住
            (params (json-ref j "params"))
           ) ;
        (if (not (json-object? params))
          #f
          (string-append "%chat "
            (json->string
              (json-set j
                "content"
                (string-append "[fake-llm] 我收到了你的消息："
                  content
                  ;; 缺键时 json-ref 返回 ()，由 injections 的 vector? 判定兜住
                  (fake-llm-attachment-injections (json-ref j "attachments"))
                ) ;string-append
              ) ;json-set
            ) ;json->string
          ) ;string-append
        ) ;if
      ) ;let
    ) ;if
  ) ;let*
) ;define

(define (eval-and-print data)
  ;; 文本回显一律走 utf8: 通道：scheme: 通道要求负载是表示树的 scheme
  ;; 代码，自由文本会被解析成「首词作树标签」的畸形树，渲染只剩标签且
  ;; 打断本轮完成（超时）。旧 echo 侥幸可用是因为旧协议行恰好形如
  ;; (document "...")，本身即是合法树代码
  (if (> (string-length data) *large-data-threshold*)
    (flush-verbatim (llm-write-temp-file data))
    (let ((reply (and (string-starts? data "%chat ") (fake-llm-chat-reply data))))
      (when reply
        (ai-chat-debug-log data)
        (flush-fake-reasoning *fake-reasoning-content*)
      ) ;when
      (flush-verbatim (or reply data))
    ) ;let
  ) ;if
) ;define

(define (read-eval-print)
  (let ((data (read-paragraph-by-visible-eof)))
    (if (string=? data "") #t (eval-and-print data))
  ) ;let
) ;define

(define (repl)
  (read-eval-print)
  (repl)
) ;define

;; 测试加载本文件时预定义 *tm-llm-test-mode* 以跳过 REPL
(if (not (defined? '*tm-llm-test-mode*)) (begin (welcome) (repl)))
