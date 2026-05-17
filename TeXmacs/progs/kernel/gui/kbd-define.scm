
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : kbd-define.scm
;; DESCRIPTION : Definition of keyboard shortcuts/wildcards
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel gui kbd-define) (:use (kernel texmacs tm-define)))

(import (liii hash-table))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Lazy keyboard bindings
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define lazy-keyboard-waiting '())
(tm-define lazy-keyboard-done (make-ahash-table))

(tm-define (lazy-keyboard-do module mode*)
  (with mode
    (texmacs-mode-mode mode*)
    (set! lazy-keyboard-waiting (acons mode module lazy-keyboard-waiting))
  ) ;with
) ;tm-define

(tm-define-macro (lazy-keyboard module . modes)
  (for-each (lambda (mode) (lazy-keyboard-do module mode)) modes)
  `(delayed (:idle 250)
     (ahash-set! lazy-keyboard-done (quote ,module) ,#t)
     (module-provide (quote ,module)))
) ;tm-define-macro

(define lazy-force-all? #f)

(define lazy-force-busy? #f)

(define (lazy-keyboard-force-do l)
  (cond ((null? l) l)
        ((ahash-ref lazy-keyboard-done (cdar l)) (lazy-keyboard-force-do (cdr l)))
        ((or lazy-force-all? (texmacs-in-mode? (caar l)))
         (module-provide (cdar l))
         (ahash-set! lazy-keyboard-done (cdar l) #t)
         (lazy-keyboard-force-do (cdr l))
        ) ;
        (else (cons (car l) (lazy-keyboard-force-do (cdr l))))
  ) ;cond
) ;define

(tm-define (lazy-keyboard-force . opt)
  (set! lazy-force-all? (or lazy-force-all? (nnull? opt)))
  (when (not lazy-force-busy?)
    (set! lazy-force-busy? #t)
    (let* ((l1 (reverse lazy-keyboard-waiting)) (l2 (lazy-keyboard-force-do l1)))
      (set! lazy-keyboard-waiting (reverse l2))
      (set! lazy-force-busy? #f)
      (when (null? lazy-keyboard-waiting)
        (set! lazy-force-all? #f)
      ) ;when
      (when (and lazy-force-all? (nnull? lazy-keyboard-waiting))
        (lazy-keyboard-force #t)
      ) ;when
    ) ;let*
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Definition of keyboard wildcards
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (kbd-wildcards-sub l post)
  (if (nnull? l)
    (let* ((w (car l))
           (key (car w))
           (im (cadr w))
           (left (if (>= (length w) 3) (caddr w) #f))
           (right (if (>= (length w) 4) (cadddr w) #t))
          ) ;
      (insert-kbd-wildcard key im post left right)
      (kbd-wildcards-sub (cdr l) post)
    ) ;let*
  ) ;if
) ;define

(tm-define (kbd-wildcards-body l)
  (:synopsis "Helper routine for kbd-wildcards macro")
  (cond ((null? l) (noop))
        ((== (car l) 'pre) (kbd-wildcards-sub (cdr l) #f))
        ((== (car l) 'post) (kbd-wildcards-sub (cdr l) #t))
        (else (kbd-wildcards-sub l #t))
  ) ;cond
) ;tm-define

(tm-define-macro (kbd-wildcards . l)
  (:synopsis "Add entries in @l to the keyboard wildcard table")
  `(kbd-wildcards-body ,(list 'quasiquote l))
) ;tm-define-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines for the definition of keyboard shortcuts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define kbd-map-table (make-ahash-table))

(define kbd-inv-table (make-ahash-table))

(define kbd-rev-table (make-ahash-table))

(define (kbd-set-map! key im)
  (ahash-set! kbd-map-table key im)
) ;define

(define (kbd-set-inv! key im)
  (ahash-set! kbd-inv-table key im)
) ;define

(define (kbd-set-rev! key im)
  (ahash-set! kbd-rev-table key im)
) ;define

(define (kbd-get-map key)
  (ahash-ref kbd-map-table key)
) ;define

(define (kbd-get-inv key)
  (ahash-ref kbd-inv-table key)
) ;define
(tm-define (kbd-get-rev key) (ahash-ref kbd-rev-table key))

(define (kbd-remove-map! key)
  (ahash-remove! kbd-map-table key)
) ;define

(define (kbd-source cmd)
  (if (procedure? cmd) (promise-source cmd) cmd)
) ;define

(define (simple-insert l x)
  (if (nlist? l) (list x) (list-union (list x) l))
) ;define

(define (simple-remove l x)
  (if (nlist? l) (list) (list-difference l (list x)))
) ;define

(define (kbd-insert-key-binding conds key im)
  (let* ((com (kbd-source (car im))) (cmd (if (string? com) com (object->string com))))
    ;; (display* "Binding '" key "' when " conds " to " com "\n")
    (kbd-delete-key-binding2 conds key)
    (kbd-set-map! key (ctx-insert (kbd-get-map key) im conds))
    (kbd-set-inv! com (ctx-insert (kbd-get-inv com) key conds))
    (kbd-set-rev! cmd (simple-insert (kbd-get-rev cmd) key))
    ;; (display* key " > " (kbd-get-map key) "\n")
    ;; (display* com " < " (kbd-get-inv com) "\n")
    ;; (display* cmd " < " (kbd-get-rev cmd) "\n")
  ) ;let*
) ;define

(tm-define (kbd-delete-key-binding2 conds key)
  ;; (display* "Deleting binding '" key "' when " conds "\n")
  (with im
    (ctx-find (kbd-get-map key) conds)
    (if im
      (let* ((com (kbd-source (car im))) (cmd (object->string com)))
        (kbd-set-map! key (ctx-remove (kbd-get-map key) conds))
        (kbd-set-inv! com (ctx-remove (kbd-get-inv com) conds))
        (kbd-set-rev! cmd (simple-remove (kbd-get-inv cmd) key))
      ) ;let*
    ) ;if
  ) ;with
) ;tm-define

(tm-define (kbd-find-key-binding key)
  (:synopsis "Find the command associated to the keystroke @key")
  ;; (display* "Find binding '" key "'\n")
  (lazy-keyboard-force)
  (ctx-resolve (kbd-get-map key) #f)
) ;tm-define

(tm-define (kbd-find-inv-binding com)
  (:synopsis "Find keyboard binding for command @com")
  ;; (display* "Find inverse binding '" com "'\n")
  (lazy-keyboard-force)
  (with r (ctx-resolve (kbd-get-inv com) #f) (if r r ""))
) ;tm-define

(tm-define (kbd-find-inv-system-binding com)
  (:synopsis "Find system keyboard binding for command @com")
  (and-with b
    (tm->stree (kbd-system-rewrite (kbd-find-inv-binding com)))
    (when (tm-is? b 'render-key)
      (set! b (tm-ref b 0))
    ) ;when
    (when (tm-is? b 'with)
      (set! b (tm-ref b (- (tm-arity b) 1)))
    ) ;when
    (and (string? b) b)
  ) ;and-with
) ;tm-define

(tm-define (kbd-find-rev-binding cmd)
  (:synopsis "Find modeless keyboard binding for command @cmd")
  ;; (display* "Find reverse binding '" com "'\n")
  (lazy-keyboard-force)
  (cond ((tree? cmd) (kbd-find-rev-binding (tree->stree cmd)))
        ((string? cmd)
         (with l
           (kbd-get-rev (object->string (string->object cmd)))
           (and l (nnull? l) (string? (car l)) (string->tmstring (car l)))
         ) ;with
        ) ;
        (else #f)
  ) ;cond
) ;tm-define

(define (kbd-find-key-binding2 conds key)
  ;; (display* "Find binding '" key "' when " conds "\n")
  ;; FIXME: we really need an ctx-find which does mode inference
  (or (ctx-find (kbd-get-map key) conds) (ctx-find (kbd-get-map key) '()))
) ;define

(tm-define (kbd-base-sequence comb)
  (let loop
    ((s comb))
    (if (string-ends? s " tab") (loop (substring s 0 (- (string-length s) 4))) s)
  ) ;let
) ;tm-define

(tm-define (kbd-find-prefix-tab-inner prefix)
  (let* ((pairs (filter (lambda (pair)
                          (let* ((key (car pair)) (val (cdr pair)))
                            (and (string? key)
                              (string-ends? key " tab")
                              (let* ((base (kbd-base-sequence key)) (resolved (ctx-resolve val #f)))
                                (and (string=? base prefix) resolved)
                              ) ;let*
                            ) ;and
                          ) ;let*
                        ) ;lambda
                  (ahash-table->list kbd-map-table)
                ) ;filter
         ) ;pairs
         ;; 将 (key . context-map) 转换为 (key . resolved-value)
         (resolved-pairs (map (lambda (pair) (cons (car pair) (ctx-resolve (cdr pair) #f))) pairs)
         ) ;resolved-pairs
        ) ;
    (sort resolved-pairs
      (lambda (x y)
        (let ((len-x (string-length (car x))) (len-y (string-length (car y))))
          (< len-x len-y)
        ) ;let
      ) ;lambda
    ) ;sort
  ) ;let*
) ;tm-define

(define kbd-find-prefix-tab-cache (make-hash-table))

(tm-define (kbd-find-prefix-tab prefix)
  (let ((cached (hash-table-ref/default kbd-find-prefix-tab-cache prefix #f)))
    (if cached
      cached
      (let ((result (kbd-find-prefix-tab-inner prefix)))
        (hash-table-set! kbd-find-prefix-tab-cache prefix result)
        result
      ) ;let
    ) ;if
  ) ;let
) ;tm-define



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Yet more subroutines for the definition of keyboard shortcuts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (kbd-append prefix s1)
  (let* ((s2 (string-replace s1 " " ""))
         (s3 (string-replace s2 "<" "<."))
         (s4 (string-replace s3 ">" "<gtr>"))
         (s5 (string-replace s4 "<." "<less>"))
        ) ;
    (string-append prefix s5)
  ) ;let*
) ;define

(define (kbd-sub-binding conds s prev-end end)
  (let* ((this-ss (substring s 0 end)) (this (kbd-find-key-binding2 conds this-ss)))
    (if (not this)
      (let* ((prev-ss (substring s 0 prev-end))
             (prev (kbd-find-key-binding2 conds prev-ss))
            ) ;
        (if (and (list? prev) (= (length prev) 2)) (set! prev (car prev)))
        (if (or (not prev) (nstring? prev)) (set! prev prev-ss))
        (with im
          (kbd-append prev (substring s prev-end end))
          (kbd-insert-key-binding conds this-ss (list im ""))
        ) ;with
      ) ;let*
    ) ;if
  ) ;let*
) ;define

(define (kbd-sub-bindings-sub conds s prev-end end)
  (cond ((== end (string-length s)) (noop))
        ((== (string-ref s end) #\space)
         (kbd-sub-binding conds s prev-end end)
         (kbd-sub-bindings-sub conds s end (+ end 1))
        ) ;
        (else (kbd-sub-bindings-sub conds s prev-end (+ end 1)))
  ) ;cond
) ;define

(define (kbd-sub-bindings conds s)
  (kbd-sub-bindings-sub conds s 0 0)
) ;define

(tm-define (kbd-binding conds key2 cmd help)
  (:synopsis "Helper routine for kbd-map macro")
  ;; (display* conds ", " key2 ", " cmd ", " help "\n")
  (with key
    (kbd-pre-rewrite key2)
    (kbd-sub-bindings conds key)
    (kbd-insert-key-binding conds key (list cmd help))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Definition of keyboard shortcuts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (kbd-add-condition conds opt)
  ;; (display* "Add condition " opt "\n")
  (cond ((== (car opt) :mode) (ctx-add-condition conds 0 (cadr opt)))
        ((== (car opt) :require) (ctx-add-condition conds 0 `(lambda ,()
                                                               ,(cadr opt))))
        (else (texmacs-error "kbd-add-condition" "Bad keyboard option ~S" opt))
  ) ;cond
) ;define

(define (kbd-map-one conds l)
  (if (not (and (pair? l) (string? (car l)) (pair? (cdr l))))
    (texmacs-error "kbd-map-pre-one" "Bad keymap in: ~S" l)
  ) ;if
  (with (key action . opt)
    l
    (if (string? action)
      (with help
        (if (null? opt) "" (car opt))
        `(kbd-binding (list ,@conds) ,key ,action ,help)
      ) ;with
      `(kbd-binding (list ,@conds) ,key (lambda ,() ,action ,@opt) ,"")
    ) ;if
  ) ;with
) ;define

(define (kbd-map-body conds l)
  (cond ((null? l) '())
        ((symbol? (car l)) (kbd-map-body (list 0 (car l)) (cdr l)))
        ((and (pair? (car l)) (== (caar l) :profile))
         (if (not (has-look-and-feel? (cdar l))) '((noop)) (kbd-map-body conds (cdr l)))
        ) ;
        ((and (pair? (car l)) (keyword? (caar l)))
         (kbd-map-body (kbd-add-condition conds (car l)) (cdr l))
        ) ;
        (else (map (lambda (x) (kbd-map-one conds x)) l))
  ) ;cond
) ;define

(tm-define-macro (kbd-map . l)
  (:synopsis "Add entries in @l to the keyboard mapping")
  `(begin ,@(kbd-map-body '() l))
) ;tm-define-macro

(define (kbd-remove-one conds key)
  `(kbd-delete-key-binding2 (list ,@conds) ,key)
) ;define

(define (kbd-remove-body conds l)
  (cond ((null? l) '())
        ((symbol? (car l)) (kbd-remove-body (list 0 (car l)) (cdr l)))
        ((and (pair? (car l)) (keyword? (caar l)))
         (kbd-remove-body (kbd-add-condition conds (car l)) (cdr l))
        ) ;
        (else (map (lambda (x) (kbd-remove-one conds x)) l))
  ) ;cond
) ;define

(tm-define-macro (kbd-unmap . l)
  (:synopsis "Remove entries in @l from keyboard mapping")
  `(begin ,@(kbd-remove-body '() l))
) ;tm-define-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Definition of keyboard (backslashed) commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define kbd-command-table (make-ahash-table))

(define (kbd-set-command! key im)
  (ahash-set! kbd-command-table key im)
) ;define

(tm-define (kbd-get-command key)
  (lazy-keyboard-force)
  (ahash-ref kbd-command-table key)
) ;tm-define

(tm-define (kbd-command-pre arg)
  (:synopsis "Helper routine for kbd-commands macro")
  (with (cmd help . action)
    arg
    (list cmd help (list 'unquote `(lambda ,() ,@action)))
  ) ;with
) ;tm-define

(tm-define (kbd-command arg)
  (:synopsis "Helper routine for kbd-commands macro")
  (with (cmd help action) arg (kbd-set-command! cmd (cons help action)))
) ;tm-define

(tm-define-macro (kbd-commands . l)
  (:synopsis "Add backslashed commands in @l to keyboard mapping")
  `(for-each kbd-command ,(list 'quasiquote (map kbd-command-pre l)))
) ;tm-define-macro

(tm-define-macro (kbd-symbols . l)
  (:synopsis "Add symbols in @l to keyboard mapping")
  (define (fun s)
    (list s
      (string-append "insert#<" s ">")
      (list 'kbd-insert (string-append "<" s ">"))
    ) ;list
  ) ;define
  `(kbd-commands ,@(map fun l))
) ;tm-define-macro

(tm-define (emulate-keyboard k) (delayed (raw-emulate-keyboard k)))

;; extract-code-str
;; 从过程对象（Procedure）或列表中提取源码对象（S-Expression）。
;; 这是用于调试快捷键绑定的核心辅助函数，能够“透视”匿名 Lambda 函数的内部逻辑，
;; 并返回可供进一步程序处理的原始代码结构。
;;
;; 语法
;; ----
;; (extract-code-str proc)
;;
;; 参数
;; ----
;; proc : procedure | any
;;    需要提取源码的目标对象。通常是 `kbd-get-map` 返回列表中的条件闭包或命令闭包。
;;    如果是 procedure，尝试获取其源码；如果是普通列表或其他数据，则按原样处理。
;;
;; 返回值
;; ----
;; any (list | symbol)
;; - 返回对象源码的原始 Scheme 结构（S-Expression）。
;; - 如果输入是 `(lambda () (func))`，则返回列表 `(func)`（保留括号结构）。
;; - 如果输入是 `(lambda () (func arg))`，则返回列表 `(func arg)`。
;;
;; 逻辑
;; ----
;; 1. 源码获取：首先检查输入是否为过程，如果是则调用 `procedure-source` 获取源码列表。
;; 2. Lambda 识别：
;;   - 检查源码是否为列表 (pair)。
;;   - 检查第一个元素是否为符号 `lambda`。
;;   - 检查列表长度是否足够包含函数体 (cddr)。
;; 3. 剥离外壳：
;;   - 如果确认为 Lambda 结构，直接提取第三个元素 `(caddr src)` 作为函数体代码。
;;   - 注意：不进行额外的 `car` 拆包，完整保留函数体内的逻辑结构。

(define (extract-code-str proc)
  (let* ((src (if (procedure? proc) (procedure-source proc) proc))
         (code (if (and (pair? src) (eq? (car src) 'lambda) (pair? (cddr src)))
                 (caddr src)
                 src
               ) ;if
         ) ;code
        ) ;
    code
  ) ;let*
) ;define

;; get-kbd-bindings
;; 获取指定按键序列的所有绑定详情，并以结构化数据的形式返回。
;; 这是一个高层调试工具，用于查看某个快捷键在不同上下文（条件）下的行为定义。
;;
;; 语法
;; ----
;; (get-kbd-bindings key-str)
;;
;; 参数
;; ----
;; key-str : string
;;    按键序列字符串，例如 "return"、"C-x C-f" 或 "tab"。
;;
;; 返回值
;; ----
;; list
;; - 如果按键未定义：返回 `(not-bound)`。
;; - 如果按键已定义：返回一个列表的列表，格式为：
;;  `(( (条件代码列表...) 命令代码 "帮助文档" ) ...)`
;;  示例：`(( ((inside-replace-buffer?)) (replace-one ...) "" ) ...)`
;;  注意：这里的条件和命令是 Scheme 代码对象（List），而非字符串。
;;
;; 逻辑
;; ----
;; 1. 获取原始映射：调用 `kbd-get-map` 获取按键对应的原始关联列表 (Association List)。
;; 2. 空值检查：如果 `kbd-get-map` 返回 #f，则返回 `(not-bound)`。
;; 3. 遍历处理：使用 `map` 遍历原始列表中的每一项绑定：
;;   - 条件处理：原始数据的 car 部分是条件闭包列表，对其中每个元素调用 `extract-code-str` 获取源码对象。
;;   - 命令处理：原始数据的 cadr 部分是命令闭包，对其调用 `extract-code-str` 获取源码对象。
;;   - 帮助文档：原始数据的 caddr 部分是字符串，保持原样。
;; 4. 结果组装：将处理后的条件代码列表、命令代码对象和帮助文档重新组装成一个新的列表返回。
;;
;; 注意
;; ----
;; 1. 本函数使用 `tm-define` 定义，使其在 Mogan/TeXmacs 的模块系统中可见。
;; 2. 返回结果中的条件部分始终是一个列表（如 `((cond1) (cond2))`），
;;   因为 Mogan 允许一个快捷键绑定同时依赖多个条件（逻辑与关系）。
(tm-define (get-kbd-bindings key-str)
  (let ((raw-map (kbd-get-map key-str)))
    (if (not raw-map)
      (list 'not-bound)
      (map (lambda (item)
             (let ((conds (car item)) (cmd (cadr item)) (help (caddr item)))
               (list (map extract-code-str conds) (extract-code-str cmd) help)
             ) ;let
           ) ;lambda
        raw-map
      ) ;map
    ) ;if
  ) ;let
) ;tm-define

;; get-bindings-by-command
;; 根据指定的命令代码，反向查找绑定了该命令的所有快捷键及其生效条件。
;;
;; 语法
;; ----
;; (get-bindings-by-command target-cmd)
;;
;; 参数
;; ----
;; target-cmd : list | symbol
;;    目标命令的源码结构。
;;    例如：
;;    - 查找特定调用：`'(search-next-match #t)`
;;    - 查找简单命令：`'(make 'select-region)`
;;
;; 返回值
;; ----
;; list
;; - 返回一个列表，其中每个元素结构为 `(按键字符串 (条件代码列表...))`。
;; - 示例：`(("F3" ((inside-search-or-replace-buffer?))) ("return" (...)))`
;;
;; 逻辑
;; ----
;; 1. 遍历：扫描 `kbd-map-table` 中的每一个按键定义。
;; 2. 提取：对每个绑定的命令闭包调用 `extract-code-str` 获取其源码对象。
;; 3. 比对：使用 `equal?` 将提取出的命令源码与 `target-cmd` 进行深度比对。
;; 4. 收集：如果匹配，将 `(Key Conditions)` 加入结果列表。
(tm-define (get-bindings-by-command target-cmd)
  (let ((all-entries (ahash-table->list kbd-map-table)) (matches '()))
    (for-each (lambda (map-entry)
                (let ((key (car map-entry)) (ctx-list (cdr map-entry)))
                  (for-each (lambda (ctx-item)
                              (let* ((cmd-proc (cadr ctx-item))
                                     ;; 核心步骤：提取命令的源码进行比对
                                     (cmd-code (extract-code-str cmd-proc))
                                    ) ;
                                (when (equal? cmd-code target-cmd)
                                  (let ((cond-codes (map extract-code-str (car ctx-item))))
                                    ;; 收集结果：(按键 (条件...))
                                    (set! matches (cons (list key cond-codes) matches))
                                  ) ;let
                                ) ;when
                              ) ;let*
                            ) ;lambda
                    ctx-list
                  ) ;for-each
                ) ;let
              ) ;lambda
      all-entries
    ) ;for-each
    (reverse matches)
  ) ;let
) ;tm-define



;; get-bindings-by-condition
;; 根据指定的条件列表，反向查找所有匹配的快捷键及其对应的命令代码。
;; 此函数用于回答“在某个特定环境（如表格中、数学模式中）下，定义了哪些快捷键？”
;;
;; 语法
;; ----
;; (get-bindings-by-condition target-conds)
;;
;; 参数
;; ----
;; target-conds : list
;;    目标条件源码列表。这是一个由 Scheme 代码对象（S-expression）组成的列表。
;;    例如：
;;    - 查找无条件绑定：`()`
;;    - 查找数学模式绑定：`'((and (== (get-env "mode") "text") (not (in-graphics?))))` 或 `(list '(and (== (get-env "mode") "text") (not (in-graphics?))))`
;;    - 查找特定条件：`'((inside-replace-buffer?))`
;;
;; 返回值
;; ----
;; list
;; - 返回一个列表，其中每个元素也是一个列表，结构为 `(按键字符串 命令代码对象)`。
;; - 示例：`(("return" (replace-one ...)) ("C-2" (insert ...)))`
;;
;; 逻辑
;; ----
;; 1. 获取全集：调用 `ahash-table->list` 将 `kbd-map-table` 哈希表转换为遍历列表。
;; 2. 双层遍历：
;;   - 外层遍历每一个按键定义（Key Entry）。
;;   - 内层遍历该按键下的每一个上下文重载（Context Entry）。
;; 3. 源码比对：
;;   - 使用 `extract-code-str` 提取当前上下文中的条件源码。
;;   - 使用 `equal?` 将提取出的条件与参数 `target-conds` 进行深度比对。
;; 4. 收集结果：
;;   - 如果匹配成功，提取对应的命令源码，并将 `(Key Command)` 组合加入结果列表。
(tm-define (get-bindings-by-condition target-conds)
  (let ((all-entries (ahash-table->list kbd-map-table)) (matches '()))
    (for-each (lambda (map-entry)
                (let ((key (car map-entry)) (ctx-list (cdr map-entry)))
                  ;; 遍历该按键的所有重载定义
                  (for-each (lambda (ctx-item)
                              (let* ((cond-funcs (car ctx-item))
                                     ;; 提取源码用于比对
                                     (cond-codes (map extract-code-str cond-funcs))
                                    ) ;
                                ;; 深度比对条件结构
                                (when (equal? cond-codes target-conds)
                                  (let ((cmd-code (extract-code-str (cadr ctx-item))))
                                    ;; 收集结果：(按键 命令)
                                    (set! matches (cons (list key cmd-code) matches))
                                  ) ;let
                                ) ;when
                              ) ;let*
                            ) ;lambda
                    ctx-list
                  ) ;for-each
                ) ;let
              ) ;lambda
      all-entries
    ) ;for-each
    ;; 返回结果（反转以保持发现顺序，虽不强求）
    (reverse matches)
  ) ;let
) ;tm-define

;; kbd-conflict-query
;; 冲突查询函数
;; 检查在新按键序列（new-key）下，是否存在与给定条件（conds）完全一致的绑定。
;;
;; 参数
;; ----
;; conds   : list
;;    条件源码列表（S-Expression），例如 '((and (== (get-env "mode") "text") (not (in-graphics?)))) 或 '()。
;; new-key : string
;;    待检查的新按键序列，例如 "C-a"。
;;
;; 返回值
;; ----
;; any | #f
;; - 如果存在冲突：返回冲突绑定的命令源码（通常是一个列表）。
;; - 如果无冲突：返回 #f。
(tm-define (kbd-conflict-query conds new-key)
  (let ((raw-map (kbd-get-map new-key)))
    (if raw-map
      (let loop
        ((entries raw-map))
        (if (null? entries)
          #f
          (let* ((entry (car entries))
                 (entry-conds (car entry))
                 ;; 提取现有绑定的条件源码进行比对
                 (entry-conds-src (map extract-code-str entry-conds))
                ) ;
            ;; 检查条件是否完全一致（精确匹配上下文）
            (if (equal? entry-conds-src conds)
              ;; 发现冲突，返回占用该位置的命令源码
              (extract-code-str (cadr entry))
              ;; 继续检查下一个重载
              (loop (cdr entries))
            ) ;if
          ) ;let*
        ) ;if
      ) ;let
      #f
    ) ;if
  ) ;let
) ;tm-define

;; kbd-execute-edit
;; 编辑执行函数
;; 根据给定的条件，删除旧按键绑定（如果存在），并创建新按键绑定。
;;
;; 参数
;; ----
;; conds   : list
;;    条件源码列表。
;; cmd     : any
;;    命令源码（S-Expression），例如 '(insert "alpha")。
;; old-key : string
;;    旧按键序列。如果非空，函数将尝试移除该按键下匹配 conds 的绑定。
;; new-key : string
;;    新按键序列。如果非空，函数将在该按键下创建指向 cmd 的新绑定。
;;
;; 逻辑 (四种状态)
;; --------------
;; 1. 清理旧绑定 (Old Key)：
;;   若 old-key 非空，遍历其映射表，寻找条件源码与 conds 完全匹配的条目。
;;   找到后，利用原始闭包对象将其从哈希表中移除。
;;
;; 2. 应用新绑定 (New Key)：
;;   若 new-key 非空，将 conds 和 cmd 动态编译为闭包，插入到 new-key 的映射表中。
;;   **若 new-key 为空，则跳过此步。**
;;
;; 四种行为
;; --------
;; - 新增: old-key="", new-key="C-a" -> 旧的无动作，新增 C-a 绑定。
;; - 修改: old-key="C-b", new-key="C-a"  -> 将绑定从 C-b 搬到 C-a。
;; - 删除: old-key="C-b",  new-key="" -> C-b 被解绑，且没有新绑定生成。
;; - 无效操作：都为空，直接返回。
(tm-define (kbd-execute-edit conds cmd old-key new-key)
  ;; 1. 如果提供了旧按键，则尝试删除旧绑定
  (when (and (string? old-key) (> (string-length old-key) 0))
    (let ((raw-map (kbd-get-map old-key)))
      (when raw-map
        (for-each (lambda (entry)
                    (let* ((entry-conds (car entry))
                           (entry-conds-src (map extract-code-str entry-conds))
                           (entry-cmd-src (extract-code-str (cadr entry)))
                          ) ;

                      ;; 只有当 (条件匹配) 且 (命令也匹配) 时，才认为是同一个绑定进行删除
                      ;; 这样可以防止在交换快捷键时，误删刚刚绑定上去的新命令
                      (when (and (equal? entry-conds-src conds) (equal? entry-cmd-src cmd))
                        ;; 使用原始的条件闭包列表进行删除
                        (kbd-delete-key-binding2 entry-conds old-key)
                      ) ;when
                    ) ;let*
                  ) ;lambda
          raw-map
        ) ;for-each
      ) ;when
    ) ;let
  ) ;when

  ;; 2. 如果提供了新按键，则插入新绑定
  (when (and (string? new-key) (> (string-length new-key) 0))
    ;; 将数据层面的源码 (S-Expression) 转换为可执行的闭包 (Procedure)
    (let ((cond-funcs (map (lambda (c) (eval `(lambda ,() ,c) (current-module))) conds))
          (cmd-func (eval `(lambda ,() ,cmd) (current-module)))
         ) ;
      ;; 插入新绑定，帮助文档暂留空字符串
      (kbd-insert-key-binding cond-funcs new-key (list cmd-func ""))
    ) ;let
  ) ;when
) ;tm-define
