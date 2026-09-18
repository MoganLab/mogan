;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ai-actions-bar-test.scm
;; DESCRIPTION : AI 操作栏选区判定纯逻辑测试（树形状契约）
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

;; ---- ai-translate-eligible?：翻译按钮可见性（0995）----
;; 注：复合树用 (tm->tree <stree>) 构造——string->tree 不解析序列化，
;; 只会生成内容为字面串的原子树。

(define (test-translate-eligible)
  ;; 字符数门槛：>= 10 才显示翻译按钮
  (check (ai-translate-eligible? (tm->tree "This is a long enough selection."))
    =>
    #t
  ) ;check
  (check (ai-translate-eligible? (tm->tree "0123456789")) => #t)
  (check (ai-translate-eligible? (tm->tree "012345678")) => #f)
  (check (ai-translate-eligible? (tm->tree "short")) => #f)
  (check (ai-translate-eligible? (tm->tree "短文本")) => #f)
  ;; 纯数学公式选区：不显示
  (check (ai-translate-eligible? (tm->tree '(math "x+y"))) => #f)
  (check
    (ai-translate-eligible? (tm->tree '(equation (document "x"))))
    =>
    #f
  ) ;check
  ;; 含数学但非纯公式：按字符数判定
  (check
    (ai-translate-eligible? (tm->tree '(concat "This is long text " (math "x"))))
    =>
    #t
  ) ;check
) ;define

(tm-define (regtest-ai-actions-bar)
  (test-only-images)
  (test-translate-eligible)
  (check-report)
) ;tm-define
