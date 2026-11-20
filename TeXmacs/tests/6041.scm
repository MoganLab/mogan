;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6041.scm
;; DESCRIPTION : 「文档 -> 元数据...」QML 对话框数据契约集成测试
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/generic/document-widgets.scm")

(check-set-mode! 'report-failed)

(define (test-metadata-dialog-hook)
  (let ((form (stree->tree '(metadata-form (title "Document metadata")
                              (field "Title" "global-title" "CurTitle"
                                "DefTitle")
                              (field "Author" "global-author" "CurAuthor"
                                "DefAuthor")
                              (field "Subject" "global-subject" "CurSubject" ""))
              ) ;stree->tree
        ) ;form
       ) ;
    (system-setenv "MOGAN_TEST_DOCUMENT_METADATA" "cancel")
    (check (tree->stree (cpp-document-metadata-dialog form)) => '(tuple))

    (system-setenv "MOGAN_TEST_DOCUMENT_METADATA" "ok")
    (check (tree->stree (cpp-document-metadata-dialog form))
      =>
      '(tuple (tuple "global-title" "CurTitle")
         (tuple "global-author" "CurAuthor")
         (tuple "global-subject" "CurSubject")
         (tuple "reset" "false"))
    ) ;check

    (system-setenv "MOGAN_TEST_DOCUMENT_METADATA" "reset")
    (check (tree->stree (cpp-document-metadata-dialog form))
      =>
      '(tuple (tuple "global-title" "DefTitle")
         (tuple "global-author" "DefAuthor")
         (tuple "global-subject" "")
         (tuple "reset" "true"))
    ) ;check
  ) ;let
  (system-setenv "MOGAN_TEST_DOCUMENT_METADATA" "")
) ;define

(define (test-metadata-buffer-contract)
  (new-buffer)
  (let ((u (current-buffer)))
    (initial-set u "global-title" (utf8->cork "测试标题"))
    (initial-set u "global-author" "Alice")
    (initial-set u "global-subject" "Math")

    (check (buffer-get-metadata u "title") => "测试标题")
    (check (buffer-get-metadata u "author") => "Alice")
    (check (buffer-get-metadata u "subject") => "Math")

    ;; Cancel 不修改任何元数据
    (system-setenv "MOGAN_TEST_DOCUMENT_METADATA" "cancel")
    (open-document-metadata-window)
    (check (buffer-get-metadata u "title") => "测试标题")
    (check (buffer-get-metadata u "author") => "Alice")
    (check (buffer-get-metadata u "subject") => "Math")

    ;; 提交更新（reset = "false"）写回 global-*
    (document-metadata-apply-result u
      (stree->tree
        `(tuple (tuple "global-title" ,(utf8->cork "新标题"))
           (tuple "global-author" "Bob")
           (tuple "global-subject" "Physics")
           (tuple "reset" "false"))
      ) ;stree->tree
    ) ;document-metadata-apply-result
    (check (buffer-get-metadata u "title") => "新标题")
    (check (buffer-get-metadata u "author") => "Bob")
    (check (buffer-get-metadata u "subject") => "Physics")

    ;; Reset 提交清除 global-title / global-author / global-subject
    (system-setenv "MOGAN_TEST_DOCUMENT_METADATA" "reset")
    (open-document-metadata-window)
    (check (initial-has? u "global-title") => #f)
    (check (initial-has? u "global-author") => #f)
    (check (initial-has? u "global-subject") => #f)
  ) ;let
  (system-setenv "MOGAN_TEST_DOCUMENT_METADATA" "")
) ;define

(tm-define (test_6041)
  (test-metadata-dialog-hook)
  (test-metadata-buffer-contract)
  (check-report)
) ;tm-define
