;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : extract-style-package-test.scm
;; DESCRIPTION : 单元测试：导出样式包生成 .stem 草稿文件及宏菜单结构验证
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r extract-style-package-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/source/source-menu.scm")
(load "./TeXmacs/progs/source/source-edit.scm")

(define (menu-contains-label? m label)
  (cond ((null? m) #f)
        ((pair? m)
         (or (menu-contains-label? (car m) label) (menu-contains-label? (cdr m) label))
        ) ;
        ((string? m) (string=? m label))
        (else #f)
  ) ;cond
) ;define

(define (test-source-macros-menu-items)
  (let ((menu (source-macros-menu)))
    ;; 验证已移除 "Extract style file"，仅保留 "Extract style package"
    (check (menu-contains-label? menu "Extract style file") => #f)
    (check (menu-contains-label? menu "Extract style package") => #t)
  ) ;let
) ;define

(define (test-extract-style-package-stem-buffer)
  (let ((orig-buf (current-buffer)))
    (extract-style-package)
    (let* ((new-buf (current-buffer)) (name (url->string (url-tail new-buf))))
      ;; 验证新创建的 buffer 是 .stem 后缀的草稿文件
      (check (string-starts? name "draft_") => #t)
      (check (string-ends? name ".stem") => #t)
      (check (url-scratch? new-buf) => #t)
      ;; 验证记录了目标保存路径
      (check (url? (style-package-target-url new-buf)) => #t)
    ) ;let*
    ;; 切回原 buffer 并关闭测试草稿 buffer
    (buffer-close (current-buffer))
    (switch-to-buffer orig-buf)
  ) ;let
) ;define

(define (test-style-package-compute-target)
  (for-each (lambda (case
                    ) ;case
              (check (url->system (style-package-compute-target (system->url (car case))))
                =>
                (cdr case)
              ) ;check
            ) ;lambda
    '(("/home/da/docs/paper.tmu" . "/home/da/docs/paper.stem")
      ("/home/da/projects/report.tm" . "/home/da/projects/report.stem")
      ("/home/da/文档/测试.tmu" . "/home/da/文档/测试.stem")
      ("/tmp/my_paper.tmu" . "/tmp/my_paper.stem"))
  ) ;for-each
) ;define

(tm-define (regtest-extract-style-package)
  (test-source-macros-menu-items)
  (test-extract-style-package-stem-buffer)
  (test-style-package-compute-target)
  (check-report)
) ;tm-define
