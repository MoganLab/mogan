;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0828.scm
;; DESCRIPTION : UTF-8 raw 插件 I/O 序列化器测试
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (scheme base)
        (liii check)
        (liii string))

(check-set-mode! 'report-failed)

;;; ========== 辅助函数 ==========

;; 由含 UTF-8 字符串标签的 stree 直接构造 UTF-8 tree
(define (utf8-tree label)
  (stree->tree `(document ,label)))

;; 加载插件 init 脚本（注册其自定义 serializer）
(define (load-plugin-init name)
  (load (string-append (url->system (get-texmacs-path))
                       "/plugins/" name "/progs/init-" name ".scm")))

;; 用 UTF-8 stree 调 plugin-serialize（plugin-serialize 约定接收 stree）
(define (plugin-serialize-utf8 lan label)
  (plugin-serialize lan `(document ,label)))

;;; ========== texmacs->utf8raw / utf8raw->texmacs 往返 ==========

(define (test-utf8raw-roundtrip)
  (define (roundtrip s)
    (texmacs->utf8raw (utf8raw->texmacs s)))
  (check (roundtrip "中文测试") => "中文测试")
  (check (roundtrip "αβγδ") => "αβγδ")
  (check (roundtrip "Hello 世界") => "Hello 世界")
  (check (roundtrip "∑∏∫√") => "∑∏∫√")
  (check (roundtrip "🎉🎊") => "🎉🎊")
  (check (roundtrip "line1\nline2") => "line1\nline2")
  (check (roundtrip "a\n\nb") => "a\n\nb")
  (check (roundtrip "") => "")
  (check (roundtrip "  ") => "  "))

;;; ========== 由 UTF-8 tree 序列化 ==========

(define (test-utf8raw-from-tree)
  (check (texmacs->utf8raw (utf8-tree "中文测试")) => "中文测试")
  (check (texmacs->utf8raw (utf8-tree "αβγδ")) => "αβγδ")
  (check (texmacs->utf8raw (utf8-tree "Hello 世界")) => "Hello 世界")
  (check (texmacs->utf8raw (utf8-tree "∑∏∫√")) => "∑∏∫√")
  (check (texmacs->utf8raw (stree->tree '(document "line1" "line2"))) => "line1\nline2"))

;;; ========== utf8raw->texmacs 保留原始字节 ==========

(define (test-utf8raw-to-tree)
  ;; utf8: 块里的 <#XXXX> 字面必须原样保留，不得被解码
  (define t (utf8raw->texmacs "<#5206><#5B50>"))
  (check (texmacs->utf8raw t) => "<#5206><#5B50>")
  ;; CR/LF 归一化
  (check (texmacs->utf8raw (utf8raw->texmacs "a\r\nb")) => "a\nb")
  (check (texmacs->utf8raw (utf8raw->texmacs "a\rb")) => "a\nb"))

;;; ========== 默认 serializer 输出 UTF-8 raw ==========

(define (test-utf8raw-serialize-utf8)
  (define s (utf8raw-serialize "python" (utf8-tree "print('中文')")))
  (check (string-contains? s "中文") => #t)
  (check (string-contains? s "<#4E2D>") => #f))

(define (test-generic-serialize-utf8)
  (define s (generic-serialize "python" (utf8-tree "print('中文')")))
  (check (string-starts? s (char->string #\x02)) => #t)
  (check (string-contains? s "utf8:") => #t)
  (check (string-contains? s "中文") => #t)
  (check (string-contains? s "<#4E2D>") => #f)
  (check (string-ends? s (char->string #\x05)) => #t))

;;; ========== 各插件自定义 serializer 输出 UTF-8 ==========

(define (test-python-serialize-utf8)
  (load-plugin-init "python")
  (with s (plugin-serialize-utf8 "python" "print('中文')")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-julia-serialize-utf8)
  (load-plugin-init "julia")
  (with s (plugin-serialize-utf8 "julia" "println(\"中文\")")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-goldfish-serialize-utf8)
  (load-plugin-init "goldfish")
  (with s (plugin-serialize-utf8 "goldfish" "(display \"中文\")")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-gnuplot-serialize-utf8)
  (load-plugin-init "gnuplot")
  (with s (plugin-serialize-utf8 "gnuplot" "set title \"中文\"")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-autosave-serialize-utf8)
  (load-plugin-init "autosave")
  (with s (plugin-serialize-utf8 "autosave" "(display \"中文\")")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-tikz-serialize-utf8)
  (load-plugin-init "tikz")
  (with s (plugin-serialize-utf8 "tikz" "\\node {中文};")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-quiver-serialize-utf8)
  (load-plugin-init "quiver")
  (with s (plugin-serialize-utf8 "quiver" "\\node {中文};")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

;; Maxima 经 (plugin-configure :serializer ...) 注册，依赖二进制存在；测试环境无二进制，
;; plugin-serialize 会落回默认 utf8raw-serialize 而测不到 maxima-serialize，故直接调用。
(define (test-maxima-serialize-utf8)
  (load-plugin-init "maxima")
  (with s (maxima-serialize "maxima" '(document "x:中文"))
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (check (string-ends? s ";\n") => #t))
  (check (maxima-serialize "maxima" '(document "")) => "0;\n")
  (with s (maxima-serialize "maxima" '(document "中文$"))
    (check (string-ends? s "中文$\n") => #t)))

;;; ========== 输入端：utf8raw->texmacs 后转 Herk ==========

;; 模拟 input.cpp：把 utf8: 负载解析成 UTF-8 tree，再转成内部 Herk tree。
(define (test-input-utf8-to-herk)
  (define herk-str (texmacs->stm (utf8-tree->herk-tree (utf8raw->texmacs "print('中文')"))))
  (check (string-contains? herk-str "<#4E2D>") => #t)
  (check (string-contains? herk-str "<#6587>") => #t)
  (check (string-contains? herk-str "中文") => #f))

;; emoji（BMP 外）在 utf8_to_herk 映射表里没有对应项，tree_utf8_to_herk 须以 <#XXXX>
;; 保留而非丢弃——这是 0828 相对 0827（走 utf8_to_cork 会静默丢失此类字符）的关键修正。
(define (test-input-emoji-to-herk)
  (define herk-str (texmacs->stm (utf8-tree->herk-tree (utf8raw->texmacs "x:🎉"))))
  (check (string-contains? herk-str "<#1F389>") => #t)
  (check (string-contains? herk-str "🎉") => #f))

;; 普通文本与 <#XXXX> 字面混合：<#XXXX> 原样存活（不被当作 UTF-8 字节再解码），CJK 正常往返。
(define (test-utf8raw-mixed-hex-literal)
  (define t (utf8raw->texmacs "代码<#5206>"))
  (check (texmacs->utf8raw t) => "代码<#5206>"))

;;; ========== 写出端：Herk tree -> UTF-8 tree -> raw 串 ==========

;; 模拟主进程到插件的路径：内部 Herk tree 先转 UTF-8 tree，再由 texmacs->utf8raw 序列化。
(define (test-write-path-herk-to-utf8raw)
  (define s (texmacs->utf8raw
              (herk-tree->utf8-tree
                (stm-snippet->texmacs "(frac \"<#5206><#5B50>\" \"<#5206><#6BCD>\")"))))
  (check (string-contains? s "分子") => #t)
  (check (string-contains? s "分母") => #t)
  (check (string-contains? s "<#5206>") => #f))

;;; ========== input-done? 旁路：序列化前 Herk stree 转 UTF-8 ==========

;; session-edit.scm / program-edit.scm 在 input-done? 路径上，先把 pre（Herk stree）转成
;; UTF-8 stree 再 plugin-serialize。下方正向断言验证转换后输出 UTF-8；反向断言证明不转换
;; 则 Herk 的 <#XXXX> 字面会原样漏给插件（插件收到 8 个 ASCII 字符 "<#4E2D>" 而非"中"），
;; 即该转换是必需的、不可省略。
(define (test-input-done-herk-to-utf8)
  (load-plugin-init "python")
  (define herk-stree '(document "print('<#4E2D><#6587>')"))
  (define pre-u8 (tree->stree (herk-tree->utf8-tree (stree->tree herk-stree))))
  (check (string-contains? (plugin-serialize "python" pre-u8) "中文") => #t)
  (check (string-contains? (plugin-serialize "python" pre-u8) "<#4E2D>") => #f)
  (define bad (plugin-serialize "python" herk-stree))
  (check (string-contains? bad "<#4E2D>") => #t)
  (check (string-contains? bad "中文") => #f))

;;; ========== 测试入口 ==========

(tm-define (test_0828)
  (test-utf8raw-roundtrip)
  (test-utf8raw-from-tree)
  (test-utf8raw-to-tree)
  (test-utf8raw-serialize-utf8)
  (test-generic-serialize-utf8)
  (test-python-serialize-utf8)
  (test-julia-serialize-utf8)
  (test-goldfish-serialize-utf8)
  (test-gnuplot-serialize-utf8)
  (test-autosave-serialize-utf8)
  (test-tikz-serialize-utf8)
  (test-quiver-serialize-utf8)
  (test-maxima-serialize-utf8)
  (test-input-utf8-to-herk)
  (test-input-emoji-to-herk)
  (test-utf8raw-mixed-hex-literal)
  (test-write-path-herk-to-utf8raw)
  (test-input-done-herk-to-utf8)
  (check-report))
