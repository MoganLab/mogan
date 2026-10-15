
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-attachment.scm
;; DESCRIPTION : Fake attachment plugin (%attachment ingest/ready protocol)
;; COPYRIGHT   : (C) 2026 Mogan STEM
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
  (liii json)
  (liii string)
  (liii hashlib)
  (scheme char)
) ;import

;; 假附件插件不联网：收到 "%attachment {json}" ingest 请求后先按月清理缓存
;; （共享 attachment-cache 库，同真实插件约定），再做插件侧预检（扩展名
;; 白名单/每轮数量/每轮总大小，违规回 error 事件），通过则回 ready 事件。
;; 假 sha256 由 path 字符串确定性散列生成；假提取文本经共享缓存库落盘，
;; 供 llm 插件按 sha256 取用——llm 插件只认 sha256、不接触本地路径

;; 预检策略（真实插件遵从同一约定与 error 事件形态，liii 侧只遵从不定义）

(define attachment-max-count 10)

(define attachment-max-total-bytes (* 80 1024 1024))

(define attachment-allowed-extensions
  '("pdf" "doc" "docx" "xls" "xlsx" "ppt" "pptx" "md" "txt" "csv" "epub" "html"
    "json" "log" "c" "cpp" "h" "java" "py" "js" "ts" "go" "rs")
) ;define

(define (fake-attachment-sha256 file-path)
  (sha256 file-path)
) ;define

(define (fake-attachment-precheck filename size count bytes)
  ;; count/bytes 为请求上报的本轮已选累计。通过返回 #f，违规返回原因
  ;; 字符串（mogan UI 原样 toast，不再本地预检）
  (let ((ext
          (let ((s (path->string (path-suffix (path filename)))))
            (if (string-starts? s ".") (string-downcase (string-drop s 1)) "")
          ) ;let
        ) ;ext
       ) ;
    (cond ((not (member ext attachment-allowed-extensions))
           (string-append "不支持的附件类型： " filename)
          ) ;
          ((>= count attachment-max-count)
           (string-append "附件数量超过上限（"
             (number->string attachment-max-count)
             " 个）"
           ) ;string-append
          ) ;
          ((> (+ bytes size) attachment-max-total-bytes)
           (string-append "附件总大小超过上限（"
             (number->string (/ attachment-max-total-bytes 1024 1024))
             " MiB）"
           ) ;string-append
          ) ;
          (else #f)
    ) ;cond
  ) ;let
) ;define

(define (fake-attachment-extracted-text filename)
  (string-append "[fake] extracted text of " filename)
) ;define

(define (fake-attachment-ready-event task-id file-path filename size)
  ;; filename/size 由 ingest 统一计算传入（预检与事件共用，不重复求值）；
  ;; 参数名避开 path：(liii path) 的 path 构造函数在 s7 下被字符串参数
  ;; 遮蔽后 (path x) 会被当作 string-ref 调用
  (let* ((sha256 (fake-attachment-sha256 file-path))
         (content (fake-attachment-extracted-text filename))
        ) ;
    (attachment-cache-put! sha256 filename content)
    (json->string (list (cons "event" "ready")
                    (cons "taskId" task-id)
                    (cons "sha256" sha256)
                    (cons "filename" filename)
                    (cons "size" size)
                    (cons "extractedChars" (string-length content))
                  ) ;list
    ) ;json->string
  ) ;let*
) ;define

(define (fake-attachment-error-event task-id reason)
  (json->string (list (cons "event" "error") (cons "taskId" task-id) (cons "reason" reason))
  ) ;json->string
) ;define

(define (fake-attachment-ingest data)
  ;; 入口先月度清理缓存（同真实插件约定），再做插件侧预检（count/bytes
  ;; 为请求上报的本轮已选累计），通过则回 ready 事件；请求非法或预检
  ;; 违规回 error 事件
  (attachment-cache-sweep!)
  (let ((j
          (catch #t
            (lambda ()
              (string->json (string-trim (string-drop data (string-length "%attachment "))))
            ) ;lambda
            (lambda args #f)
          ) ;catch
        ) ;j
       ) ;
    (if (not (json-object? j))
      (fake-attachment-error-event "" "invalid ingest request")
      (let* ((task-id (json-ref-string j "taskId" ""))
             (file-path (json-ref-string j "path" ""))
             (count (json-ref-number j "count" 0))
             (bytes (json-ref-number j "bytes" 0))
             ;; 文件名与大小在此单点计算，预检与 ready 事件共用
             (p (path file-path))
             (filename (path->string (path-name p)))
             (size (if (path-file? p) (path-getsize p) 0))
             (err (fake-attachment-precheck filename size count bytes))
            ) ;
        (if err
          (fake-attachment-error-event task-id err)
          (fake-attachment-ready-event task-id file-path filename size)
        ) ;if
      ) ;let*
    ) ;if
  ) ;let
) ;define

(define (attachment-eval-and-print data)
  ;; 与 tm-llm 一致走 utf8: 通道回传 %attachment 协议行（单行单包，
  ;; mogan 侧一个 ingest 任务对应一个 ready/error 事件包）
  (flush-verbatim (if (string-starts? data "%attachment ")
                    (string-append "%attachment " (fake-attachment-ingest data))
                    data
                  ) ;if
  ) ;flush-verbatim
) ;define

(define (attachment-read-eval-print)
  (let ((data (read-paragraph-by-visible-eof)))
    (if (string=? data "") #t (attachment-eval-and-print data))
  ) ;let
) ;define

(define (attachment-main)
  (flush-prompt "attachment> ")
  (flush-verbatim "Attachment Plugin")
  (let loop
    ()
    (attachment-read-eval-print)
    (loop)
  ) ;let
) ;define

;; 测试加载本文件时预定义 *tm-attachment-test-mode* 以跳过 REPL
(if (not (defined? '*tm-attachment-test-mode*)) (attachment-main))
