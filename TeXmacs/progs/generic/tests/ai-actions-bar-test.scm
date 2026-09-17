;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ai-actions-bar-test.scm
;; DESCRIPTION : AI 操作栏纯逻辑测试（树形状契约）：ai-selection-only-images?、
;;               ai-translate-eligible? 与释义上下文展平 ai-flatten-text /
;;               ai-common-prefix
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

;; ===== 释义上下文展平（引文1）=====

(define (test-flatten-plain-text)
  ;; 段落与嵌套 compound 逐层拼接；空节点得空串
  (check (ai-flatten-text '(para "hello " (concat "wor" "ld"))) => "hello world")
  (check (ai-flatten-text '()) => "")
  (check (ai-flatten-text '(para "")) => "")
) ;define

(define (test-flatten-markers)
  ;; 表格/图片/公式换成标记（照 ghost 上下文约定）
  (check (ai-flatten-text '(para "a " (image "x.png") " b")) => "a [IMAGE] b")
  (check
    (ai-flatten-text '(table (row (cell "1"))))
    =>
    "[TABLE]"
  ) ;check
  (check (ai-flatten-text '(para (equation "x"))) => "[FORMULA]")
) ;define

(define (test-flatten-with-node)
  ;; with：inline 数学整体替换；其余只取末尾 body（前面是 key/val 属性对）
  (check
    (ai-flatten-text '(para "see "
                        (with ("mode" "math") (rsub "x" "1"))
                        " here"))
    =>
    "see [FORMULA] here"
  ) ;check
  (check
    (ai-flatten-text '(para (with ("color" "red") (concat "a" "b"))))
    =>
    "ab"
  ) ;check
) ;define

(define (test-flatten-document-joins-lines)
  ;; document 子节点换行连接：跨段选区退回共同祖先时的上下文形态
  (check (ai-flatten-text '(document (para "one") (para "two"))) => "one\ntwo")
) ;define

(define (test-common-prefix)
  ;; 完整包含选区两端的最深节点路径；无公共前缀得空
  (check (ai-common-prefix '(0 1 5) '(0 1 9)) => '(0 1))
  (check (ai-common-prefix '(0 1 5) '(0 2 3)) => '(0))
  (check (ai-common-prefix '(0) '(1)) => '())
  (check (ai-common-prefix '() '(1)) => '())
) ;define

(tm-define (regtest-ai-actions-bar)
  (test-only-images)
  (test-translate-eligible)
  (test-flatten-plain-text)
  (test-flatten-markers)
  (test-flatten-with-node)
  (test-flatten-document-joins-lines)
  (test-common-prefix)
  (check-report)
) ;tm-define
