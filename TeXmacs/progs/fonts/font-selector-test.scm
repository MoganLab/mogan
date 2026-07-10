
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : font-selector-test.scm
;; DESCRIPTION : 纯逻辑单元测试：QML 字体选择器 facade（font-new-widgets.scm 末段
;;               font-selector-* proc）的 specsKey 句柄往返、选项列表常量、
;;               filter/customize meta 形状、分类谓词。不弹 GUI、不依赖字体数据库，
;;               headless 可跑。GUI 真实交互（三栏联动/预览刷新/写回）见
;;               TeXmacs/tests/2027.scm。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   bin/test_only font-selector-test
;;   （等价 xmake b font-selector-test ; xmake r font-selector-test）
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/fonts/font-new-widgets.scm")

;; specsKey 句柄注册表：register -> lookup 往返，key 单调递增。

(define (test-specs-registry-roundtrip)
  (with stub
    (list (lambda (var) "stub") (lambda (changes) (noop)) #t)
    (let ((k1 (font-selector-register-specs stub))
          (k2 (font-selector-register-specs stub))
         ) ;
      (check (font-selector-lookup-specs k1) => stub)
      (check (font-selector-lookup-specs k2) => stub)
      (check (= k2 (+ k1 1)) => #t)
    ) ;let
  ) ;with
) ;define

;; 选项列表常量锁定，防回归（值与 tm-widget 内联一致）。

(define (test-sample-kinds-list)
  (check (length (font-selector-sample-kinds 0)) => 17)
  (check (car (font-selector-sample-kinds 0)) => "Standard")
) ;define

(define (test-sizes-list)
  ;; font-default-sizes：文档字体选择器的字号档，含 5..192 共 26 档。
  (check (length (font-selector-sizes 0)) => 26)
) ;define

;; subfont? 分类：Variants/Mathematics 组的 which 为 #t，Effects 组为 #f。
;; 决定 options 来源（default-subfonts vs font-effect-defaults）。

(define (test-subfont-classification)
  (check (font-selector-subfont? "bold") => #t)
  (check (font-selector-subfont? "math") => #t)
  (check (font-selector-subfont? "frak") => #t)
  (check (font-selector-subfont? "slant") => #f)
  (check (font-selector-subfont? "embold") => #f)
  (check (font-selector-subfont? "hmagnify") => #f)
) ;define

;; filter 选项 / 标签的覆盖与一致性：9 个 filter，每个有 label 与非空 options。

(define (test-filter-options-coverage)
  (check (length font-filter-options) => 9)
  (for-each (lambda (cell)
              (check (string? (font-filter-label (car cell))) => #t)
              (check (and (pair? (cdr cell)) (list? (cdr cell))) => #t)
              (check (member "Any" (cdr cell)) => (cdr cell))
            ) ;lambda
    font-filter-options
  ) ;for-each
) ;define

;; customize meta 形状：6 effects + 5 variants + 5 math = 16 项，每项 5 元素
;; (group label which (options...) value)。

(define (test-customize-meta-shape)
  (let ((meta (append font-effect-meta font-variant-meta font-math-meta)))
    (check (length meta) => 16)
    (for-each (lambda (m) (check (length m) => 3) (check (string? (car m)) => #t))
      meta
    ) ;for-each
  ) ;let
) ;define

(tm-define (regtest-font-selector)
  (test-specs-registry-roundtrip)
  (test-sample-kinds-list)
  (test-sizes-list)
  (test-subfont-classification)
  (test-filter-options-coverage)
  (test-customize-meta-shape)
  (check-report)
) ;tm-define
