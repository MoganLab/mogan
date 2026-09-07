
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

(import (texmacs protocol) (liii path) (liii uuid) (liii json) (liii string))

(define (welcome)
  (flush-prompt "llm> ")
  (flush-verbatim "LLM Plugin")
) ;define

(define *large-data-threshold* 1048576)

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
            (params (catch #t (lambda () (json-ref j "params")) (lambda args #f)))
           ) ;
        (if (not (json-object? params))
          #f
          (string-append "%chat "
            (json->string (json-set j
                            "content"
                            (string-append "[fake-llm] 我收到了你的消息：" content)
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

(welcome)
(repl)
