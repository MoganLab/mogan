;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 4160.scm
;; DESCRIPTION : 选择打印为文件 QML 对话框的数据契约
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r 4160
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
  (stree->tree '(print-to-file-form (path "File name:" "name" "a.ps")
                  (number "First page:" "first" "1")
                  (number "Last page:" "last" "2"))
  ) ;stree->tree
) ;define

(tm-define (test_4160)
  (system-setenv "MOGAN_TEST_PRINT_TO_FILE" "cancel")
  (check (cdr (tree->stree (cpp-print-to-file-dialog (sample-form)))) => '())
  (system-setenv "MOGAN_TEST_PRINT_TO_FILE" "ok")
  (let ((kvs (cdr (tree->stree (cpp-print-to-file-dialog (sample-form))))))
    (check (kv-ref kvs "name") => "a.ps")
    (check (kv-ref kvs "first") => "1")
    (check (kv-ref kvs "last") => "2")
  ) ;let
  (system-setenv "MOGAN_TEST_PRINT_TO_FILE" "")
  (check-report)
) ;tm-define
