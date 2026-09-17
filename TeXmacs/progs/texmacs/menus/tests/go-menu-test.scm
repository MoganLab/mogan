;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : go-menu-test.scm
;; DESCRIPTION : Go 菜单数据契约与 Scheme 接口纯逻辑单元测试
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r go-menu-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/texmacs/menus/file-menu.scm")

(define (test-go-menu-meta-shape)
  (let ((meta (go-menu-meta)))
    (check (list? meta) => #t)
    (check (pair? (assoc "label_recent" meta)) => #t)
    (check (pair? (assoc "recent" meta)) => #t)
    (check (list? (cdr (assoc "recent" meta))) => #t)
  ) ;let
) ;define

(tm-define (regtest-go-menu) (test-go-menu-meta-shape) (check-report))
