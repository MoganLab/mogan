;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : shortcut-edit-test.scm
;; DESCRIPTION : 纯逻辑单元测试：快捷键数据的构建、校验、增删查改及 JSON 序列化
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r shortcut-edit-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(import (liii json))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/source/shortcut-edit.scm")

;; 1. 条目构造与访问器

(define (test-make-shortcut-entry)
  (let ((entry (make-shortcut-entry "C-a" "(noop)")))
    (check (shortcut-entry-shortcut entry) => "C-a")
    (check (shortcut-entry-command entry) => "(noop)")
    (check (shortcut-entry-valid? entry) => #t)
  ) ;let
) ;define

;; 2. 非法条目校验

(define (test-shortcut-entry-valid-failures)
  (check (shortcut-entry-valid? '()) => #f)
  (check (shortcut-entry-valid? "not-an-entry") => #f)
  (check (shortcut-entry-valid? '(("shortcut" . "C-a"))) => #f)
  (check (shortcut-entry-valid? '(("command" . "(noop)"))) => #f)
  (check (shortcut-entry-valid? '(("shortcut" . 123) ("command" . "(noop)")))
    =>
    #f
  ) ;check
  (check (shortcut-entry-valid? '(("shortcut" . "C-a") ("command" . 456))) => #f)
) ;define

;; 3. 空 JSON 构造与序列化

(define (test-empty-user-shortcuts-json)
  (let ((empty-json (make-empty-user-shortcuts-json)))
    (check (user-shortcuts-json-valid? empty-json) => #t)
    (check (json->string empty-json)
      =>
      "{\"meta\":{\"version\":1,\"total\":0},\"shortcuts\":[]}"
    ) ;check
  ) ;let
) ;define

;; 4. 非空条目构造与序列化

(define (test-make-user-shortcuts-json)
  (let* ((entries (list (make-shortcut-entry "C-x C-f" "(find-file)")
                    (make-shortcut-entry "C-x C-s" "(save-buffer)")
                  ) ;list
         ) ;entries
         (data (make-user-shortcuts-json entries))
         (json-str (json->string data))
         (parsed (string->json json-str))
        ) ;
    (check (user-shortcuts-json-valid? data) => #t)
    (check (user-shortcuts-json-valid? parsed) => #t)
    (check json-str
      =>
      "{\"meta\":{\"version\":1,\"total\":2},\"shortcuts\":[{\"shortcut\":\"C-x C-f\",\"command\":\"(find-file)\"},{\"shortcut\":\"C-x C-s\",\"command\":\"(save-buffer)\"}]}"
    ) ;check
  ) ;let*
) ;define

;; 5. user-shortcuts-json-valid? 边界与非法结构检测

(define (test-user-shortcuts-json-valid-failures)
  ;; 非 json-object
  (check (user-shortcuts-json-valid? "string") => #f)
  (check (user-shortcuts-json-valid? 123) => #f)
  (check (user-shortcuts-json-valid? #()) => #f)
  (check (user-shortcuts-json-valid? '()) => #f)
  ;; 缺少 meta
  (check (user-shortcuts-json-valid? (string->json "{\"shortcuts\":[]}")) => #f)
  ;; 缺少 shortcuts
  (check (user-shortcuts-json-valid? (string->json "{\"meta\":{\"version\":1,\"total\":0}}")
         ) ;user-shortcuts-json-valid?
    =>
    #f
  ) ;check
  ;; meta version 非整数
  (check (user-shortcuts-json-valid? (string->json "{\"meta\":{\"version\":\"1\",\"total\":0},\"shortcuts\":[]}")
         ) ;user-shortcuts-json-valid?
    =>
    #f
  ) ;check
  ;; meta total 负数
  (check (user-shortcuts-json-valid? (string->json "{\"meta\":{\"version\":1,\"total\":-1},\"shortcuts\":[]}")
         ) ;user-shortcuts-json-valid?
    =>
    #f
  ) ;check
  ;; meta total 与 shortcuts 长度不一致
  (check (user-shortcuts-json-valid? (string->json "{\"meta\":{\"version\":1,\"total\":1},\"shortcuts\":[]}")
         ) ;user-shortcuts-json-valid?
    =>
    #f
  ) ;check
  ;; shortcuts 非数组
  (check (user-shortcuts-json-valid? (string->json "{\"meta\":{\"version\":1,\"total\":0},\"shortcuts\":{}}")
         ) ;user-shortcuts-json-valid?
    =>
    #f
  ) ;check
  ;; shortcuts 中含非法条目
  (check (user-shortcuts-json-valid? (string->json "{\"meta\":{\"version\":1,\"total\":1},\"shortcuts\":[{\"shortcut\":\"C-a\"}]}"
                                     ) ;string->json
         ) ;user-shortcuts-json-valid?
    =>
    #f
  ) ;check
  (check (user-shortcuts-json-valid? (string->json "{\"meta\":{\"version\":1,\"total\":1},\"shortcuts\":[\"bad\"]}")
         ) ;user-shortcuts-json-valid?
    =>
    #f
  ) ;check
) ;define

;; 6. 内存状态与 CRUD

(define (test-shortcuts-crud)
  (let ((saved current-user-shortcuts))
    (set-current-user-shortcuts-list (list (make-shortcut-entry "C-x C-f" "(find-file)")
                                       (make-shortcut-entry "C-x C-s" "(save-buffer)")
                                     ) ;list
    ) ;set-current-user-shortcuts-list
    (check (length (current-user-shortcuts-list)) => 2)
    (check (find-user-shortcut-entry "C-x C-f")
      =>
      (make-shortcut-entry "C-x C-f" "(find-file)")
    ) ;check
    (check (find-user-shortcut-entry "non-existent") => #f)
    ;; 恢复原状态
    (replace-current-user-shortcuts! saved)
  ) ;let
) ;define

;; 7. 快捷键查询与列表排序

(define (test-shortcuts-query-and-sort)
  (let ((saved current-user-shortcuts))
    (set-current-user-shortcuts-list (list (make-shortcut-entry "C-b" "(prev)") (make-shortcut-entry "C-a" "(next)"))
    ) ;set-current-user-shortcuts-list
    (check (has-user-shortcut? "(prev)") => #t)
    (check (has-user-shortcut? "(non-existent)") => #f)
    (check (get-user-shortcut "C-b") => "(prev)")
    (check (get-user-shortcut "C-c") => #f)
    (check (user-shortcuts-list) => '("C-a" "C-b"))
    ;; 恢复原状态
    (replace-current-user-shortcuts! saved)
  ) ;let
) ;define

;; 8. 旧版 scm 条目迁移与合并

(define (test-legacy-scm-migration)
  (check (legacy-scm-user-shortcut-entry->json '("C-f" "(find)"))
    =>
    (make-shortcut-entry "C-f" "(find)")
  ) ;check
  (check (legacy-scm-user-shortcut-entry->json '("bad")) => #f)
  (check (legacy-scm-user-shortcut-entry->json 123) => #f)

  (let* ((existing (list (make-shortcut-entry "C-a" "(cmd-a)")))
         (legacy-to-merge (list (make-shortcut-entry "C-a" "(cmd-a-old)")
                            (make-shortcut-entry "C-b" "(cmd-b)")
                          ) ;list
         ) ;legacy-to-merge
         (merged (merge-legacy-scm-user-shortcuts existing legacy-to-merge))
        ) ;
    (check (length merged) => 2)
    (check (assoc-ref (car merged) "shortcut") => "C-a")
    (check (assoc-ref (car merged) "command") => "(cmd-a)")
    (check (assoc-ref (cadr merged) "shortcut") => "C-b")
    (check (assoc-ref (cadr merged) "command") => "(cmd-b)")
  ) ;let*
) ;define

;; 9. 快捷键字符串规范化

(define (test-normalize-shortcut-string)
  (check (normalize-shortcut-string "<less>   <gtr>") => "< >")
  (check (normalize-shortcut-string "C-x   C-s") => "C-x C-s")
  (check (normalize-shortcut-string 123) => 123)
) ;define

;; 10. 文件持久化读写与异常恢复

(define (test-save-and-load-user-shortcuts)
  (let* ((orig-file user-shortcuts-file)
         (orig-shortcuts current-user-shortcuts)
         (tmp-path (url->string (url-append (url-temp) "shortcuts.json")))
        ) ;
    (set! user-shortcuts-file tmp-path)
    (set-current-user-shortcuts-list (list (make-shortcut-entry "C-1" "(insert-1)")
                                       (make-shortcut-entry "C-2" "(insert-2)")
                                     ) ;list
    ) ;set-current-user-shortcuts-list
    (save-user-shortcuts)
    (check (url-exists? tmp-path) => #t)

    ;; 验证保存出的 JSON 文件内容可由 (liii json) 正常解析
    (let ((file-content (string->json (string-load tmp-path))))
      (check (user-shortcuts-json-valid? file-content) => #t)
    ) ;let

    ;; 清空内存，重新从文件 load
    (replace-current-user-shortcuts! (make-empty-user-shortcuts-json))
    (check (length (current-user-shortcuts-list)) => 0)
    (load-user-shortcuts)
    (check (length (current-user-shortcuts-list)) => 2)
    (check (get-user-shortcut "C-1") => "(insert-1)")
    (check (get-user-shortcut "C-2") => "(insert-2)")

    ;; 写入损坏的 JSON，load-user-shortcuts 应重置为有效空状态
    (string-save "corrupted-json-data{{{" tmp-path)
    (load-user-shortcuts)
    (check (length (current-user-shortcuts-list)) => 0)
    (check (user-shortcuts-json-valid? current-user-shortcuts) => #t)

    ;; 清理临时文件并恢复现场
    (catch #t (lambda () (url-remove tmp-path)) (lambda args #f))
    (set! user-shortcuts-file orig-file)
    (replace-current-user-shortcuts! orig-shortcuts)
  ) ;let*
) ;define

;; 11. decode-shortcut 映射

(define (test-decode-shortcut)
  (let ((saved current-user-shortcuts))
    (set-current-user-shortcuts-list (list (make-shortcut-entry "C-a" "(insert-a)"))
    ) ;set-current-user-shortcuts-list
    (check (decode-shortcut (encode-shortcut "C-a")) => "C-a")
    (replace-current-user-shortcuts! saved)
  ) ;let
) ;define

(tm-define (regtest-shortcut-edit)
  (test-make-shortcut-entry)
  (test-shortcut-entry-valid-failures)
  (test-empty-user-shortcuts-json)
  (test-make-user-shortcuts-json)
  (test-user-shortcuts-json-valid-failures)
  (test-shortcuts-crud)
  (test-shortcuts-query-and-sort)
  (test-legacy-scm-migration)
  (test-normalize-shortcut-string)
  (test-save-and-load-user-shortcuts)
  (test-decode-shortcut)
  (check-report)
) ;tm-define
