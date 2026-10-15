
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
  (scheme char)
) ;import

;; 假附件插件不联网：收到 "%attachment {json}" ingest 请求后先按月清理缓存，
;; 再做插件侧预检（扩展名白名单/每轮数量/每轮总大小，违规回 error 事件），
;; 通过则回 ready 事件。假 sha256 由 path 字符串确定性散列生成；假提取文本
;; 按真实插件缓存约定写入 ~/.cache/Liiistem/attachment/extract/<yyyymm>/
;; <sha256>/result.json（二元数组 [{"id": sha256, "ts"}, {"content": …}]），
;; 供 llm 插件按 sha256 取用——llm 插件只认 sha256、不接触本地路径。

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
           "附件总大小超过上限（80 MiB）"
          ) ;
          (else #f)
    ) ;cond
  ) ;let
) ;define

;;; ---------- 抽取缓存：月度布局、命中刷新、月度清理（同真实插件约定） ----------

(define (attachment-extract-root)
  (path-join (path->string (path-home))
    ".cache"
    "Liiistem"
    "attachment"
    "extract"
  ) ;path-join
) ;define

(define (attachment-yyyymm year month)
  (string-append (number->string year)
    (if (< month 10)
      (string-append "0" (number->string month))
      (number->string month)
    ) ;if
  ) ;string-append
) ;define

(define (attachment-current-yyyymm)
  (date->string (current-date) "~Y~m")
) ;define

(define (attachment-previous-yyyymm)
  (let* ((d (current-date)) (y (date-year d)) (m (date-month d)))
    (if (= m 1) (attachment-yyyymm (- y 1) 12) (attachment-yyyymm y (- m 1)))
  ) ;let*
) ;define

(define (attachment-remove-tree dir)
  ;; 递归删除目录树（缓存为 extract/<yyyymm>/<sha256>/result.json 两层结构）
  (when (path-dir? dir)
    (vector-for-each
      (lambda (entry)
        (let ((sub (path-join (path->string dir) entry)))
          (if (path-dir? sub) (attachment-remove-tree sub) (path-unlink sub #t))
        ) ;let
      ) ;lambda
      (path-list dir)
    ) ;vector-for-each
    (path-rmdir dir)
  ) ;when
) ;define

(define (attachment-cache-sweep!)
  ;; 月度清理：删除 extract/ 下当月与上月之外的所有 yyyymm 目录；
  ;; 目录不存在跳过，多删不报错
  (let ((root (attachment-extract-root))
        (keep (list (attachment-current-yyyymm) (attachment-previous-yyyymm)))
       ) ;
    (when (path-dir? root)
      (vector-for-each
        (lambda (entry)
          (when (not (member entry keep))
            (attachment-remove-tree (path-join (path->string root) entry))
          ) ;when
        ) ;lambda
        (path-list root)
      ) ;vector-for-each
    ) ;when
  ) ;let
) ;define

(define (attachment-extract-cache-dir sha256)
  (path-join (attachment-extract-root) (attachment-current-yyyymm) sha256)
) ;define

(define (attachment-extract-cache-find sha256)
  ;; 当月 → 上月两候选目录查找缓存，命中返回 result.json 文本，否则 #f
  (let loop
    ((months (list (attachment-current-yyyymm) (attachment-previous-yyyymm))))
    (if (null? months)
      #f
      (let ((f (path-join (attachment-extract-root) (car months) sha256 "result.json")))
        (if (path-file? f) (path-read-text (path->string f)) (loop (cdr months)))
      ) ;let
    ) ;if
  ) ;let
) ;define

(define (fake-attachment-extracted-text filename)
  (string-append "[fake] extracted text of " filename)
) ;define

(define (attachment-fresh-payload sha256 filename ts content)
  (json->string (vector (list (cons "id" sha256) (cons "ts" ts))
                  (list (cons "content" content) (cons "filename" filename))
                ) ;vector
  ) ;json->string
) ;define

(define (fake-attachment-write-cache sha256 filename content)
  ;; 命中刷新（同真实插件约定）：命中时保留缓存原文、仅刷新 ts 重写至
  ;; 当月目录（命中文件可能在上月目录），热附件不被月度清理误删；
  ;; 未命中写入新缓存（content 仅此时使用）
  (let* ((dir (attachment-extract-cache-dir sha256))
         (cached (attachment-extract-cache-find sha256))
         (ts
           (number->string (inexact->exact (floor (current-second))))
         ) ;ts
         (j
           (and cached (catch #t (lambda () (string->json cached)) (lambda args #f)))
         ) ;j
         (payload
           (if (and j (vector? j) (= (vector-length j) 2))
             (json->string (vector (json-set (vector-ref j 0) "ts" ts) (vector-ref j 1)))
             (attachment-fresh-payload sha256 filename ts content)
           ) ;if
         ) ;payload
        ) ;
    (path-mkdir dir #t #t)
    (path-write-text (path->string (path-join dir "result.json")) payload)
  ) ;let*
) ;define

(define (fake-attachment-ready-event task-id file-path)
  ;; 参数名避开 path：(liii path) 的 path 构造函数在s7 下被字符串参数遮蔽后
  ;; (path x) 会被当作 string-ref 调用
  (let* ((p (path file-path))
         (sha256 (fake-attachment-sha256 file-path))
         (filename (path->string (path-name p)))
         (size (if (path-file? p) (path-getsize p) 0))
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
             (p (path file-path))
             (filename (path->string (path-name p)))
             (size (if (path-file? p) (path-getsize p) 0))
             (err (fake-attachment-precheck filename size count bytes))
            ) ;
        (if err
          (fake-attachment-error-event task-id err)
          (fake-attachment-ready-event task-id file-path)
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
