;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ai-actions-bar-test.scm
;; DESCRIPTION : AI 操作栏纯逻辑测试（树形状契约）：ai-selection-only-images?、
;;               ai-translate-eligible? 与释义上下文的 ai-flatten-text /
;;               ai-split-node / ai-herk-head / ai-herk-tail /
;;               ai-sentinel-split / ai-context->document
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
  ;; 表格/图片换成文本标记（照 ghost 上下文约定）；公式登记子树、留哨兵
  (ai-formula-store-reset!)
  (check (ai-flatten-text '(para "a " (image "x.png") " b")) => "a [IMAGE] b")
  (check
    (ai-flatten-text '(table (row (cell "1"))))
    =>
    "[TABLE]"
  ) ;check
  (check (ai-flatten-text '(para (equation "x"))) => "<#F0G>")
  (check ai-formula-store => '((equation "x")))
) ;define

(define (test-flatten-with-node)
  ;; with：inline 数学登记子树留哨兵；其余只取末尾 body（前面是 key/val 属性对）
  (ai-formula-store-reset!)
  (check
    (ai-flatten-text '(para "see "
                        (with ("mode" "math") (rsub "x" "1"))
                        " here"))
    =>
    "see <#F0G> here"
  ) ;check
  (check ai-formula-store => '((with ("mode" "math") (rsub "x" "1"))))
  (check
    (ai-flatten-text '(para (with ("color" "red") (concat "a" "b"))))
    =>
    "ab"
  ) ;check
) ;define

(define (test-flatten-document-joins-lines)
  ;; document 子节点换行连接：切分拼回 document 时分隔符的来源
  (check (ai-flatten-text '(document (para "one") (para "two"))) => "one\ntwo")
) ;define

;; ===== 按路径切分（ghost 窗口的左右两半）=====

(define (test-split-node)
  ;; 字符串叶子：path 末位是字符偏移
  (check (ai-split-node "hello" '(2)) => '("he" . "llo"))
  ;; document 内跨段切分：分隔符落在切口两侧（before 带尾换行、after 带首换行）
  (check (ai-split-node '(document (para "one") (para "two") (para "three")) '(1
                                                                               0
                                                                               1))
    =>
    '("one\nt" . "wo\nthree")
  ) ;check
  ;; 光标在段首：before 以换行结尾；段尾：after 以换行开头
  (check (ai-split-node '(document (para "a") (para "b")) '(1 0 0))
    =>
    '("a\n" . "b")
  ) ;check
  ;; 公式/表格/图片不透明：光标入内时整体标记归 after（行内内容外面套
  ;; para——document 的直接子节点是段落，裸字符串并列会被加换行分隔）；
  ;; 公式登记子树、留哨兵
  (ai-formula-store-reset!)
  (check
    (ai-split-node '(document (para "see " (equation "x") " here")) '(0 1 0))
    =>
    '("see " . "<#F0G> here")
  ) ;check
  (check ai-formula-store => '((equation "x")))
  ;; inline 数学（with "mode" "math"）同样不透明
  (ai-formula-store-reset!)
  (check
    (ai-split-node
      '(document (para "a" (with ("mode" "math") (rsub "x" "1")) "b"))
      '(0 1 0)
    ) ;ai-split-node
    =>
    '("a" . "<#F0G>b")
  ) ;check
) ;define

;; ===== 截断不切半 herk 的 <#XXXX> 序列 =====

(define (test-herk-truncate)
  ;; 未超预算原样返回
  (check (ai-herk-head "abc" 10) => "abc")
  (check (ai-herk-tail "abc" 10) => "abc")
  ;; 纯 ASCII 直接按字节截断
  (check (ai-herk-head "abcdef" 3) => "abc")
  (check (ai-herk-tail "abcdef" 3) => "def")
  ;; "ab<#4E2D>cd"：<#4E2D> 占字节 2..8；head 切在序列内退到序列起点
  (check (ai-herk-head "ab<#4E2D>cd" 5) => "ab")
  (check (ai-herk-head "ab<#4E2D>cd" 9) => "ab<#4E2D>")
  ;; tail 切在序列内从 '>' 之后开始
  (check (ai-herk-tail "ab<#4E2D>cd" 7) => "cd")
  (check (ai-herk-tail "ab<#4E2D>cd" 2) => "cd")
  ;; 公式哨兵 <#F0G> 同为 <#...> 闭合片段：切在哨兵内时整体让位，不切半
  (check (ai-herk-head "ab<#F0G>cd" 5) => "ab")
  (check (ai-herk-tail "ab<#F0G>cd" 7) => "cd")
) ;define

;; ===== 重组：哨兵换回公式子树 =====

(define (test-sentinel-split)
  (let ((store '((equation "x") (with ("mode" "math") "y"))))
    ;; 无哨兵：整行一个字符串节点；空行得空列表
    (check (ai-sentinel-split "plain" store) => '("plain"))
    (check (ai-sentinel-split "" store) => '())
    ;; 哨兵原位换回登记表中的子树，两侧字符串保留
    (check (ai-sentinel-split "see <#F0G> here" store)
      =>
      '("see " (equation "x") " here")
    ) ;check
    ;; 多个哨兵各自按下标换回
    (check (ai-sentinel-split "a<#F0G>b<#F1G>c" store)
      =>
      '("a" (equation "x") "b" (with ("mode" "math") "y") "c")
    ) ;check
  ) ;let
) ;define

(define (test-context-document)
  (let ((store '((equation "x"))))
    ;; 空上下文：空段落占位
    (check (ai-context->document "" '()) => '(document ""))
    ;; 按行拆段，空行跳过（原 C++ aiTextTree 的约定）
    (check (ai-context->document "line 1\n\nline 3" '())
      =>
      '(document "line 1" "line 3")
    ) ;check
    ;; 段内多节点用 concat 连接，公式子树原位插回
    (check (ai-context->document "see <#F0G> here" store)
      =>
      '(document (concat "see " (equation "x") " here"))
    ) ;check
    ;; 公式独占一段：不套 concat
    (check (ai-context->document "<#F0G>" store) => '(document (equation "x")))
  ) ;let
) ;define

(tm-define (regtest-ai-actions-bar)
  (test-only-images)
  (test-translate-eligible)
  (test-flatten-plain-text)
  (test-flatten-markers)
  (test-flatten-with-node)
  (test-flatten-document-joins-lines)
  (test-split-node)
  (test-herk-truncate)
  (test-sentinel-split)
  (test-context-document)
  (check-report)
) ;tm-define
