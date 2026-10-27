
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

;; 假附件插件不联网（与 llm 假插件同款自包含写法：只依赖 goldfish 内置
;; 库，不向内置 goldfish 树添加共享库）：收到 "%attachment {json}" ingest
;; 请求后先按月清理缓存（同真实插件约定），再处理任务。
;; 预检策略归属宿主（TeXmacs/plugins/attachment/progs/attachment-policy.scm
;; 由 init-attachment.scm 加载）；本文件不再定义数量/总大小/扩展名常量
;; 与 fake-attachment-precheck，禁入不可读路径仅靠宿主拦截。请求瘦身
;; 为 {taskId, path}；宿主做过格式转换（文档转临时 PDF）时另带可选的
;; sourcePath（原文件路径）。违反宿主预检的请求根本不会到达本插件。
;; 假 sha256 由路径字符串确定性散列生成：带 sourcePath 时对原路径取
;; 散列（缓存键随原文档，不随每次转换随机生成的临时 PDF 路径），否则
;; 对读取路径取散列；假提取文本写入抽取缓存，供 llm 插件按 sha256 取用
;; ——llm 插件只认 sha256、不接触本地路径。缓存载荷 [1] 标注
;; supplier=fake（标明数据来自假插件），ready 以回读当月缓存确认为前提，
;; 确认失败返回带原 taskId 的 error 事件

;;; ---------- 抽取缓存：平台基目录、月度布局、命中刷新、月度清理 ----------
;; 磁盘契约（真实插件遵从同一约定，tm-llm.scm 的读取方内联同一套约定）：
;; <缓存基目录>/Liiistem/attachment/extract/<yyyymm>/<sha256>/result.json，
;; 二元数组 [{"id": sha256, "ts"}, {"content": …, "filename": …,
;; "supplier": …}]（[1].supplier 标注数据来源插件；命中刷新沿用原缓存的
;; supplier），保留当月与上月

(define (attachment-cache-base)
  ;; 平台惯例缓存基目录（对齐 mogan C++ get_tm_cache_path 的分布）：
  ;; Windows %LOCALAPPDATA%（未设置回退 ~/AppData/Local）、macOS
  ;; ~/Library/Caches、Linux XDG_CACHE_HOME（未设置回退 ~/.cache）
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

(define (attachment-fresh-entry filename content)
  ;; 新缓存 [1]（expected-payload 的唯一构造点）：假提取文本 + 文件名 +
  ;; supplier=fake（真实链路的 supplier 由 liii 真实插件写入）
  (list (cons "content" content)
    (cons "filename" filename)
    (cons "supplier" "fake")
  ) ;list
) ;define

(define (attachment-cache-put! sha256 filename content)
  ;; 命中刷新：命中时保留缓存原文 [1]（含其 supplier，不用假提取文本
  ;; 重造 content）、仅刷新 ts 重写至当月目录（命中文件可在上月目录），
  ;; 热附件不被月度清理误删；未命中写入新缓存（content 仅此时使用）。
  ;; 返回本次应保存的 [1] 供调用方独立回读比对——它只是预期值，
  ;; 不能代替落盘确认
  (let* ((dir (attachment-extract-cache-dir sha256))
         (cached (attachment-extract-cache-find sha256))
         (ts
           (number->string (inexact->exact (floor (current-second))))
         ) ;ts
         (j
           (and cached (catch #t (lambda () (string->json cached)) (lambda args #f)))
         ) ;j
         (hit (and j (vector? j) (= (vector-length j) 2)))
         (expected-payload (if hit (vector-ref j 1) (attachment-fresh-entry filename content))
         ) ;expected-payload
        ) ;
    (path-mkdir dir #t #t)
    (path-write-text (path->string (path-join dir "result.json"))
      (json->string
        (vector (if hit
                  ;; 命中时沿用缓存原文头部，仅重写 ts
                  (json-set (vector-ref j 0) "ts" ts)
                  (list (cons "id" sha256) (cons "ts" ts))
                ) ;if
          expected-payload
        ) ;vector
      ) ;json->string
    ) ;path-write-text
    expected-payload
  ) ;let*
) ;define

(define (attachment-cache-payload-stored? sha256 expected-payload)
  ;; 当月落盘确认：只回读当月目标 result.json 并校验 id 与完整 [1]，
  ;; 任何读取/解析异常一律视为未落盘。不得用逐月查找代替——命中刷新
  ;; 写当月失败而上月旧文件还在时会误判成功。只读：不写、不删、不重试
  (catch #t
    (lambda ()
      (let ((f (path-join (attachment-extract-cache-dir sha256) "result.json")))
        (and (path-file? f)
          (let ((j (string->json (path-read-text (path->string f)))))
            (and (vector? j)
              (= (vector-length j) 2)
              (string=? (json-ref-string (vector-ref j 0) "id" "") sha256)
              ;; 比较解析后的 JSON 结构（非格式化文本），字段顺序与
              ;; 空白差异不误判
              (equal? (vector-ref j 1) expected-payload)
            ) ;and
          ) ;let
        ) ;and
      ) ;let
    ) ;lambda
    (lambda args #f)
  ) ;catch
) ;define

(define (fake-attachment-sha256 file-path)
  (sha256 file-path)
) ;define

(define (fake-attachment-sha256-key file-path source-path)
  ;; sha256/抽取缓存键以原文件为准：宿主转换过格式时 source-path 非空，
  ;; 键随原文档而非临时产物；常规附件 source-path 为空串，退回读取路径
  (if (string-null? source-path) file-path source-path)
) ;define

(define (fake-attachment-extracted-text filename)
  (string-append "[fake] extracted text of " filename)
) ;define

(define (fake-attachment-ready-event task-id file-path source-path)
  ;; 请求已瘦身：filename 与 size 由 file-path 在此计算（ready 事件六键形态
  ;; 不变：filename/size 冗余但保留以稳定协议契约）；缓存回读确认失败时
  ;; 不返回 ready，改返回带原 taskId 的 error（函数名保留历史叫法）；
  ;; 参数名避开 path：(liii path) 的 path 构造函数在 s7 下被字符串参数
  ;; 遮蔽后 (path x) 会被当作 string-ref 调用
  (let* ((p (path file-path))
         (filename (path->string (path-name p)))
         (size (if (and (path-file? p) (access file-path 'R_OK)) (path-getsize p) 0))
         (sha256 (fake-attachment-sha256 (fake-attachment-sha256-key file-path source-path))
         ) ;sha256
         (content (fake-attachment-extracted-text filename))
         ;; put 成功返回不等于落盘成功：expected-payload 只是预期值，
         ;; 必须独立回读当月 result.json 确认（不能用缓存目录存在性或
         ;; 逐月 content reader 代替）
         (expected-payload (attachment-cache-put! sha256 filename content))
        ) ;
    (if (attachment-cache-payload-stored? sha256 expected-payload)
      (json->string (list (cons "event" "ready")
                      (cons "taskId" task-id)
                      (cons "sha256" sha256)
                      (cons "filename" filename)
                      (cons "size" size)
                      (cons "extractedChars" (string-length content))
                    ) ;list
      ) ;json->string
      (fake-attachment-error-event task-id "附件缓存写入失败，请重试")
    ) ;if
  ) ;let*
) ;define

(define (fake-attachment-error-event task-id reason)
  (json->string (list (cons "event" "error") (cons "taskId" task-id) (cons "reason" reason))
  ) ;json->string
) ;define

(define (fake-attachment-ingest data)
  ;; 先解析请求并取得 task-id，再在 catch #t 保护下执行缓存月度清理与
  ;; ready 缓存写入。预检已上移宿主（attachment-policy.scm），请求只含
  ;; {taskId, path} 与可选的 sourcePath。每个能解析出 taskId 的合法请求
  ;; 无论发生何种异常都必须返回带原 taskId 的 error 事件，不向 REPL 外抛出
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
      (let ((task-id (json-ref-string j "taskId" "")))
        (catch #t
          (lambda ()
            (attachment-cache-sweep!)
            (let ((file-path (json-ref-string j "path" ""))
                  (source-path (json-ref-string j "sourcePath" ""))
                 ) ;
              (fake-attachment-ready-event task-id file-path source-path)
            ) ;let
          ) ;lambda
          (lambda (tag . rest)
            (let ((detail
                    (catch #t
                      (lambda ()
                        (cond ((null? rest) (object->string tag))
                              ;; TeXmacs/Guile catch 风格：(key subr message args ...)
                              ((and (>= (length rest) 2) (string? (cadr rest)))
                               (let ((msg (cadr rest))
                                     (args
                                       (if (and (>= (length rest) 3) (pair? (caddr rest))) (caddr rest) '())
                                     ) ;args
                                    ) ;
                                 (if (null? args) msg (apply format #f msg args))
                               ) ;let
                              ) ;
                              ;; 原生 S7 风格：(tag ((format-string . format-args)))
                              ((and (pair? (car rest)) (string? (caar rest))) (apply format #f (car rest)))
                              ((string? (car rest)) (car rest))
                              (else (object->string (car rest)))
                        ) ;cond
                      ) ;lambda
                      (lambda _ (object->string tag))
                    ) ;catch
                  ) ;detail
                 ) ;
              (fake-attachment-error-event task-id
                (string-append "附件处理异常： " detail)
              ) ;fake-attachment-error-event
            ) ;let
          ) ;lambda
        ) ;catch
      ) ;let
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
