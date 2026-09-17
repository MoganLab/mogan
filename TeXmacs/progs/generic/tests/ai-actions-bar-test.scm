;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ai-actions-bar-test.scm
;; DESCRIPTION : ai-selection-only-images? 纯逻辑测试（树形状契约）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(check-set-mode! 'report-failed)
(load "./TeXmacs/progs/generic/ai-actions-bar.scm")

(define img
  '(image "$TEXMACS_PATH/misc/images/new-mogan-128.png" "77pt" "77pt" "" "")
) ;define

(define (test-only-images)
  ;; 纯文本 / 空白 / 空选区：不是「只有图片」
  (check (ai-selection-only-images? (tm->tree "hello")) => #f)
  (check (ai-selection-only-images? (tm->tree "")) => #f)
  (check (ai-selection-only-images? (tm->tree "  ")) => #f)
  ;; 单张图片、容器包裹、空白 padding：只有图片
  (check (ai-selection-only-images? (tm->tree img)) => #t)
  (check
    (ai-selection-only-images?
      (tm->tree `(document ,img))
    ) ;ai-selection-only-images?
    =>
    #t
  ) ;check
  (check
    (ai-selection-only-images?
      (tm->tree `(concat ,img ,img))
    ) ;ai-selection-only-images?
    =>
    #t
  ) ;check
  (check
    (ai-selection-only-images?
      (tm->tree `(document ," " ,img ,""))
    ) ;ai-selection-only-images?
    =>
    #t
  ) ;check
  ;; 图片夹杂文字/其它内容：照常弹出
  (check
    (ai-selection-only-images?
      (tm->tree `(concat ,img ,"text"))
    ) ;ai-selection-only-images?
    =>
    #f
  ) ;check
  (check
    (ai-selection-only-images?
      (tm->tree `(document ,img ,"x"))
    ) ;ai-selection-only-images?
    =>
    #f
  ) ;check
  ;; with 包裹（参数串非空白）：保守判 #f，照常弹出
  (check
    (ai-selection-only-images?
      (tm->tree `(with ,"color" ,"red" ,img))
    ) ;ai-selection-only-images?
    =>
    #f
  ) ;check
) ;define

(tm-define (regtest-ai-actions-bar) (test-only-images) (check-report))
