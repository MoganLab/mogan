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
    (check (pair? (assoc "label_open" meta)) => #t)
    (check (pair? (assoc "recent" meta)) => #t)
    (check (list? (cdr (assoc "recent" meta))) => #t)
  ) ;let
) ;define

;; File 菜单缺失提示 glue 的接线冒烟：qt-show-missing-recent-toast 可调用，
;; 无活动窗口时安全 no-op，保证 scheme 侧链路（file-list-menu 点击）已接通

(define (test-missing-recent-toast-glue)
  (check (catch #t (lambda () (qt-show-missing-recent-toast) #t) (lambda args #f))
    =>
    #t
  ) ;check
) ;define

(tm-define (regtest-go-menu)
  (test-go-menu-meta-shape)
  (test-missing-recent-toast-glue)
  (check-report)
) ;tm-define
