;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 4482.scm
;; DESCRIPTION : 文件 → 导出为 PDF QML 对话框数据契约
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r 4482
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (kv-ref kvs key)
  (cond ((null? kvs) #f)
        ((== (cadr (car kvs)) key) (caddr (car kvs)))
        (else (kv-ref (cdr kvs) key))
  ) ;cond
) ;define

(define (sample-form)
  (stree->tree '(export-pdf-form (path "File name:" "filename" "a.pdf")
                  (path "Location:" "folder" "C:/tmp")
                  (number "First page:" "first" "1")
                  (number "Last page:" "last" "2")
                  (toggle "Embed TMU source" "embed" "false"))
  ) ;stree->tree
) ;define

(tm-define (test_4482)
  (system-setenv "MOGAN_TEST_EXPORT_PDF" "cancel")
  (check (cdr (tree->stree (cpp-export-pdf-dialog (sample-form)))) => '())
  (system-setenv "MOGAN_TEST_EXPORT_PDF" "ok")
  (let ((kvs (cdr (tree->stree (cpp-export-pdf-dialog (sample-form))))))
    (check (kv-ref kvs "filename") => "a.pdf")
    (check (kv-ref kvs "folder") => "C:/tmp")
    (check (kv-ref kvs "first") => "1")
    (check (kv-ref kvs "last") => "2")
    (check (kv-ref kvs "embed") => "false")
  ) ;let
  (system-setenv "MOGAN_TEST_EXPORT_PDF" "")
  (check-report)
) ;tm-define
