;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : stem-format-test.scm
;; DESCRIPTION : 纯逻辑单元测试：STEM 格式文档保存时自动使用 (liii goldfmt stem) 格式化（Issue 6011）
;;               - texmacs->stem 导出结果符合 goldfmt 格式规范（含缩进、闭合括号注释等）
;;               - tree-export 导出到 .stem 文件时自动完成格式化
;;               - 格式化后的 .stem 文件经 tree-import 导入能完整无损还原 tree 结构（roundtrip 一致）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r stem-format-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (stem stem-format) (tmu tmu-format))

(import (liii check) (liii path) (liii goldfmt stem))

(check-set-mode! 'report-failed)

;; 1. 测试 texmacs-tree -> stem-document 转换时自动调用 goldfmt stem 格式化

(define (test-stem-conversion-formatted)
  (let* ((orig-str "(document (TeXmacs \"2.1.4\") (style (tuple \"generic\")) (body (document (chapter* \"Chapter 1\") (section* \"Section 1\") (itemize (document (concat (item) \"First item\") (concat (item) \"Second item\"))))))"
         ) ;orig-str
         (t (stm->texmacs orig-str))
         (stem-doc (convert t "texmacs-tree" "stem-document"))
         (expected-doc (format-stem-string orig-str))
        ) ;
    (check (string? stem-doc) => #t)
    ;; 验证转换结果与直接经 format-stem-string 格式化的内容一致
    (check stem-doc => expected-doc)
    ;; 验证包含 goldfmt 特有的换行缩进与闭合标签注释
    (check (string-contains? stem-doc "  (style (tuple \"generic\"))\n") => #t)
    (check (string-contains? stem-doc ") ;document\n") => #t)
  ) ;let*
) ;define

;; 2. 测试 tree-export 导出到 .stem 文件并经 tree-import 往返一致性

(define (test-stem-file-export-and-roundtrip)
  (let* ((orig-str "(document (TeXmacs \"2.1.4\") (style (tuple \"generic\")) (body (document (chapter* \"Intro\") (para \"Hello STEM World\"))))"
         ) ;orig-str
         (t (stm->texmacs orig-str))
         (tmp-path "/tmp/test_export_6011.stem")
         (tmp-url (system->url tmp-path))
        ) ;
    ;; 导出到 .stem 文件
    (tree-export t tmp-path "stem")
    (check (url-exists? tmp-url) => #t)

    ;; 读取导出的文件内容，断言为格式化文本
    (let ((content (string-load tmp-url)))
      (check content => (format-stem-string orig-str))
      (check (string-contains? content ") ;document\n") => #t)
    ) ;let

    ;; 导入该 .stem 文件，验证 tree 结构完全一致
    (let ((imported-tree (tree-import tmp-path "stem")))
      (check (equal? imported-tree t) => #t)
    ) ;let

    ;; 清理临时文件
    (system-remove tmp-url)
  ) ;let*
) ;define

(tm-define (regtest-stem-format)
  (test-stem-conversion-formatted)
  (test-stem-file-export-and-roundtrip)
  (check-report)
) ;tm-define
