
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-dialogue-test.scm
;; DESCRIPTION : recent-files 路径归一的纯逻辑单元测试（不弹 GUI，headless 可跑）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel texmacs tm-dialogue-test)
  (:use (kernel texmacs tm-dialogue))
) ;texmacs-module

(import (liii check) (liii json))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for recent-files-canonical-path
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Windows 下 scheme 侧 url->system 记录 '\' 分隔，C++ 启动页
;; QDir::fromNativeSeparators 传入 '/' 分隔；归一必须消除该差异，
;; 否则 recent-files-remove-by-path 按路径移除失效（任务 0948 回归）。

(define (test-canonical-path-win-separators)
  (when (os-windows?)
    (check (recent-files-canonical-path "C:/Users/a b/x.tmu")
      =>
      "C:\\Users\\a b\\x.tmu"
    ) ;check
    (check (recent-files-canonical-path "C:\\Users\\a b\\x.tmu")
      =>
      "C:\\Users\\a b\\x.tmu"
    ) ;check
    ;; 含中文与空格的真实路径
    (check (recent-files-canonical-path "C:/Users/测试 文件/x.tmu")
      =>
      "C:\\Users\\测试 文件\\x.tmu"
    ) ;check
  ) ;when
) ;define

;; tmfs://（云端文档等虚拟路径）无盘符归一，往返后须原样保持。

(define (test-canonical-path-tmfs)
  (check (recent-files-canonical-path "tmfs://collab/8fc7bec4-f069-458f-8578-8fabcd4696a2"
         ) ;recent-files-canonical-path
    =>
    "tmfs://collab/8fc7bec4-f069-458f-8578-8fabcd4696a2"
  ) ;check
  (check (recent-files-canonical-path "tmfs://aux/test") => "tmfs://aux/test")
) ;define

;; 非 Windows 下 '/' 即系统分隔符，绝对路径归一幂等。

(define (test-canonical-path-unix-idempotent)
  (when (not (os-windows?))
    (check (recent-files-canonical-path "/home/u/a b.tm") => "/home/u/a b.tm")
  ) ;when
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for interactive commands learned state
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-interactive-commands)
  (forget-interactive 'test-regtest-cmd)
  (check (learned-interactive 'test-regtest-cmd) => '())

  ;; 学习第一组参数
  (learn-interactive 'test-regtest-cmd '(("0" . "arg0") ("1" . "arg1")))
  (check (learned-interactive 'test-regtest-cmd)
    =>
    '((("0" . "arg0") ("1" . "arg1")))
  ) ;check

  ;; 学习第二组参数，后学习的条目在前
  (learn-interactive 'test-regtest-cmd '(("0" . "argA") ("1" . "argB")))
  (check (learned-interactive 'test-regtest-cmd)
    =>
    '((("0" . "argA") ("1" . "argB")) (("0" . "arg0") ("1" . "arg1")))
  ) ;check

  ;; 重复学习第一组参数，应被提至头部且不重复
  (learn-interactive 'test-regtest-cmd '(("0" . "arg0") ("1" . "arg1")))
  (check (learned-interactive 'test-regtest-cmd)
    =>
    '((("0" . "arg0") ("1" . "arg1")) (("0" . "argA") ("1" . "argB")))
  ) ;check

  ;; 清除学习的参数
  (forget-interactive 'test-regtest-cmd)
  (check (learned-interactive 'test-regtest-cmd) => '())
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for recent-files state
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-recent-files)
  ;; 清空最近文件
  (forget-interactive 'recent-buffer)
  (check (learned-interactive 'recent-buffer) => '())

  ;; 增加第 1 个最近文件
  (recent-files-learn "/tmp/test1.tm" "test1.tm")
  (check (recent-files-get-name "/tmp/test1.tm") => "test1.tm")
  (check (learned-interactive 'recent-buffer) => '((("0" . "/tmp/test1.tm"))))

  ;; 增加第 2 个最近文件
  (recent-files-learn "/tmp/test2.tm" "test2.tm")
  (check (recent-files-get-name "/tmp/test2.tm") => "test2.tm")
  (check (length (learned-interactive 'recent-buffer)) => 2)

  ;; 再次 learn 第 1 个文件（更新 last_open）
  (recent-files-learn "/tmp/test1.tm" "test1.tm")
  ;; 最新访问的排在最前
  (check (caar (learned-interactive 'recent-buffer)) => '("0" . "/tmp/test1.tm"))

  ;; 按路径移除第 1 个文件
  (recent-files-remove-by-path "/tmp/test1.tm")
  (check (recent-files-get-name "/tmp/test1.tm") => #f)
  (check (length (learned-interactive 'recent-buffer)) => 1)
  (check (caar (learned-interactive 'recent-buffer)) => '("0" . "/tmp/test2.tm"))

  ;; 移除不存在的文件应无副作用
  (recent-files-remove-by-path "/tmp/non-existent.tm")
  (check (length (learned-interactive 'recent-buffer)) => 1)

  ;; 移除第 2 个文件
  (recent-files-remove-by-path "/tmp/test2.tm")
  (check (recent-files-get-name "/tmp/test2.tm") => #f)
  (check (learned-interactive 'recent-buffer) => '())

  ;; 通过 learn-interactive 'recent-buffer 间接增加
  (learn-interactive 'recent-buffer '((0 . "/tmp/test3.tm")))
  (check (recent-files-get-name "/tmp/test3.tm") => "test3.tm")
  (check (learned-interactive 'recent-buffer) => '((("0" . "/tmp/test3.tm"))))

  ;; 最终清空
  (forget-interactive 'recent-buffer)
  (check (learned-interactive 'recent-buffer) => '())
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for schema validation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-validation)
  ;; 空状态有效性
  (check (interactive-args-json-valid? (make-empty-state 'interactive-arg)) => #t)
  (check (recent-files-json-valid? (make-empty-state 'recent-file)) => #t)

  ;; 非法顶层结构
  (check (interactive-args-json-valid? "not-an-object") => #f)
  (check (interactive-args-json-valid? 123) => #f)
  (check (recent-files-json-valid? "not-an-object") => #f)
  (check (recent-files-json-valid? #f) => #f)

  ;; interactive-args 版本号需为 >= 1 的整数
  (check (interactive-args-json-valid? `((,"meta" (,"version" . ,0))
                                         ("commands" ())))
    =>
    #f
  ) ;check
  (check (interactive-args-json-valid? `((,"meta" (,"version" . ,"1"))
                                         ("commands" ())))
    =>
    #f
  ) ;check

  ;; interactive-args 包含合法命令条目
  (check (interactive-args-json-valid? `((,"meta" (,"version" . ,1))
                                         (,"commands"
                                          (,"my-cmd"
                                           . ,#((("key1" . "val1")
                                                 ("key2" . "val2"))
                                              ) ;#
                                          )))
         ) ;interactive-args-json-valid?
    =>
    #t
  ) ;check

  ;; interactive-args 命令条目不是字符串键值映射时非法
  (check (interactive-args-json-valid? `((,"meta" (,"version" . ,1))
                                         (,"commands"
                                          (,"my-cmd" . ,#((("key1" . 123))))))
         ) ;interactive-args-json-valid?
    =>
    #f
  ) ;check

  ;; recent-files 包含合法文件记录
  (check (recent-files-json-valid? `((,"meta" (,"version" . ,1) (,"total" . ,1))
                                     (,"files"
                                      . ,#((("path" . "/tmp/doc.tm")
                                            ("name" . "doc.tm")
                                            ("last_open" . 1700000000)
                                            ("open_count" . 3)
                                            ("show" . #t))
                                         ) ;#
                                     ))
         ) ;recent-files-json-valid?
    =>
    #t
  ) ;check

  ;; recent-files total 不能为负数
  (check (recent-files-json-valid? `((,"meta"
                                      (,"version" . ,1)
                                      (,"total" . ,-1))
                                     (,"files" . ,#()))
         ) ;recent-files-json-valid?
    =>
    #f
  ) ;check

  ;; recent-files 缺少必填字段（如缺少 show 或 last_open）
  (check (recent-files-json-valid? `((,"meta" (,"version" . ,1) (,"total" . ,1))
                                     (,"files"
                                      . ,#((("path" . "/tmp/doc.tm")
                                            ("name" . "doc.tm"))
                                         ) ;#
                                     ))
         ) ;recent-files-json-valid?
    =>
    #f
  ) ;check
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for LRU capacity limit
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-recent-files-lru-limit)
  (forget-interactive 'recent-buffer)
  ;; 添加 30 个文件（超过 limit 25）
  (let loop
    ((i 1))
    (when (<= i 30)
      (let ((p (string-append "/tmp/doc_" (number->string i) ".tm"))
            (n (string-append "doc_" (number->string i) ".tm"))
           ) ;
        (recent-files-learn p n)
        (loop (+ i 1))
      ) ;let
    ) ;when
  ) ;let
  ;; 全部 30 个文件记录均存在
  (check (length (learned-interactive 'recent-buffer)) => 30)
  ;; 最晚添加的排在最前
  (check (caar (learned-interactive 'recent-buffer)) => '("0"
                                                          . "/tmp/doc_30.tm"))
  ;; 清空
  (forget-interactive 'recent-buffer)
  (check (learned-interactive 'recent-buffer) => '())
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for persistence and fallback
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-persistence)
  ;; 1. 不存在的文件，应返回 fallback
  (check (load-json-with-fallback "/tmp/non_existent_file_1281.json"
           (lambda (x) #t)
           (lambda () 'fallback-value)
         ) ;load-json-with-fallback
    =>
    'fallback-value
  ) ;check

  ;; 2. 格式错误的 JSON，应捕获异常并返回 fallback
  (string-save "{malformed json:" (string->url "/tmp/test_bad_1281.json"))
  (check (load-json-with-fallback "/tmp/test_bad_1281.json"
           (lambda (x) #t)
           (lambda () 'fallback-bad)
         ) ;load-json-with-fallback
    =>
    'fallback-bad
  ) ;check

  ;; 3. 合法 JSON 但校验不通过，应返回 fallback
  (string-save "{\"meta\":{}}" (string->url "/tmp/test_invalid_schema_1281.json"))
  (check (load-json-with-fallback "/tmp/test_invalid_schema_1281.json"
           interactive-args-json-valid?
           (lambda () 'fallback-schema)
         ) ;load-json-with-fallback
    =>
    'fallback-schema
  ) ;check

  ;; 4. 合法 JSON 且校验通过，应成功解析
  (string-save "{\"meta\":{\"version\":1},\"commands\":{}}"
    (string->url "/tmp/test_valid_1281.json")
  ) ;string-save
  (check (interactive-args-json-valid? (load-json-with-fallback "/tmp/test_valid_1281.json"
                                         interactive-args-json-valid?
                                         (lambda () #f)
                                       ) ;load-json-with-fallback
         ) ;interactive-args-json-valid?
    =>
    #t
  ) ;check

  ;; 清理临时文件
  (system-remove (string->url "/tmp/test_bad_1281.json"))
  (system-remove (string->url "/tmp/test_invalid_schema_1281.json"))
  (system-remove (string->url "/tmp/test_valid_1281.json"))

  ;; 5. 测试 recent-files-save 真实落盘与重新加载
  (recent-files-learn "/tmp/test_persist_1281.tm" "test_persist_1281.tm")
  (recent-files-save)
  (check (url-exists? interactive-arg-recent-file-path) => #t)
  (let ((saved-json (string->json (string-load (string->url interactive-arg-recent-file-path)))
        ) ;saved-json
       ) ;
    (check (recent-files-json-valid? saved-json) => #t)
    (check (recent-files-get-name "/tmp/test_persist_1281.tm")
      =>
      "test_persist_1281.tm"
    ) ;check
  ) ;let
  ;; 清理
  (recent-files-remove-by-path "/tmp/test_persist_1281.tm")
  (recent-files-save)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-tm-dialogue)
  (test-canonical-path-win-separators)
  (test-canonical-path-tmfs)
  (test-canonical-path-unix-idempotent)
  (test-interactive-commands)
  (test-recent-files)
  (test-validation)
  (test-recent-files-lru-limit)
  (test-persistence)
  (check-report)
) ;tm-define
