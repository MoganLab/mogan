
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
  (liii path)
  (liii os)
  (liii json)
  (liii string)
  (liii hashlib)
  (liii time)
  (scheme time)
) ;import

;; 假附件插件不联网：收到 "%attachment {json}" ingest 请求后立即回 ready
;; 事件。假 sha256 由 path 字符串确定性散列生成；假提取文本按真实插件
;; 缓存约定写入 ~/.cache/Liiistem/attachment/extract/<yyyymm>/<sha256>/
;; result.json（二元数组 [{"id": sha256, "ts"}, {"content": …}]），供
;; llm 插件按 sha256 取用——llm 插件只认 sha256、不接触本地路径。

(define (fake-attachment-sha256 file-path)
  (sha256 file-path)
) ;define

(define (attachment-extract-cache-dir sha256)
  (path-join (path->string (path-home))
    ".cache"
    "Liiistem"
    "attachment"
    "extract"
    (date->string (current-date) "~Y~m")
    sha256
  ) ;path-join
) ;define

(define (fake-attachment-extracted-text filename)
  (string-append "[fake] extracted text of " filename)
) ;define

(define (fake-attachment-write-cache sha256 filename content)
  (let* ((dir (attachment-extract-cache-dir sha256))
         (ts
           (number->string (inexact->exact (floor (current-second))))
         ) ;ts
         (payload
           (json->string (vector (list (cons "id" sha256) (cons "ts" ts))
                           (list (cons "content" content) (cons "filename" filename))
                         ) ;vector
           ) ;json->string
         ) ;payload
        ) ;
    ;; mkdir 对已有目录报 file-exists-error，先判存在
    (when (not (path-exists? dir))
      (mkdir (path->string dir))
    ) ;when
    (path-write-text (path->string (path-join dir "result.json")) payload)
  ) ;let*
) ;define

(define (fake-attachment-ready-event task-id file-path)
  ;; 参数名避开 path：(liii path) 的 path 构造函数在s7 下被字符串参数遮蔽后
  ;; (path x) 会被当作 string-ref 调用
  (let* ((sha256 (fake-attachment-sha256 file-path))
         (filename (path->string (path-name (path file-path))))
         (size (if (path-file? (path file-path)) (path-getsize (path file-path)) 0))
         (content (fake-attachment-extracted-text filename))
        ) ;
    (fake-attachment-write-cache sha256 filename content)
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

(define (fake-attachment-ingest data)
  ;; 返回 ready 事件的 JSON 字符串；请求非法时回 error 事件
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
      (json->string (list (cons "event" "error")
                      (cons "taskId" "")
                      (cons "reason" "invalid ingest request")
                    ) ;list
      ) ;json->string
      (fake-attachment-ready-event (json-ref-string j "taskId" "")
        (json-ref-string j "path" "")
      ) ;fake-attachment-ready-event
    ) ;if
  ) ;let
) ;define

(define (attachment-eval-and-print data)
  ;; 与 tm-llm 一致走 utf8: 通道回传 %attachment 协议行（单行单包，
  ;; mogan 侧一个 ingest 任务对应一个 ready/error 事件包）
  (if (string-starts? data "%attachment ")
    (flush-verbatim (string-append "%attachment " (fake-attachment-ingest data)))
    (flush-verbatim data)
  ) ;if
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
