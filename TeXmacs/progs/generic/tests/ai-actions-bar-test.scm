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
  ;; with 包裹：属性对不算内容，只看末尾 body（1603：居中图片的选区树是
  ;; (with "par-mode" "center" (image ...))，body 只有图片时不弹）
  (check
    (ai-selection-only-images?
      (tm->tree `(with ,"par-mode" ,"center" ,img))
    ) ;ai-selection-only-images?
    =>
    #t
  ) ;check
  (check
    (ai-selection-only-images?
      (tm->tree `(with ,"color" ,"red" ,img))
    ) ;ai-selection-only-images?
    =>
    #t
  ) ;check
  ;; with 的 body 里图片夹杂文字：照常弹出
  (check
    (ai-selection-only-images?
      (tm->tree
        `(with ,"par-mode" ,"center" (concat ,img ,"text"))
      ) ;tm->tree
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
  ;; 图片换文本标记（照 ghost 上下文约定）；公式/表格登记子树、留哨兵
  (ai-subtree-store-reset!)
  (check (ai-flatten-text '(para "a " (image "x.png") " b")) => "a [IMAGE] b")
  (check (ai-flatten-text '(para (equation "x"))) => "<#Z0G>")
  (check ai-subtree-store => '((equation "x")))
  ;; 1607：表格与公式一样整棵登记留哨兵，不再换成 [TABLE] 字面文本；取外层
  ;; tabular 整体（内层 table 会丢包裹），tformat 的 cwith 属性串不外泄
  (let ((tab
          '(tabular (tformat (cwith "1" "1" "-1" "-1" "cell-background"
                               "#eeeeee")
                      (table (row (cell "1") (cell "2")))))
        ) ;tab
       ) ;
    (ai-subtree-store-reset!)
    (check
      (ai-flatten-text `(para ,"see " ,tab ," here"))
      =>
      "see <#Z0G> here"
    ) ;check
    (check ai-subtree-store => (list tab))
  ) ;let
  ;; 裸 table（无包裹兜底）同样登记
  (ai-subtree-store-reset!)
  (check
    (ai-flatten-text '(table (row (cell "1"))))
    =>
    "<#Z0G>"
  ) ;check
  (check ai-subtree-store => '((table (row (cell "1")))))
  ;; 6202：无序列表/有序列表整棵登记为子树留哨兵，(item) 标记与列表结构不丢失
  (let ((item-node
          '(itemize (document (concat (item) "子会：天开始有根")
                      (concat (item) "丑会：地开始凝结")))
        ) ;item-node
       ) ;
    (ai-subtree-store-reset!)
    (check
      (ai-flatten-text `(document (para "前文") ,item-node (para "后文")))
      =>
      "前文\n<#Z0G>\n后文"
    ) ;check
    (check ai-subtree-store => (list item-node))
  ) ;let
) ;define

(define (test-flatten-with-node)
  ;; with：inline 数学登记子树留哨兵；其余只取末尾 body（前面是 key/val 属性对）
  (ai-subtree-store-reset!)
  (check
    (ai-flatten-text '(para "see "
                        (with ("mode" "math") (rsub "x" "1"))
                        " here"))
    =>
    "see <#Z0G> here"
  ) ;check
  (check ai-subtree-store => '((with ("mode" "math") (rsub "x" "1"))))
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
  ;; 公式/表格登记子树、留哨兵
  (ai-subtree-store-reset!)
  (check
    (ai-split-node '(document (para "see " (equation "x") " here")) '(0 1 0))
    =>
    '("see " . "<#Z0G> here")
  ) ;check
  (check ai-subtree-store => '((equation "x")))
  ;; inline 数学（with "mode" "math"）同样不透明
  (ai-subtree-store-reset!)
  (check
    (ai-split-node
      '(document (para "a" (with ("mode" "math") (rsub "x" "1")) "b"))
      '(0 1 0)
    ) ;ai-split-node
    =>
    '("a" . "<#Z0G>b")
  ) ;check
  ;; 1607：表格（外层包裹）同样不透明，光标入内整棵归 after
  (ai-subtree-store-reset!)
  (check
    (ai-split-node
      '(document (para "a" (tabular (table (row (cell "1")))) "b"))
      '(0 1 0)
    ) ;ai-split-node
    =>
    '("a" . "<#Z0G>b")
  ) ;check
  (check ai-subtree-store => '((tabular (table (row (cell "1"))))))
  ;; 6202：无序列表同样不透明，光标入内整棵归 after
  (let ((item-node
          '(itemize (document (concat (item) "第一项")
                      (concat (item) "第二项")))
        ) ;item-node
       ) ;
    (ai-subtree-store-reset!)
    (check
      (ai-split-node `(document (para "a") ,item-node (para "b")) '(1 0 0 1))
      =>
      '("a\n" . "<#Z0G>\nb")
    ) ;check
    (check ai-subtree-store => (list item-node))
  ) ;let
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
  ;; 公式哨兵 <#Z0G> 同为 <#...> 闭合片段：切在哨兵内时整体让位，不切半
  (check (ai-herk-head "ab<#Z0G>cd" 5) => "ab")
  (check (ai-herk-tail "ab<#Z0G>cd" 7) => "cd")
  ;; 相邻 herk 序列（密集 CJK 文本常态）：窗口里先命中的前一个序列已闭合，
  ;; 须继续向后排查（1601 回归：旧实现只看第一个 <#，回退原始字节切割，
  ;; 引文开头出现 "#8A00>" 之类缺 '<' 的半截序列）
  (check (ai-herk-tail "ab<#4E2D><#8A00>cd" 8) => "cd")
  (check (ai-herk-head "ab<#4E2D><#8A00>cd" 10) => "ab<#4E2D>")
) ;define

;; ===== 重组：哨兵换回公式子树 =====

(define (test-sentinel-split)
  (let ((store '((equation "x") (with ("mode" "math") "y"))))
    ;; 无哨兵：整行一个字符串节点；空行得空列表
    (check (ai-sentinel-split "plain" store) => '("plain"))
    (check (ai-sentinel-split "" store) => '())
    ;; 哨兵原位换回登记表中的子树，两侧字符串保留
    (check (ai-sentinel-split "see <#Z0G> here" store)
      =>
      '("see " (equation "x") " here")
    ) ;check
    ;; 多个哨兵各自按下标换回
    (check (ai-sentinel-split "a<#Z0G>b<#Z1G>c" store)
      =>
      '("a" (equation "x") "b" (with ("mode" "math") "y") "c")
    ) ;check
    ;; 哨兵前缀 <#Z 与真实 herk 不撞车（1601 回归）：全角标点等
    ;; U+F000-U+FFFF 的 herk 序列（<#FF08> 等）不是哨兵，整行当普通文本
    ;; 保留（旧实现用 <#F 前缀，把该标点之后的行内容整段丢弃，引文1 段落
    ;; 千疮百孔）
    (check (ai-sentinel-split "a<#FF08>b" '()) => '("a<#FF08>b"))
    (check (ai-sentinel-split "<#FF0C><#FF1A>" '()) => '("<#FF0C><#FF1A>"))
    ;; herk 与哨兵混排：哨兵原位换回，herk 原样保留
    (check (ai-sentinel-split "<#FF0C>x<#Z0G>y" store)
      =>
      '("<#FF0C>x" (equation "x") "y")
    ) ;check
    ;; 形态不合法一律按普通文本：下标越出登记表、<#F123> 这类 PUA herk、
    ;; <#ZG> 无下标
    (check (ai-sentinel-split "<#Z9G>z" store) => '("<#Z9G>z"))
    (check (ai-sentinel-split "q<#F123>r" '()) => '("q<#F123>r"))
    (check (ai-sentinel-split "q<#ZG>r" '()) => '("q<#ZG>r"))
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
    (check (ai-context->document "see <#Z0G> here" store)
      =>
      '(document (concat "see " (equation "x") " here"))
    ) ;check
    ;; 公式独占一段：不套 concat
    (check (ai-context->document "<#Z0G>" store) => '(document (equation "x")))
    ;; 6202：无序列表还原为整棵子树独占一段
    (let* ((lst
             '(itemize (document (concat (item) "项目1")
                         (concat (item) "项目2")))
           ) ;lst
           (lstore (list lst))
          ) ;
      (check (ai-context->document "引言\n<#Z0G>\n结语" lstore)
        =>
        (list 'document "引言" lst "结语")
      ) ;check
    ) ;let*
  ) ;let
) ;define

;; ===== 1601 端到端回归（纯函数层）：含全角标点的多段中文正文 =====

(define (test-context-full-width-punct)
  ;; 选区嵌在末段中间：引文1 各段完整（含以全角括号开头的段），无半截 herk
  (ai-subtree-store-reset!)
  ;; 上下文预算等长（1601：下文 500 字节明显短于上文，拉齐为同一预算）
  (check ai-context-after-limit => ai-context-before-limit)
  (let* ((para-a "<#76F8><#6DF7><#6DC6><#3002> <#5173><#952E><#8BCD><#FF08>3-5 <#4E2A><#FF09><#662F>"
         ) ;para-a
         (para-b "<#FF08><#56DB><#FF09><#76EE><#5F55>")
         (para-c "<#76EE><#5F55><#662F><#6BD5><#4E1A><#8BBE><#8BA1><#FF08><#8BBA><#6587><#FF09>"
         ) ;para-c
         (body (list 'document para-a para-b para-c))
         ;; 选区为 para-c 中「毕业」之外的切口：起点字节 21、终点字节 28
         (at-start (ai-split-node body '(2 21)))
         (at-end (ai-split-node body '(2 28)))
         (before (ai-herk-tail (car at-start) ai-context-before-limit))
         (after (ai-herk-head (cdr at-end) ai-context-after-limit))
         (doc-tree (ai-context->document (string-append before "XY" after) ai-subtree-store)
         ) ;doc-tree
        ) ;
    (check doc-tree
      =>
      (list 'document
        para-a
        para-b
        (string-append (string-take para-c 21) "XY" (string-drop para-c 28))
      ) ;list
    ) ;check
  ) ;let*
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
  (test-context-full-width-punct)
  (check-report)
) ;tm-define
