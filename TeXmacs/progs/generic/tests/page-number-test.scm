;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : page-number-test.scm
;; DESCRIPTION : 纯逻辑单元测试：页码宏生成与回读 round-trip 验证。
;;               测试 make-pn-m/l/g-stree 与 pn-extract-range / pn-extract-style。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r page-number-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/generic/document-widgets.scm")

;; 1. 范围与样式提取：基础规则 (1 ~ 5, roman)

(define (test-pn-roundtrip-roman)
  (let* ((m1 (make-pn-m-stree "pn-m1" "1"))
         (l1 (make-pn-l-stree "pn-l1" "pn-m1" "roman"))
         (g1 (make-pn-g-stree "pn-g1" "pn-g0" "pn-l1" "1" "5"))
         (range (pn-extract-range g1))
         (style (pn-extract-style l1))
        ) ;
    (check range => '("1" . "5"))
    (check style => "roman")
  ) ;let*
) ;define

;; 2. 范围与样式提取：至文末规则 (6 ~ total, arabic)

(define (test-pn-roundtrip-total)
  (let* ((m2 (make-pn-m-stree "pn-m2" "6"))
         (l2 (make-pn-l-stree "pn-l2" "pn-m2" "arabic"))
         (g2 (make-pn-g-stree "pn-g2" "pn-g1" "pn-l2" "6" '(page-the-total)))
         (range (pn-extract-range g2))
         (style (pn-extract-style l2))
        ) ;
    (check range => '("6" . "total"))
    (check style => "arabic")
  ) ;let*
) ;define

;; 3. 样式提取：隐藏页码 (blank)

(define (test-pn-roundtrip-blank)
  (let* ((l-blank '(macro "")) (style (pn-extract-style l-blank)))
    (check style => "blank")
  ) ;let*
) ;define

;; 4. 样式提取：大写罗马与汉字数字

(define (test-pn-styles)
  (let* ((l-Roman (make-pn-l-stree "pn-l1" "pn-m1" "Roman"))
         (l-hanzi (make-pn-l-stree "pn-l2" "pn-m2" "hanzi"))
        ) ;
    (check (pn-extract-style l-Roman) => "Roman")
    (check (pn-extract-style l-hanzi) => "hanzi")
  ) ;let*
) ;define

;; 5. 宏结构生成测试

(define (test-pn-macro-structure)
  (let ((m (make-pn-m-stree "pn-m1" "3")))
    (check (car m) => 'macro)
    (check (cadr m) => '(minus (value "page-nr") "2"))
  ) ;let
) ;define

(tm-define (regtest-page-number)
  (test-pn-roundtrip-roman)
  (test-pn-roundtrip-total)
  (test-pn-roundtrip-blank)
  (test-pn-styles)
  (test-pn-macro-structure)
  (check-report)
) ;tm-define
