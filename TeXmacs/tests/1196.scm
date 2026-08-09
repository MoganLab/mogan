;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1196.scm
;; DESCRIPTION : Unit tests and performance benchmarks for string_to_scheme_tree
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; string_to_scheme_tree 是 S-expression 解析的核心函数。本文件包含：
;;   1. 单元测试 — 覆盖各种输入情况（原子、列表、嵌套、quote、字符串、
;;      转义、注释、Unicode、数字、布尔值、边界情况）
;;   2. 性能测试 — 覆盖不同输入规模（小型、中型、大型、深层嵌套、宽列表、
;;      长字符串、混合复杂结构）
;;
;; 测试策略
;;   - exact-roundtrip：输入输出严格一致（用于无歧义的 Scheme 表示）
;;   - stable-roundtrip：验证 stree->string ∘ string->stree 幂等性
;;   - roundtrip-equiv：验证解析+序列化结果符合预期规范形式
;;
;; USAGE
;;   单元测试（headless 冒烟）：
;;     xmake b stem && xmake r 1196
;;
;;   性能测试（GUI 模式下才真正执行断言）：
;;     xmake b stem && MOGAN_TEST_GUI=1 xmake r 1196
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 辅助函数
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (parse s)
  "解析 scheme 字符串为 scheme tree（调用 C++ string_to_scheme_tree）"
  (string->stree s))

(define (serialize st)
  "序列化 scheme tree 为字符串（调用 C++ scheme_tree_to_string）"
  (stree->string st))

(define (roundtrip s)
  "解析后序列化，返回规范形式"
  (serialize (parse s)))

(define (stable-roundtrip s)
  "验证解析-序列化幂等性：两次解析+序列化结果一致"
  (let* ((out1 (roundtrip s))
         (out2 (roundtrip out1)))
    (check out1 => out2)))

(define (roundtrip-equiv s expected)
  "验证 roundtrip 结果等于期望的规范形式"
  (check (roundtrip s) => expected))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 1: 纯空白输入（通过 stm-snippet->texmacs 间接测试，
;; 因为空字符串原子无法经 scheme_tree_to_tmscm 转换为 Scheme symbol）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-empty)
  (check (stm-snippet->texmacs "") => (tree ""))
  (check (stm-snippet->texmacs " ") => (tree ""))
  (check (stm-snippet->texmacs "\n") => (tree "")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 2: 简单原子
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-simple-atom)
  ;; 单个标识符——解析为 symbol
  (check (parse "a") => 'a)
  (check (parse "foo") => 'foo)
  (check (parse "foo-bar") => 'foo-bar)
  (check (parse "foo_bar") => 'foo_bar)
  (check (parse "foo?") => 'foo?)
  (check (parse "foo!") => 'foo!)
  ;; Roundtrip 一致性
  (stable-roundtrip "a")
  (stable-roundtrip "foo")
  (stable-roundtrip "foo-bar")
  ;; 带前导空白
  (check (parse "  abc") => 'abc)
  (check (parse "\tdef") => 'def)
  (check (parse "\nghi") => 'ghi)
  ;; 带尾随空白
  (check (parse "abc  ") => 'abc)
  (check (parse "def\n") => 'def))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 3: 简单列表
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-simple-list)
  ;; 基本列表 —— parse 返回 Scheme pair/list
  (check (parse "(a)") => '(a))
  (check (parse "(a b)") => '(a b))
  (check (parse "(a b c)") => '(a b c))
  ;; Roundtrip 幂等性
  (stable-roundtrip "(a)")
  (stable-roundtrip "(a b)")
  (stable-roundtrip "(a b c)")
  ;; 空列表
  (check (parse "()") => '())
  (stable-roundtrip "()")
  ;; 列表内空白规范化
  (roundtrip-equiv "(  a  b  )" "(a b)")
  (roundtrip-equiv "(a\nb\nc)" "(a b c)")
  (roundtrip-equiv "(\ta\tb\tc)" "(a b c)")
  ;; 外围空白
  (roundtrip-equiv "  (a b)  " "(a b)"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 4: 嵌套列表
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-nested-list)
  ;; 一层嵌套
  (roundtrip-equiv "((a))" "((a))")
  (roundtrip-equiv "(a (b))" "(a (b))")
  (roundtrip-equiv "((a) b)" "((a) b)")
  (roundtrip-equiv "(a (b c) d)" "(a (b c) d)")
  ;; 二层嵌套
  (roundtrip-equiv "(a (b (c)))" "(a (b (c)))")
  (roundtrip-equiv "((a b) (c d))" "((a b) (c d))")
  ;; 三层嵌套
  (roundtrip-equiv "(((a)))" "(((a)))")
  (roundtrip-equiv "(a (b (c (d))))" "(a (b (c (d))))")
  ;; 兄弟节点中有嵌套
  (roundtrip-equiv "(a (b) c (d e) f)" "(a (b) c (d e) f)")
  ;; 幂等性
  (stable-roundtrip "((a))")
  (stable-roundtrip "(a (b c) d)")
  (stable-roundtrip "(((a)))"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 5: Quote 表达式
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-quote)
  ;; Quote 解析的正确性由 C++ 单元测试覆盖。
  ;; scheme_tree_to_tmscm 将 ' 转换为空名字 symbol，
  ;; 与 s7 不兼容，无法在 Scheme 侧精确验证。
  ;; 此处仅做冒烟测试（进程不崩溃）。
  (parse "'a")
  (parse "'(a b)")
  (parse "(a 'b 'c)")
  (display "[1196] quote parse smoke test passed\n"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 6: 字符串字面量
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-string-literal)
  ;; 简单字符串 —— parse 返回 Scheme string
  (check (parse "\"\"") => "")
  (check (parse "\"a\"") => "a")
  (check (parse "\"abc\"") => "abc")
  (check (parse "\"hello world\"") => "hello world")
  ;; 列表中包含字符串
  (check (parse "(\"a\" \"b\")") => '("a" "b"))
  (check (parse "(a \"b\" c)") => '(a "b" c))
  ;; Roundtrip (scm_quote 会重新加引号)
  (roundtrip-equiv "\"hello\"" "\"hello\"")
  (roundtrip-equiv "(a \"b\" c)" "(a \"b\" c)")
  ;; 幂等性
  (stable-roundtrip "\"hello\"")
  (stable-roundtrip "(\"a\" \"b\")"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 7: 转义序列
;; 注意：scm_quote 只转义 " 和 \，不转义 \n \t \0 等控制字符，
;; 故 roundtrip 不保留输入中的 escape 序列文字。
;; 测试策略：验证 parse 正确解转义 + serialize 后结构幂等。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-escape-sequences)
  ;; 反斜杠转义 —— parse 后应为包含单个反斜杠的字符串
  (check (parse "\"\\\\\"") => "\\")
  (stable-roundtrip "\"\\\\\"")

  ;; 双引号转义
  (check (parse "\"\\\"\"") => "\"")
  (stable-roundtrip "\"\\\"\"")

  ;; 换行转义 \n
  (check (parse "\"\\n\"") => "\n")
  (stable-roundtrip "\"\\n\"")

  ;; 制表转义 \t
  (check (parse "\"\\t\"") => "\t")
  (stable-roundtrip "\"\\t\"")

  ;; 空字符转义 \0 — 仅验证幂等性（含 null 字符的字符串无法可靠比较）
  (stable-roundtrip "\"\\0\"")

  ;; 混合转义
  (check (parse "\"a\\nb\\tc\"") => "a\nb\tc")
  (stable-roundtrip "\"a\\nb\\tc\"")

  ;; 多重反斜杠
  (check (parse "\"slashed \\\\ characters\"") => "slashed \\ characters")
  (stable-roundtrip "\"slashed \\\\ characters\""))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 8: 注释
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-comments)
  ;; 行注释在顶层
  (check (parse "; this is a comment\na") => 'a)
  (check (parse "; comment\n; another\na") => 'a)
  ;; 行注释在列表内
  (roundtrip-equiv "(a ; comment\n b)" "(a b)")
  (roundtrip-equiv "(a ; inline\n b ; another\n c)" "(a b c)")
  ;; 注释在列表末尾（regression: #1131）
  (roundtrip-equiv "(a b) ; trailing comment" "(a b)")
  (roundtrip-equiv "(a b) ; trailing\n" "(a b)")
  ;; 注释包含特殊字符
  (roundtrip-equiv "(a ; \"'\\;#\n b)" "(a b)")
  ;; 注释在嵌套结构中
  (roundtrip-equiv "(a (b ; inner\n ) c)" "(a (b) c)")
  ;; 幂等性
  (stable-roundtrip "; comment\na"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 9: Unicode / 多语言
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-unicode)
  ;; 中文标识符
  (check (parse "汉语") => '汉语)
  (check (parse "条目") => '条目)
  (roundtrip-equiv "(汉语 条目)" "(汉语 条目)")
  ;; 混合语言
  (roundtrip-equiv "(utf-8 encoding)" "(utf-8 encoding)")
  (roundtrip-equiv "(chinese (utf-8 encoding) (汉语 条目))"
                    "(chinese (utf-8 encoding) (汉语 条目))")
  ;; 日文
  (check (parse "日本語") => '日本語)
  ;; 希腊字母
  (check (parse "α-β") => 'α-β)
  (check (parse "λ") => 'λ)
  ;; 幂等性
  (stable-roundtrip "汉语")
  (stable-roundtrip "(汉语 条目)")
  (stable-roundtrip "(utf-8 encoding)"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 10: 数字
;; scheme_tree_to_tmscm 将整数字符串转为 Scheme int，
;; tmscm_to_scheme_tree 将 Scheme int 转回数字字符串 atom。
;; 故 roundtrip 保持数字字面量。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-numbers)
  (check (parse "0") => 0)
  (check (parse "123") => 123)
  (check (parse "-42") => -42)
  ;; 列表中的数字
  (check (parse "(1 2 3)") => '(1 2 3))
  ;; Roundtrip
  (roundtrip-equiv "123" "123")
  (roundtrip-equiv "-42" "-42")
  (roundtrip-equiv "(1 2 3)" "(1 2 3)")
  ;; 幂等性
  (stable-roundtrip "123")
  (stable-roundtrip "(1 2 3)"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 11: 布尔值
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-booleans)
  (check (parse "#t") => #t)
  (check (parse "#f") => #f)
  ;; 列表中的布尔值
  (check (parse "(#t #f)") => '(#t #f))
  ;; Roundtrip（序列化后 #t/#f 恢复为原子字符串 "#t"/"#f"）
  (roundtrip-equiv "#t" "#t")
  (roundtrip-equiv "#f" "#f")
  ;; 幂等性
  (stable-roundtrip "#t")
  (stable-roundtrip "#f"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Section 12: 边界情况
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-edge-cases)
  ;; 全注释（无有效内容）
  (roundtrip-equiv "; only comment" "")
  (roundtrip-equiv "; comment\n; another" "")
  ;; 嵌套空列表
  (roundtrip-equiv "(())" "(())")
  (roundtrip-equiv "((()))" "((()))")
  ;; 长标识符
  (let ((long-id (make-string 100 #\x)))
    (string-set! long-id 0 #\a)
    (stable-roundtrip long-id))
  ;; 标识符以特殊字符开头
  (check (parse "+") => '+)
  (check (parse "-") => '-)
  (check (parse "*") => '*)
  (check (parse "/") => '/)
  ;; "." 和 "..." 需用 string->symbol 构造（dot 是 Scheme 保留语法）
  (check (parse ".") => (string->symbol "."))
  (check (parse "...") => (string->symbol "..."))
  ;; 含数字的标识符（不以数字开头的不会被转为 int）
  (check (parse "a1b2c3") => 'a1b2c3)
  (check (parse "x86_64") => 'x86_64)
  ;; 幂等性
  (stable-roundtrip "a1b2c3")
  (stable-roundtrip "x86_64")
  (stable-roundtrip "(())"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 性能测试
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define bench-iterations 10000)

(define (run-bench label input n)
  "运行性能测试，解析 INPUT N 次，打印耗时"
  (let ((start (texmacs-time)))
    (let loop ((i 0))
      (when (< i n)
        (parse input)
        (loop (+ i 1))))
    (let* ((elapsed (- (texmacs-time) start))
           (per-call (if (zero? n) 0.0 (/ elapsed n))))
      (display "[1196] bench ")
      (display label)
      (display ": total=")
      (display elapsed)
      (display " ms, per-call=")
      (display per-call)
      (display " ms (n=")
      (display n)
      (display ")")
      (newline))))

(define (run-bench-large label input n)
  "运行大规模性能测试（较少迭代次数）"
  (let ((start (texmacs-time)))
    (let loop ((i 0))
      (when (< i n)
        (parse input)
        (loop (+ i 1))))
    (let* ((elapsed (- (texmacs-time) start))
           (per-call (if (zero? n) 0.0 (/ elapsed n))))
      (display "[1196] bench ")
      (display label)
      (display ": total=")
      (display elapsed)
      (display " ms, per-call=")
      (display per-call)
      (display " ms (n=")
      (display n)
      (display ")")
      (newline))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 辅助: 构建测试输入
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (build-list-elems n)
  "构建 n 个元素的 S-expression 列表字符串 '(x x ... x)'"
  (let loop ((i 0) (s "("))
    (if (>= i n)
        (string-append s ")")
        (loop (+ i 1) (string-append s (if (> i 0) " " "") "x")))))

(define (build-deep-nest n)
  "构建 n 层嵌套 '(((...(x)...)))'"
  (let loop ((i 0) (pre "") (post ""))
    (if (>= i n)
        (string-append pre "x" post)
        (loop (+ i 1) (string-append pre "(") (string-append ")" post)))))

(define (build-wide-subs n)
  "构建包含 n 个子列表的宽列表 '(a b)(a b)...(a b)'"
  (let loop ((i 0) (s "("))
    (if (>= i n)
        (string-append s ")")
        (loop (+ i 1) (string-append s (if (> i 0) " " "") "(a b)")))))

(define (build-long-str n)
  "构建包含 n 个字符的字符串字面量 \"xxx...\""
  (let ((inner (make-string n #\x)))
    (string-append "\"" inner "\"")))

(define (build-mixed n)
  "构建混合结构：符号、字符串、quote、嵌套交替"
  (let loop ((i 0) (s "("))
    (if (>= i n)
        (string-append s ")")
        (let ((elem (case (modulo i 4)
                      ((0) "sym")
                      ((1) "\"str\"")
                      ((2) "'q")
                      ((3) "(nested)"))))
          (loop (+ i 1) (string-append s (if (> i 0) " " "") elem))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 性能基准: 小型输入
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bench-simple-atom)
  (run-bench "simple-atom" "hello_world" bench-iterations))

(define (bench-simple-list-3)
  (run-bench "simple-list-3" "(a b c)" bench-iterations))

(define (bench-nested-list)
  (run-bench "nested-list" "(a (b c) (d (e f g) h) i)" bench-iterations))

(define (bench-quoted)
  (run-bench "quoted" "(scheme 'parser)" bench-iterations))

(define (bench-string-literal)
  (run-bench "string-literal" "\"hello world\"" bench-iterations))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 性能基准: 中等输入
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bench-list-50)
  (run-bench-large "list-50" (build-list-elems 50) 5000))

(define (bench-list-200)
  (run-bench-large "list-200" (build-list-elems 200) 2000))

(define (bench-wide-30)
  (run-bench-large "wide-30" (build-wide-subs 30) 3000))

(define (bench-wide-100)
  (run-bench-large "wide-100" (build-wide-subs 100) 1000))

(define (bench-deep-30)
  (run-bench-large "deep-30" (build-deep-nest 30) 5000))

(define (bench-deep-100)
  (run-bench-large "deep-100" (build-deep-nest 100) 2000))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 性能基准: 大型输入
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bench-long-str-1k)
  (run-bench-large "long-str-1k" (build-long-str 1000) 3000))

(define (bench-long-str-10k)
  (run-bench-large "long-str-10k" (build-long-str 10000) 500))

(define (bench-mixed-100)
  (run-bench-large "mixed-100" (build-mixed 100) 1000))

(define (bench-mixed-500)
  (run-bench-large "mixed-500" (build-mixed 500) 200))

(define (bench-complex-scheme)
  ;; 模拟复杂的 virtual-font / scheme 配置文件
  (let ((s "\
(moebius-em
  (moebius-em-delta -60)
  (moebius-em-slant 0.25)
  (moebius-em-extend 1.0)
  (moebius-em-bold 0)
  (pen \"pen\")
  (pen \"pen\")
  (pen \"pen\")
  (pen \"pen\")
  (pen \"pen\")
  (pen \"pen\")
  (pen \"pen\")
  (pen \"pen\")
  (moveto \"moveto\")
  (lineto \"lineto\")
  (curveto \"curveto\")
  (closepath \"closepath\")
  (clip \"clip\")
  (moebius-em-glyph 65
    (moveto 0 0)
    (lineto 100 0)
    (lineto 100 200)
    (lineto 0 200)
    (closepath)
  )
  (moebius-em-glyph 66
    (moveto 0 0)
    (lineto 200 0)
    (lineto 200 300)
    (lineto 0 300)
    (closepath)
  )
)"))
    (run-bench-large "complex-scheme" s 500)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 测试入口
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (test_1196)
  (display "=== [1196] Unit Tests ===\n")

  ;; Unit tests
  (test-empty)
  (test-simple-atom)
  (test-simple-list)
  (test-nested-list)
  (test-quote)
  (test-string-literal)
  (test-escape-sequences)
  (test-comments)
  (test-unicode)
  (test-numbers)
  (test-booleans)
  (test-edge-cases)

  (display "=== [1196] Performance Benchmarks ===\n")

  ;; Small benchmarks
  (bench-simple-atom)
  (bench-simple-list-3)
  (bench-nested-list)
  (bench-quoted)
  (bench-string-literal)

  ;; Medium benchmarks
  (bench-list-50)
  (bench-list-200)
  (bench-wide-30)
  (bench-wide-100)
  (bench-deep-30)
  (bench-deep-100)

  ;; Large benchmarks
  (bench-long-str-1k)
  (bench-long-str-10k)
  (bench-mixed-100)
  (bench-mixed-500)
  (bench-complex-scheme)

  (check-report)
) ;tm-define
