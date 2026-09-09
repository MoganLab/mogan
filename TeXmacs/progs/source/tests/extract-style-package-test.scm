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
         (or (menu-contains-label? (car m) label)
             (menu-contains-label? (cdr m) label)))
        ((string? m) (string=? m label))
        (else #f)))

(define (test-source-macros-menu-items)
  (let ((menu (source-macros-menu)))
    ;; 验证已移除 "Extract style file"，仅保留 "Extract style package"
    (check (menu-contains-label? menu "Extract style file") => #f)
    (check (menu-contains-label? menu "Extract style package") => #t)))

(define (test-extract-style-package-stem-buffer)
  (let ((orig-buf (current-buffer)))
    (extract-style-package)
    (let* ((new-buf (current-buffer))
           (name (url->string (url-tail new-buf))))
      ;; 验证新创建的 buffer 是 .stem 后缀的草稿文件
      (check (string-starts? name "draft_") => #t)
      (check (string-ends? name ".stem") => #t)
      (check (url-scratch? new-buf) => #t)
      ;; 验证记录了原文档关联
      (check (style-package-get-origin new-buf) => orig-buf))
    ;; 切回原 buffer 并关闭测试草稿 buffer
    (buffer-close (current-buffer))
    (switch-to-buffer orig-buf)))

(define (test-style-package-target-url)
  (let ((fake-buf (system->url "/home/da/docs/draft_demo.stem"))
        (orig-tmu (system->url "/home/da/docs/paper.tmu"))
        (orig-tm (system->url "/home/da/projects/report.tm"))
        (orig-cjk (system->url "/home/da/文档/测试.tmu"))
       ) ;
    (style-package-set-origin! fake-buf orig-tmu)
    (check (url->system (style-package-target-url fake-buf))
      => "/home/da/docs/paper.stem")

    (style-package-set-origin! fake-buf orig-tm)
    (check (url->system (style-package-target-url fake-buf))
      => "/home/da/projects/report.stem")

    (style-package-set-origin! fake-buf orig-cjk)
    (check (url->system (style-package-target-url fake-buf))
      => "/home/da/文档/测试.stem")
  ) ;let
) ;define

(define (test-extract-style-package-target-resolution)
  (let* ((orig-buf (system->url "/tmp/my_paper.tmu"))
         (new-buf (system->url "/home/da/Documents/LiiiSTEM/no_name/draft_20260909_120000.stem"))
        ) ;
    (style-package-set-origin! new-buf orig-buf)
    (check (url->system (style-package-target-url new-buf))
      => "/tmp/my_paper.stem")
  ) ;let*
) ;define

(tm-define (regtest-extract-style-package)
  (test-source-macros-menu-items)
  (test-extract-style-package-stem-buffer)
  (test-style-package-target-url)
  (test-extract-style-package-target-resolution)
  (check-report))
