
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : prog-edit.scm
;; DESCRIPTION : editing verbatim programs
;; COPYRIGHT   : (C) 2008  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (prog prog-edit)
  (:use (utils library tree)
        (utils library cursor)
        (utils edit selections)
        (generic document-style)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic routines for textual programs
;; WARNING: most of these fail for non-verbatim content!
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (inside-program?)
  (:synopsis "are we inside the line of a textual document?")
  (let* ((ct (cursor-tree))
         (dt (tree-ref ct :up)))
    (and (tree-atomic? ct) (tree-is? dt 'document))))

(tm-define (code-popup-copy t)
  (:synopsis "Copy code tree to clipboard")
  (when (tree-select t)
    (clipboard-copy-export "verbatim" "primary")
    (selection-cancel)))

(define code-popup-inline-options
  '(("verbatim" "verbatim")
    ("scheme" "scm")
    ("c++" "cpp")
    ("r" "r")
    ("python" "python")
    ("shell" "shell")
    ("bash" "bash")
    ("csv" "csv")
    ("gnuplot" "gnuplot")
    ("goldfish" "goldfish-lang")
    ("java" "java")
    ("javascript" "javascript")
    ("json" "json")
    ("julia" "julia")
    ("lua" "lua")
    ("matlab" "matlab")
    ("moonbit" "moonbit")
    ("r7rs" "r7rs")
    ("scala" "scala")
    ("sql" "sql")))

(define code-popup-block-options
  '(("verbatim" "verbatim-code")
    ("scheme" "scm-code")
    ("c++" "cpp-code")
    ("r" "r-code")
    ("python" "python-code")
    ("shell" "shell-code")
    ("bash" "bash-code")
    ("csv" "csv-code")
    ("gnuplot" "gnuplot-code")
    ("goldfish" "goldfish-code")
    ("java" "java-code")
    ("javascript" "javascript-code")
    ("json" "json-code")
    ("julia" "julia-code")
    ("lua" "lua-code")
    ("matlab" "matlab-code")
    ("moonbit" "moonbit-code")
    ("r7rs" "r7rs-code")
    ("scala" "scala-code")
    ("sql" "sql-code")))

(define code-popup-listing-options
  '(("verbatim" "listing")
    ("c++" "cpp-listing")
    ("scheme" "scm-listing")
    ("shell" "shell-listing")))

(define (code-popup-tag-in? tag opts)
  (cond ((null? opts) #f)
        ((string=? tag (cadr (car opts))) #t)
        (else (code-popup-tag-in? tag (cdr opts)))))

(define (code-popup-tag-kind tag)
  (cond ((string=? tag "code") 'block)
        ((code-popup-tag-in? tag code-popup-listing-options) 'listing)
        ((code-popup-tag-in? tag code-popup-block-options) 'block)
        ((code-popup-tag-in? tag code-popup-inline-options) 'inline)
        (else #f)))

(tm-define (code-popup-language-current t)
  (:synopsis "Return current code tag name")
  (symbol->string (tree-label t)))

(define code-popup-packages
  '("bash" "csv" "gnuplot" "goldfish" "java" "javascript" "json" "julia" "lua"
    "matlab" "moonbit" "octave" "python" "r" "r7rs" "scala" "sql"))

(define (code-popup-base-tag tag)
  (cond ((string-ends? tag "-code") (string-drop-right tag 5))
        ((string-ends? tag "-listing") (string-drop-right tag 8))
        ((string-ends? tag "-lang") (string-drop-right tag 5))
        (else tag)))

(define (code-popup-ensure-package tag)
  (let* ((base (code-popup-base-tag tag))
         (pack (and (in? base code-popup-packages) base)))
    (when (and pack (not (has-style-package? pack)))
      (add-style-package pack))))

(tm-define (code-popup-make tag)
  (:synopsis "Make code tag with auto package")
  (code-popup-ensure-package (symbol->string tag))
  (make tag))

(tm-define (code-popup-language-options t)
  (:synopsis "Return menu options for code popup")
  (let* ((tag (symbol->string (tree-label t)))
         (kind (code-popup-tag-kind tag))
         (opts (cond ((eq? kind 'listing) code-popup-listing-options)
                     ((eq? kind 'block) code-popup-block-options)
                     (else code-popup-inline-options))))
    (map (lambda (it) (string-append (car it) "\t" (cadr it))) opts)))

(tm-define (code-popup-set-language t target)
  (:synopsis "Change code tag for current code tree")
  (let* ((kind (code-popup-tag-kind (symbol->string (tree-label t))))
         (opts (cond ((eq? kind 'listing) code-popup-listing-options)
                     ((eq? kind 'block) code-popup-block-options)
                     (else code-popup-inline-options))))
    (when (code-popup-tag-in? target opts)
      (code-popup-ensure-package target)
      (tree-assign-node! t (string->symbol target)))))

(tm-define (program-tree)
  (:synopsis "get the entire program tree")
  (let* ((ct (cursor-tree))
         (dt (tree-ref ct :up)))
    (and (tree-atomic? ct) (tree-is? dt 'document) dt)))

(tm-define (program-row row)
  (:synopsis "get the string at a given @row")
  (and-with doc (program-tree)
    (and-with par (tree-ref doc row)
      (and (tree-atomic? par) (tree->string par)))))

(tm-define (program-row-number)
  (:synopsis "get the vertical position on the current line")
  (and (inside-program?) (cADr (cursor-path))))

(tm-define (program-column-number)
  (:synopsis "get the horizontal position on the current line")
  (and (inside-program?) (cAr (cursor-path))))

#|
program-go-to
将光标移动到指定行和列的位置。

语法
----
(program-go-to row col)

参数
----
row : integer
目标行号（0-based）。

col : integer
目标列号（0-based）。

返回值
----
#<unspecified> 或 #f
- 如果成功移动光标，返回 #<unspecified>
- 如果无法移动光标（参数无效或不在代码模式中），返回 #f

逻辑
----
1. 获取程序文档树 (program-tree)
2. 调用 tree-go-to 将光标移动到指定位置

边界情况处理
------------
当 row 或 col 超出有效范围时：

1. row 超出范围：
   - 如果 row < 0：tree-ref 返回 #f，导致 tree->path 返回 #f，最终 tree-go-to 返回 #f
   - 如果 row >= 文档行数：tree-ref 返回 #f，导致 tree->path 返回 #f，最终 tree-go-to 返回 #f

2. col 超出范围：
   - 如果 col < 0：tree->path 会创建路径，但 go-to 可能将光标移动到行的开头
   - 如果 col >= 行长度：tree->path 会创建路径，但 go-to 可能将光标移动到行的末尾

具体行为取决于 tree->path 和 go-to 的内部实现。通常：
- col 为负值会被视为 0
- col 超过行长度会被视为行末尾位置

注意
----
- 此函数仅在代码模式中有效，需要 program-tree 返回有效的文档树
- 行号和列号都是 0-based
- 如果参数无效，函数会静默失败（返回 #f），不会抛出错误
|#
(tm-define (program-go-to row col)
  (:synopsis "go to the character at a given @row and @col")
  (and-with doc (program-tree)
    (tree-go-to doc row col)))

#|
program-character
获取指定路径位置的字符。

语法
----
(program-character path)

参数
----
path : list
树路径，格式为 (行路径 . 列位置)。

返回值
----
character 或 #\nul
- 如果路径有效且位置在字符串范围内，返回对应字符
- 如果字符串为空、位置超出范围或位置为负，返回空字符 (#\nul)

逻辑
----
1. 从路径中提取行路径 (cDr path) 并转换为字符串
2. 从路径中提取列位置 (cAr path)
3. 检查边界条件：
   - 字符串是否为空 (string-null?)
   - 位置是否超出字符串长度 (>= pos (string-length s))
   - 位置是否为负数 (< pos 0)
4. 如果任何边界条件为真，返回 #\nul
5. 否则返回字符串中指定位置的字符

边界情况处理
------------
- 空字符串：返回 #\nul
- 位置超出字符串长度：返回 #\nul
- 负位置：返回 #\nul

注意
----
- 此函数用于安全地访问程序文本中的字符，避免索引越界错误
- 返回 #\nul（空字符）作为错误指示符
- 路径格式应符合树路径规范
|#
(tm-define (program-character path)
  (let ((s (tree->string (path->tree (cDr path))))
        (pos (cAr path)))
    (if (or (string-null? s) (>= pos (string-length s)) (< pos 0)) #\nul
        (string-ref s pos))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences for bracket handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define prog-auto-close-brackets? #f)
(tm-define prog-highlight-brackets? #f)
(tm-define prog-select-brackets? #f)

(define (notify-auto-close-brackets var val)
  (set! prog-auto-close-brackets? (== val "on")))
(define (notify-highlight-brackets var val)
  (set! prog-highlight-brackets? (== val "on")))
(define (notify-select-brackets var val)
  (set! prog-select-brackets? (== val "on")))

(define-preferences
  ("prog:automatic brackets" "off" notify-auto-close-brackets)
  ("prog:highlight brackets" "on" notify-highlight-brackets)
  ("prog:select brackets" "off" notify-select-brackets))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Bracket handling for strings
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (string-bracket-find* s pos inc br ibr level)
  ;(display* "find: pos= " pos ", level= " level "\n")
  (cond ((or (< pos 0) (>= pos (string-length s))) (- level 1000))
        ((and (== level 0) (== (string-ref s pos) br))
         ;(display* "returning at " pos "\n")
         pos)
        ((== (string-ref s pos) br)
         ;(display* "found at " pos "\n")
         (string-bracket-find* s (+ pos inc) inc br ibr (- level 1)))
        ((== (string-ref s pos) ibr)
         (string-bracket-find* s (+ pos inc) inc br ibr (+ level 1)))
        (else (string-bracket-find* s (+ pos inc) inc br ibr level))))

(define (string-bracket-find s pos inc br ibr level)
  (with r (string-bracket-find* s pos inc br ibr level)
    (and (>= r 0) r)))

(tm-define (string-bracket-level s pos inc br ibr)
  (with ret (string-bracket-find* s pos inc br ibr 0)
    (if (< ret 0) (+ ret 1000)
        (string-bracket-level s (+ ret inc) inc br ibr))))

(tm-define (string-bracket-forward s pos br ibr)
  (:synopsis "find next bracket @br with inverse @ibr in @s at @pos")
  (string-bracket-find s pos 1 br ibr 0))

(tm-define (string-bracket-backward s pos br ibr)
  (:synopsis "find previous bracket @br with inverse @ibr in @s at @pos")
  (string-bracket-find s pos -1 br ibr 0))

(define (program-bracket-find row col inc br ibr level)
  (and-with s (program-row row)
    (with ret (string-bracket-find* s col inc br ibr level)   
      (if (>= ret 0) (cons row ret)
	  (with level* (+ ret 1000)
	    (and-with s* (program-row (+ row inc))
	      (with col* (if (> inc 0) 0 (- (string-length s*) 1))
		(program-bracket-find (+ row inc) col* inc
				      br ibr level*))))))))

(tm-define (program-previous-match row br ibr)
  (:synopsis "find matching opening row for @row and bracket @br")
  (let* ((s (program-row row))
         (last (- (string-length s) 1)))
    (if (not s) row
        (with ret (string-bracket-level s last -1 br ibr)
          (if (== ret 0) row
              (with pos (program-bracket-find row last -1 br ibr -1)
                (if (not pos) row
                    (car pos))))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Bracket handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (path++ p)
  (rcons (cDr p) (+ 1 (cAr p))))

(define (path-- p)
  (rcons (cDr p) (- (cAr p) 1)))

(tm-define (select-brackets path lb rb)
  (:synopsis "Highlights innermost matching brackets around given @path")
  (let ((prev (find-left-bracket path lb rb))
        (next (find-right-bracket path lb rb)))
    (if (or (null? prev) (null? next))
      (if (nnull? (get-alt-selection "brackets"))
          (cancel-alt-selection "brackets"))
      (set-alt-selection "brackets" 
                         (list prev (path++ prev) next (path++ next))))))

(define (string-ref* s i)
  (char->string (string-ref s i)))

(tm-define (select-brackets-after-movement lbs rbs esc)
  (:synopsis "Highlight any of @lbs (matching @rbs) after a cursor movement")
  (let* ((p (cursor-path))
         (p* (path-- p))
         (ch (program-character p))
         (lch (program-character p*))
         (i1 (string-index lbs ch))
         (i2 (string-index rbs ch))
         (i3 (string-index rbs lch)))
    (cond (i1 (select-brackets p (string-ref* lbs i1) (string-ref* rbs i1)))
          (i2 (select-brackets p* (string-ref* lbs i2) (string-ref* rbs i2)))
          (i3 (select-brackets p* (string-ref* lbs i3) (string-ref* rbs i3)))
          ((nnull? (get-alt-selection "brackets"))
           (cancel-alt-selection "brackets")))))

(tm-define (bracket-open lb rb esc)
  (if prog-auto-close-brackets?
      (if (selection-active-normal?)
          (begin
            (clipboard-cut "temp")
            (insert-go-to (string-append lb rb) '(1))
            (clipboard-paste "temp"))
          (with ch (or (before-cursor) "")
            ; Don't create right bracket if prev char is escape char
            (if (== ch esc)
                (insert lb)
                (insert-go-to (string-append lb rb) '(1)))))
      (insert lb))
  (if prog-highlight-brackets? (select-brackets (cursor-path) lb rb)))

; TODO: warn if unmatched
(tm-define (bracket-close lb rb esc)
  (with p (cursor-path)
    (insert rb)
    (if prog-highlight-brackets? (select-brackets p lb rb))))

; HACK! I'd like to use selection-active-enlarging? But current C++ code
; doesn't mix well with this selection mechanism. Also: if mouse selections
; ever use program-select-enlarge, this has to be disabled.
(define kbd-select-enlarging 0) ; 0=no, 1=started, 2=selecting

(tm-define (program-select-enlarge lb rb)
  (if (== 0 kbd-select-enlarging)
      (set! kbd-select-enlarging 1)
      (if (and (not (selection-active-any?)) (== 2 kbd-select-enlarging))
          (set! kbd-select-enlarging 1)
          (let* ((start (selection-get-start))
                 (end (selection-get-end))
                 (start* (if (== start end) start (path-- start)))
                 (prev (find-left-bracket start* lb rb))
                 (next (find-right-bracket end lb rb)))
            (if (or (and (== start prev) (== end next))
                    (null? prev) (null? next))
                (begin
                  (set! kbd-select-enlarging 0)
                  (selection-cancel))
                (begin
                  (set! kbd-select-enlarging 2)
                  (selection-set prev (path++ next))))))))

; Cancel any active selection when we leave a code fragment
(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:require (not (in-prog?)))
  (if (nnull? (get-alt-selection "brackets"))
      (cancel-alt-selection "brackets")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Whitespace handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-preferences
  ("editor:verbatim:tabstop" 4 (lambda (pref val) (noop))))

(define (char-space? c)
  (== c #\space))

(define (char-non-space? c)
  (!= c #\space))

(tm-define (string-space? s)
  (:synopsis "does @s only contain spaces?")
  (list-and (map char-space? (string->list s))))
      
(tm-define (get-tabstop)
  (with tabstop* (get-preference "editor:verbatim:tabstop")
    (cond ((and (string? tabstop*) (string->number tabstop*))
           (string->number tabstop*))
          ((and (number? tabstop*) (> tabstop* 0)) tabstop*)
          (else (set-message
                 `(replace "Wrong tabstop: %1" ,tabstop*) "User preferences")
                8))))

(tm-define (insert-tabstop)
  (with w (get-tabstop)
    (with fill (- w (remainder (cAr (cursor-path)) w))
      (if (> fill 0) (insert (make-string fill #\space))))))

;;HACK: should rewrite program-indent-line to accept unindent
(tm-define (remove-tabstop)
  (with w (get-tabstop)
    (with c (program-get-indent)
      (if (>= c w) (program-set-indent (- c w))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Automatic indentation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

#|
代码缩进系统架构

代码缩进系统采用分层设计：

1. 用户接口层: insert-return - 响应回车键事件
2. 控制层: program-indent - 协调缩进流程
3. 逻辑层: program-indent-line - 执行单行缩进
4. 算法层: program-compute-indentation - 计算缩进规则（可扩展）
5. 工具层: program-get-indent / program-set-indent - 基础操作

这种设计允许不同编程语言通过重写 program-compute-indentation 来实现特定的缩进规则，
而其他层次保持通用。
|#

(tm-define (string-get-indent s)
  (:synopsis "get the indentation of @s")
  (with pos (list-find-index (string->list s) char-non-space?)
    (or pos (string-length s))))

(tm-define (string-set-indent s i)
  (:synopsis "set the indentation of @s to @i spaces")
  (let* ((l (make-string i #\space))
         (r (substring s (string-get-indent s) (string-length s))))
    (string-append l r)))

#|
program-get-indent
获取当前行的缩进级别。

语法
----
(program-get-indent)

返回值
----
integer 或 #f
当前行的缩进空格数，如果不在代码模式中则返回 #f。

逻辑
----
1. 检查是否在代码模式中 (inside-program?)
2. 获取当前行文本 (program-row)
3. 使用 string-get-indent 计算缩进空格数
|#
(tm-define (program-get-indent)
  (:synopsis "get the indentation of the current line")
  (and (inside-program?)
       (string-get-indent (program-row (program-row-number)))))

#|
program-set-indent
设置当前行的缩进级别。

语法
----
(program-set-indent i)

参数
----
i : integer
要设置的缩进空格数。

逻辑
----
1. 检查是否在代码模式中 (inside-program?)
2. 获取当前光标所在的文本树 (cursor-tree)
3. 使用 string-set-indent 设置文本的缩进
4. 更新树节点 (tree-set)

注意
----
此函数直接修改当前行的文本内容，设置指定的缩进空格数。
|#
(tm-define (program-set-indent i)
  (:synopsis "set the indentation of the current line to @i spaces")
  (when (inside-program?)
    (with t (cursor-tree)
      (tree-set t (string-set-indent (tree->string t) i)))))

#|
program-compute-indentation
计算指定行的缩进级别。

语法
----
(program-compute-indentation doc row col)

参数
----
doc : tree
程序文档树。

row : integer
要计算的行号。

col : integer
列位置（当前未使用，固定为-1）。

返回值
----
integer
计算出的缩进级别（空格数）。

逻辑
----
默认返回0，需要在特定语言模块中重新定义。

注意
----
- 这是可扩展的接口，不同编程语言应实现自己的缩进计算逻辑
- 参数 col 当前未使用，保留给未来扩展
|#
(tm-define (program-compute-indentation doc row col)
  (if (<= row 0) 0
      (let ((prev-row (program-row (- row 1))))
        (if prev-row (string-get-indent prev-row) 0))))
#|
program-indent-line
缩进指定行。

语法
----
(program-indent-line doc row unindent?)

参数
----
doc : tree
程序文档树。

row : integer
要缩进的行号（0-based）。

unindent? : boolean
缩进方向：
- #t : 取消缩进
- #f : 正常缩进

返回值
----
integer
缩进后的列位置。

逻辑
----
1. 调用 program-compute-indentation 计算该行的缩进级别
2. 获取该行的文本树 (tree-ref doc row)
3. 使用 string-set-indent 设置文本的缩进
4. 更新树节点 (tree-set)
5. 返回缩进级别

注意
----
- 目前 unindent? 参数尚未实现（TODO注释）
- 这是一个HACK实现，应该修改 program-set-indent 以接受行号参数
|#
(tm-define (program-indent-line doc row unindent?)
  ; TODO: implement unindent for general languages
  (let* ((i (program-compute-indentation doc row -1))
         (t (tree-ref doc row)))
    ; HACK: I should change program-set-indent to accept line numbers
    (tree-set t (string-set-indent (tree->string t) i))
    i))

(tm-define (program-indent-all unindent?)
  (:synopsis "indent a whole program")
  (and-with doc (program-tree)
    (for-each (lambda (r) (program-indent-line doc r unindent?))
              (iota (tree-arity doc)))))

#|
program-indent
缩进当前行。

语法
----
(program-indent unindent?)

参数
----
unindent? : boolean
缩进方向：
- #t : 取消缩进（减少缩进）
- #f : 正常缩进（增加缩进）

返回值
----
#<unspecified>
无显式返回值。

逻辑
----
1. 获取当前程序文档树 (program-tree)
2. 获取当前行号 (program-row-number)
3. 调用 program-indent-line 对当前行进行缩进
4. 将光标移动到缩进后的位置 (program-go-to)

注意
----
此函数是代码缩进的核心接口，通过 program-indent-line 实现具体缩进逻辑。
|#
(tm-define (program-indent unindent?)
  (and-with doc (program-tree)
    (let* ((r (program-row-number))
           (c (program-indent-line doc r unindent?)))
      (program-go-to r c))))

#|
insert-return
代码模式回车键的行为处理函数。

语法
----
(insert-return)

参数
----
无参数。

返回值
----
#<unspecified>
无显式返回值（返回 #<unspecified>）。

逻辑
----
1. 首先调用 insert-raw-return 插入原始换行符
2. 然后调用 program-indent #f 对新行进行缩进

注意
----
此函数仅在代码模式 (in-prog?) 下激活，用于实现代码编辑时的智能缩进功能。
|#
(tm-define (insert-return)
  (:mode in-prog?)
  (insert-raw-return)
  (program-indent #f))
