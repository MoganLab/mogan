
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : attachment-cache.scm
;; DESCRIPTION : 附件抽取缓存的磁盘契约（mogan 侧假插件共享，真实插件
;;               遵从同一约定）
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

;; 布局：<缓存基目录>/Liiistem/attachment/extract/<yyyymm>/<sha256>/
;; result.json，内容为二元数组 [{"id": sha256, "ts"}, {"content": …}]。
;; 缓存基目录按平台惯例解析（对齐 mogan C++ get_tm_cache_path 的分布）：
;; Windows %LOCALAPPDATA%、macOS ~/Library/Caches、Linux XDG_CACHE_HOME
;; （未设置回退 ~/.cache）。保留当月与上月；读取方不知道 yyyymm，按
;; 「当前月 → 上个月」两候选目录查找。写入方与读取方（llm 假插件）共用
;; 本库，避免布局约定各自复制后漂移

(define-library (texmacs attachment-cache)
  (import (liii path) (liii os) (liii json) (liii time) (scheme time))
  (export attachment-extract-root attachment-extract-cache-dir
    attachment-cache-payload-content attachment-cache-put!
    attachment-cache-sweep!
  ) ;export
  (begin

    (define (attachment-cache-base)
      ;; 平台惯例缓存基目录：Windows %LOCALAPPDATA%（未设置回退
      ;; ~/AppData/Local）、macOS ~/Library/Caches、Linux XDG_CACHE_HOME
      ;; （未设置回退 ~/.cache）
      (let ((home (path->string (path-home))))
        (cond
         ((os-windows?)
          (let ((local (getenv "LOCALAPPDATA")))
            (if local local (path-join home "AppData" "Local"))
          ) ;let
         ) ;
         ((os-macos?) (path-join home "Library" "Caches"))
         (else
           (let ((xdg (getenv "XDG_CACHE_HOME")))
             (if xdg xdg (path-join home ".cache"))
           ) ;let
         ) ;else
        ) ;cond
      ) ;let
    ) ;define

    (define (attachment-extract-root)
      (path-join (attachment-cache-base) "Liiistem" "attachment" "extract")
    ) ;define

    (define (attachment-yyyymm year month)
      ;; yyyymm 目录名的唯一编码器（补零两位月份）
      (string-append (number->string year)
        (if (< month 10)
          (string-append "0" (number->string month))
          (number->string month)
        ) ;if
      ) ;string-append
    ) ;define

    (define (attachment-keep-months)
      ;; 月度保留窗口：当月与上月（清理与查找共用）
      (let* ((d (current-date)) (y (date-year d)) (m (date-month d)))
        (list (attachment-yyyymm y m)
          (if (= m 1) (attachment-yyyymm (- y 1) 12) (attachment-yyyymm y (- m 1)))
        ) ;list
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
      ;; 月度清理：删除 extract/ 下保留窗口之外的所有 yyyymm 目录；
      ;; 目录不存在跳过，多删不报错（ingest 入口触发）
      (let ((root (attachment-extract-root)) (keep (attachment-keep-months)))
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
      (path-join (attachment-extract-root) (car (attachment-keep-months)) sha256)
    ) ;define

    (define (attachment-extract-cache-find sha256)
      ;; 「当前月 → 上个月」两候选目录查找，命中返回 result.json 文本
      (let loop
        ((months (attachment-keep-months)))
        (if (null? months)
          #f
          (let ((f (path-join (attachment-extract-root) (car months) sha256 "result.json")))
            (if (path-file? f) (path-read-text (path->string f)) (loop (cdr months)))
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (attachment-cache-payload-content sha256)
      ;; 读取方入口：result.json 二元数组取 [1].content，缺失或损坏返回 #f
      (let ((cached (attachment-extract-cache-find sha256)))
        (if (not cached)
          #f
          (let ((j (catch #t (lambda () (string->json cached)) (lambda args #f))))
            (if (and j (vector? j) (>= (vector-length j) 2))
              (json-ref-string (vector-ref j 1) "content" #f)
              #f
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (attachment-fresh-payload sha256 filename ts content)
      (json->string (vector (list (cons "id" sha256) (cons "ts" ts))
                      (list (cons "content" content) (cons "filename" filename))
                    ) ;vector
      ) ;json->string
    ) ;define

    (define (attachment-cache-put! sha256 filename content)
      ;; 命中刷新：命中时保留缓存原文、仅刷新 ts 重写至当月目录（命中文件
      ;; 可能在上月目录），热附件不被月度清理误删；未命中写入新缓存
      ;; （content 仅此时使用）
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
                 ;; 缓存原文 [1] 原样保留，仅重写头部 ts
                 (json->string (vector (json-set (vector-ref j 0) "ts" ts) (vector-ref j 1)))
                 (attachment-fresh-payload sha256 filename ts content)
               ) ;if
             ) ;payload
            ) ;
        (path-mkdir dir #t #t)
        (path-write-text (path->string (path-join dir "result.json")) payload)
      ) ;let*
    ) ;define

  ) ;begin
) ;define-library
