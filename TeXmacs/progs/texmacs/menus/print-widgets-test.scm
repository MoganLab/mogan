;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : print-widgets-test.scm
;; DESCRIPTION : 纯逻辑单元测试：QML form 引擎的字段表构造（enum-field /
;;               page-setup-form-tree）与 OK 返回值的 tree->stree 解构。
;;               不弹任何 GUI，headless 可跑。GUI 真实交互见 tests/2023.scm。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r print-widgets-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/texmacs/menus/print-widgets.scm")

;; enum-field：value 已在 options 时，options 原样保留（不重复插入）。

(define (test-enum-field-value-in-options)
  (check (enum-field "L:" "k" '("a" "b" "c") "b")
    =>
    '(enum "L:" "k" ("a" "b" "c") "b")
  ) ;check
) ;define

;; enum-field：value 不在 options 时，防御性插到 options 首位。
;; 场景：preference 被外部设成非标准值（如 paper type="Foo"），下拉仍需能选中。

(define (test-enum-field-value-not-in-options)
  (check (enum-field "L:" "k" '("a" "b" "c") "zzz")
    =>
    '(enum "L:" "k" ("zzz" "a" "b" "c") "zzz")
  ) ;check
) ;define

;; page-setup-form-tree：整体形状是 (form <field>...)，共 4 个字段，
;; 每个字段是 (enum <label> <key> (<opts>...) <value>)，arity = 5。

(define (test-page-setup-form-tree-shape)
  (let ((t (page-setup-form-tree)))
    (check (car t) => 'form)
    (check (length (cdr t)) => 4)
    (for-each (lambda (f) (check (car f) => 'enum) (check (length f) => 5)) (cdr t))
  ) ;let
) ;define

;; OK 返回值经 tree->stree 后用 cadr/caddr 解构 key/value。
;; 模拟 cpp_form_dialog 的 OK 返回：(tuple (tuple "k1" "v1") (tuple "k2" "v2"))。

(define (test-ok-result-destructuring)
  (let* ((stree '(tuple (tuple "k1" "v1") (tuple "k2" "v2")))
         (result (stree->tree stree))
         (kvs (cdr (tree->stree result)))
        ) ;
    (check (length kvs) => 2)
    (check (cadr (car kvs)) => "k1")
    (check (caddr (car kvs)) => "v1")
    (check (cadr (cadr kvs)) => "k2")
    (check (caddr (cadr kvs)) => "v2")
  ) ;let*
) ;define

;; Cancel / 关闭返回空 tuple：tree->stree 得 (tuple)，cdr 得 ()，
;; for-each no-op（open-page-setup-window 的写回循环安全）。

(define (test-cancel-empty-result-no-op)
  (let* ((result (stree->tree '(tuple))) (kvs (cdr (tree->stree result))))
    (check kvs => '())
    ;; 模拟 open-page-setup-window 的写回循环：对空列表应不调用 set!。
    (let ((touched #f))
      (for-each (lambda (kv) (set! touched #t)) kvs)
      (check touched => #f)
    ) ;let
  ) ;let*
) ;define

(tm-define (regtest-print-widgets)
  (test-enum-field-value-in-options)
  (test-enum-field-value-not-in-options)
  (test-page-setup-form-tree-shape)
  (test-ok-result-destructuring)
  (test-cancel-empty-result-no-op)
  (check-report)
) ;tm-define
