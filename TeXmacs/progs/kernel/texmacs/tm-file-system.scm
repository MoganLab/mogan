
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-file-system.scm
;; DESCRIPTION : The TeXmacs file system
;; COPYRIGHT   : (C) 2006  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel texmacs tm-file-system))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Lazy handlers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public lazy-tmfs-table (make-ahash-table))

;; lazy-tmfs-handler
;; 注册延迟加载的 tmfs 处理器。
;; 当访问指定类别的 tmfs URL 时，自动加载对应的模块。
;;
;; 语法
;; ----
;; (lazy-tmfs-handler module . classes)
;;
;; 参数
;; ----
;; module : symbol
;; 需要延迟加载的模块名称。
;;
;; classes : list
;; 一个或多个 tmfs 类别名称（符号）。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
;;
;; 注意
;; ----
;; 这是一个宏，会在编译时展开为对 lazy-tmfs-table 的设置操作。
(define-public-macro (lazy-tmfs-handler module . classes)
  `(for-each (lambda (class) (ahash-set! lazy-tmfs-table class (quote ,module)))
     (quote ,classes))
) ;define-public-macro

;; lazy-tmfs-force
;; 强制加载指定类别的延迟 tmfs 处理器模块。
;;
;; 语法
;; ----
;; (lazy-tmfs-force class)
;;
;; 参数
;; ----
;; class : string or symbol
;; 需要强制加载的 tmfs 类别名称。
;;
;; 返回值
;; ----
;; boolean
;; 若成功加载模块返回 #t，否则返回 #f。
;;
;; 注意
;; ----
;; 加载完成后会从 lazy-tmfs-table 中移除对应条目，避免重复加载。
(define-public (lazy-tmfs-force class)
  (if (string? class) (set! class (string->symbol class)))
  (and-with module
    (ahash-ref lazy-tmfs-table class)
    (ahash-remove! lazy-tmfs-table class)
    (eval `(use-modules ,module))
  ) ;and-with
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Handler system
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public tmfs-handler-table (make-ahash-table))

;; object->tmstring
;; 将 Scheme 对象序列化为字符串。
;; S7 对打印序列长度有上限限制，此函数临时取消该限制。
;;
;; 语法
;; ----
;; (object->tmstring s)
;;
;; 参数
;; ----
;; s : any
;; 需要序列化的 Scheme 对象。
;;
;; 返回值
;; ----
;; string
;; 序列化后的字符串表示。
;;
;; 注意
;; ----
;; FIXME: 需要寻找更优雅的实现方式来绕过 S7 的 print-length 限制。
(define-public (object->tmstring s)
  ;; S7 impose an upper bound on the lenght of sequences to be printed
  ;; we override it...
  ;; FIXME: do we have a more elegant way to do it??
  (let-temporarily (((*s7* 'print-length) 9223372036854775807))
    (unescape-guile (object->string s))
  ) ;let-temporarily
) ;define-public

;; tmstring->object
;; 将字符串反序列化为 Scheme 对象。
;;
;; 语法
;; ----
;; (tmstring->object s)
;;
;; 参数
;; ----
;; s : string
;; 需要反序列化的字符串。
;;
;; 返回值
;; ----
;; any
;; 反序列化后的 Scheme 对象。

(define (tmstring->object s)
  (string->object s)
) ;define

;; tmfs-handler
;; 注册指定类别和动作的 tmfs 处理器函数。
;;
;; 语法
;; ----
;; (tmfs-handler class action handle)
;;
;; 参数
;; ----
;; class : symbol or boolean
;; tmfs 类别名称，#t 表示默认处理器。
;;
;; action : symbol
;; 动作类型，如 'load、'save、'remove 等。
;;
;; handle : procedure
;; 处理该类别和动作的函数。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
(define-public (tmfs-handler class action handle)
  ;; (display* "Define " class " :: " action "\n")
  (ahash-set! tmfs-handler-table (cons class action) handle)
) ;define-public

;; tmfs-decompose-name
;; 将 tmfs URL 分解为类别和名称两部分。
;;
;; 语法
;; ----
;; (tmfs-decompose-name name)
;;
;; 参数
;; ----
;; name : string or url
;; 需要分解的 tmfs URL，支持字符串或 url 类型。
;;
;; 返回值
;; ----
;; list
;; 包含两个元素的列表 (class name)，class 为类别字符串，name 为剩余路径。
;; 若无法分解，class 默认为 "file"。
;;
;; 示例
;; ----
;; (tmfs-decompose-name "tmfs://aux/test") => ("aux" "test")
;; (tmfs-decompose-name "hello/world") => ("hello" "world")
;; (tmfs-decompose-name "plain") => ("file" "plain")
(define-public (tmfs-decompose-name name)
  (if (not (string? name)) (set! name (url->string name)))
  (if (string-starts? name "tmfs://") (set! name (string-drop name 7)))
  (with i
    (string-index name #\/)
    (list (if i (substring name 0 i) "file")
      (if i (substring name (+ i 1) (string-length name)) name)
    ) ;list
  ) ;with
) ;define-public

;; tmfs-load
;; 从 TeXmacs 文件系统加载指定 URL 的内容。
;;
;; 语法
;; ----
;; (tmfs-load u)
;;
;; 参数
;; ----
;; u : string or url
;; 需要加载的 tmfs URL。
;;
;; 返回值
;; ----
;; string
;; 加载的内容字符串；若处理器返回字符串则直接使用，否则通过 object->tmstring 转换。
;; 无对应处理器时返回空字符串。
(define-public (tmfs-load u)
  "Load url @u on TeXmacs file system."
  (with (class name)
    (tmfs-decompose-name u)
    (lazy-tmfs-force class)
    (cond ((ahash-ref tmfs-handler-table (cons class 'load))
           =>
           (lambda (handler)
             (with r (handler name) (if (string? r) r (object->tmstring r)))
           ) ;lambda
          ) ;
          ((ahash-ref tmfs-handler-table (cons #t 'load))
           =>
           (lambda (handler)
             (with r (handler name) (if (string? r) r (object->tmstring r)))
           ) ;lambda
          ) ;
          (else "")
    ) ;cond
  ) ;with
) ;define-public

;; tmfs-save
;; 将内容保存到 TeXmacs 文件系统的指定 URL。
;;
;; 语法
;; ----
;; (tmfs-save u what)
;;
;; 参数
;; ----
;; u : string or url
;; 目标 tmfs URL。
;;
;; what : string
;; 需要保存的内容字符串。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
(define-public (tmfs-save u what)
  "Save string @what to url @u on TeXmacs file system."
  (with (class name)
    (tmfs-decompose-name u)
    (lazy-tmfs-force class)
    (cond ((ahash-ref tmfs-handler-table (cons class 'save))
           =>
           (lambda (handler) (handler name (tmstring->object what)))
          ) ;
          (else ((ahash-ref tmfs-handler-table (cons #t 'save)) u what))
    ) ;cond
  ) ;with
) ;define-public

;; tmfs-autosave
;; 获取指定 URL 的自动保存名称。
;;
;; 语法
;; ----
;; (tmfs-autosave u suf)
;;
;; 参数
;; ----
;; u : string or url
;; 需要自动保存的 tmfs URL。
;;
;; suf : string
;; 自动保存文件的后缀名。
;;
;; 返回值
;; ----
;; string or #f
;; 自动保存的 URL 字符串；若不支持自动保存则返回 #f。
(define-public (tmfs-autosave u suf)
  "Autosave name for url @u with suffix @suf on TeXmacs file system, or @#f."
  (with (class name)
    (tmfs-decompose-name u)
    (lazy-tmfs-force class)
    (cond ((ahash-ref tmfs-handler-table (cons class 'autosave))
           =>
           (lambda (handler) (handler name suf))
          ) ;
          (else ((ahash-ref tmfs-handler-table (cons #t 'autosave)) u suf))
    ) ;cond
  ) ;with
) ;define-public

;; tmfs-can-autosave?
;; 检查指定 URL 是否支持自动保存。
;;
;; 语法
;; ----
;; (tmfs-can-autosave? u)
;;
;; 参数
;; ----
;; u : string or url
;; 需要检查的 tmfs URL。
;;
;; 返回值
;; ----
;; boolean
;; 若支持自动保存返回 #t，否则返回 #f。
(define-public (tmfs-can-autosave? u) (not (not (tmfs-autosave u "~"))))

;; tmfs-remove
;; 从 TeXmacs 文件系统中移除指定 URL。
;;
;; 语法
;; ----
;; (tmfs-remove u)
;;
;; 参数
;; ----
;; u : string or url
;; 需要移除的 tmfs URL。
;;
;; 返回值
;; ----
;; boolean
;; 移除成功返回 #t，否则返回 #f。
(define-public (tmfs-remove u)
  "Remove url @u from TeXmacs file system and return @#t on success."
  (with (class name)
    (tmfs-decompose-name u)
    (lazy-tmfs-force class)
    (cond ((ahash-ref tmfs-handler-table (cons class 'remove))
           =>
           (lambda (handler) (handler name))
          ) ;
          (else ((ahash-ref tmfs-handler-table (cons #t 'remove)) u))
    ) ;cond
  ) ;with
) ;define-public

;; tmfs-wrap
;; 获取指定 tmfs URL 对应的基础包装 URL。
;;
;; 语法
;; ----
;; (tmfs-wrap u)
;;
;; 参数
;; ----
;; u : string or url
;; 需要查询的 tmfs URL。
;;
;; 返回值
;; ----
;; url or #f
;; 包装后的基础 URL；若不存在则返回 #f。
(define-public (tmfs-wrap u)
  "Underlying wrapped url for url @u on TeXmacs file system, or @#f."
  (with (class name)
    (tmfs-decompose-name u)
    (lazy-tmfs-force class)
    (cond ((ahash-ref tmfs-handler-table (cons class 'wrap))
           =>
           (lambda (handler) (handler name))
          ) ;
          (else ((ahash-ref tmfs-handler-table (cons #t 'wrap)) u))
    ) ;cond
  ) ;with
) ;define-public

;; tmfs-date
;; 获取指定 tmfs URL 的最后修改日期。
;;
;; 语法
;; ----
;; (tmfs-date u)
;;
;; 参数
;; ----
;; u : string or url
;; 需要查询的 tmfs URL。
;;
;; 返回值
;; ----
;; string or #f
;; 最后修改日期字符串；若无法获取则返回 #f。
(define-public (tmfs-date u)
  "Get last modification date for url @u on TeXmacs file system, or @#f."
  (with (class name)
    (tmfs-decompose-name u)
    (lazy-tmfs-force class)
    (cond ((ahash-ref tmfs-handler-table (cons class 'date))
           =>
           (lambda (handler) (handler name))
          ) ;
          (else ((ahash-ref tmfs-handler-table (cons #t 'date)) u))
    ) ;cond
  ) ;with
) ;define-public

;; tmfs-title
;; 获取指定 tmfs URL 的友好标题。
;;
;; 语法
;; ----
;; (tmfs-title u doc)
;;
;; 参数
;; ----
;; u : string or url
;; 需要获取标题的 tmfs URL。
;;
;; doc : tree
;; 关联的文档树。
;;
;; 返回值
;; ----
;; string
;; 标题字符串。
(define-public (tmfs-title u doc)
  "Get a nice title for url @u on TeXmacs file system."
  (with (class name)
    (tmfs-decompose-name u)
    (lazy-tmfs-force class)
    (cond ((ahash-ref tmfs-handler-table (cons class 'title))
           =>
           (lambda (handler) (handler name doc))
          ) ;
          ((ahash-ref tmfs-handler-table (cons class 'load))
           (if (url? u) (url->system u) u)
          ) ;
          (else ((ahash-ref tmfs-handler-table (cons #t 'title)) u doc))
    ) ;cond
  ) ;with
) ;define-public

;; tmfs-permission?
;; 检查对指定 tmfs URL 是否具有给定类型的权限。
;;
;; 语法
;; ----
;; (tmfs-permission? u type)
;;
;; 参数
;; ----
;; u : string or url
;; 需要检查权限的 tmfs URL。
;;
;; type : string
;; 权限类型，如 "read"、"write"。
;;
;; 返回值
;; ----
;; boolean
;; 若有权限返回 #t，否则返回 #f。
;;
;; 注意
;; ----
;; 自动排除以 ~ 或 # 结尾的备份文件（若不支持自动保存）。
;; 若存在 load 处理器但未注册 permission? 处理器，默认允许 "read" 权限。
(define-public (tmfs-permission? u type)
  "Check whether we have the permission of a given @type for the url @u."
  (with (class name)
    (tmfs-decompose-name u)
    (lazy-tmfs-force class)
    (cond ((and (string-ends? (url->unix u) "~")
             (not (tmfs-autosave (url-unglue u 1) "~"))
           ) ;and
           #f
          ) ;
          ((and (string-ends? (url->unix u) "#")
             (not (tmfs-autosave (url-unglue u 1) "#"))
           ) ;and
           #f
          ) ;
          ((ahash-ref tmfs-handler-table (cons class 'permission?))
           =>
           (lambda (handler) (handler name type))
          ) ;
          ((tmfs-wrap u) ((ahash-ref tmfs-handler-table (cons #t 'permission?)) u type))
          ((ahash-ref tmfs-handler-table (cons class 'load)) (== type "read"))
          (else ((ahash-ref tmfs-handler-table (cons #t 'permission?)) u type))
    ) ;cond
  ) ;with
) ;define-public

;; tmfs-master
;; 获取用于链接和导航的主 URL。
;;
;; 语法
;; ----
;; (tmfs-master u)
;;
;; 参数
;; ----
;; u : string or url
;; 需要获取主 URL 的 tmfs URL。
;;
;; 返回值
;; ----
;; string or url
;; 主 URL；若无对应处理器则返回原 URL。
(define-public (tmfs-master u)
  "Get a master url @u for linking and navigation."
  (with (class name)
    (tmfs-decompose-name u)
    (lazy-tmfs-force class)
    (cond ((ahash-ref tmfs-handler-table (cons class 'master))
           =>
           (lambda (handler) (handler name))
          ) ;
          (else u)
    ) ;cond
  ) ;with
) ;define-public

;; tmfs-format
;; 获取指定 tmfs URL 的文件格式。
;;
;; 语法
;; ----
;; (tmfs-format u)
;;
;; 参数
;; ----
;; u : string or url
;; 需要获取格式的 tmfs URL。
;;
;; 返回值
;; ----
;; string
;; 文件格式字符串；默认返回 "stm"。
(define-public (tmfs-format u)
  "Get file format for url @u."
  (with (class name)
    (tmfs-decompose-name u)
    (lazy-tmfs-force class)
    (cond ((ahash-ref tmfs-handler-table (cons class 'format))
           =>
           (lambda (handler) (handler name))
          ) ;
          (else "stm")
    ) ;cond
  ) ;with
) ;define-public

;; tmfs-remote?
;; 检查指定 URL 是否由远程处理器处理。
;;
;; 语法
;; ----
;; (tmfs-remote? u)
;;
;; 参数
;; ----
;; u : string or url
;; 需要检查的 tmfs URL。
;;
;; 返回值
;; ----
;; boolean
;; 若该类别没有本地 'load 处理器（即由远程处理）返回 #t，否则返回 #f。
(define-public (tmfs-remote? u)
  "Check whether the url @u is handled remotedly."
  (with (class name)
    (tmfs-decompose-name u)
    (lazy-tmfs-force class)
    (not (ahash-ref tmfs-handler-table (cons class 'load)))
  ) ;with
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Routines for making and decomposing queries
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; escape-entry
;; 对查询参数值进行转义，将冒号替换为 %3A。

(define (escape-entry s)
  (if (string? s) (string-replace s ":" "%3A") "")
) ;define

;; unescape-entry
;; 对查询参数值进行反转义，将 %3A 还原为冒号。

(define (unescape-entry s)
  (if (string? s) (string-replace s "%3A" ":") "")
) ;define

;; pair->entry
;; 将键值对转换为查询字符串中的 "key=value" 格式。
;;
;; 语法
;; ----
;; (pair->entry p)
;;
;; 参数
;; ----
;; p : pair
;; 键值对 (key . value)，key 和 value 均为字符串。
;;
;; 返回值
;; ----
;; string
;; 转义后的 "key=value" 字符串。

(define (pair->entry p)
  (string-append (escape-entry (car p)) "=" (escape-entry (cdr p)))
) ;define

;; list->query
;; 将键值对列表转换为查询字符串。
;;
;; 语法
;; ----
;; (list->query l)
;;
;; 参数
;; ----
;; l : list
;; 键值对列表，每个元素为 (key . value) 形式的 pair。
;;
;; 返回值
;; ----
;; string
;; 以 "&" 连接各键值对的查询字符串，如 "a=1&b=2"。
;;
;; 示例
;; ----
;; (list->query '(("a" . "1") ("b" . "2"))) => "a=1&b=2"
(define-public (list->query l)
  (with r (map pair->entry l) (string-recompose r "&"))
) ;define-public

;; entry->pair
;; 将查询字符串中的单个 "key=value" 条目解析为键值对。

(define (entry->pair e)
  (with l
    (string-tokenize-by-char-n e #\= 1)
    (cond ((== (length l) 0) (cons "" ""))
          ((== (length l) 1) (cons (unescape-entry (car l)) ""))
          (else (cons (unescape-entry (car l)) (unescape-entry (cadr l))))
    ) ;cond
  ) ;with
) ;define

;; query->list
;; 将查询字符串解析为键值对列表。
;;
;; 语法
;; ----
;; (query->list q)
;;
;; 参数
;; ----
;; q : string
;; 以 "&" 分隔的查询字符串。
;;
;; 返回值
;; ----
;; list
;; 键值对列表，每个元素为 (key . value) 形式的 pair。
;;
;; 示例
;; ----
;; (query->list "a=1&b=2") => (("a" . "1") ("b" . "2"))
(define-public (query->list q)
  (with l (string-tokenize-by-char q #\&) (map entry->pair l))
) ;define-public

;; query-ref
;; 从查询字符串中获取指定变量的值。
;;
;; 语法
;; ----
;; (query-ref q var)
;;
;; 参数
;; ----
;; q : string
;; 查询字符串。
;;
;; var : string
;; 需要查询的变量名。
;;
;; 返回值
;; ----
;; string
;; 变量的值；若变量不存在则返回空字符串。
;;
;; 示例
;; ----
;; (query-ref "a=1&b=2" "a") => "1"
;; (query-ref "a=1&b=2" "c") => ""
(define-public (query-ref q var)
  (tmstring->string (or (assoc-ref (query->list q) var) ""))
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines for building and analyzing TMFS URLs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; tmfs-pair?
;; 检查字符串是否包含路径分隔符 "/"。
;;
;; 语法
;; ----
;; (tmfs-pair? s)
;;
;; 参数
;; ----
;; s : string
;; 需要检查的字符串。
;;
;; 返回值
;; ----
;; number or #f
;; 若包含 "/" 返回其索引位置，否则返回 #f。
(define-public (tmfs-pair? s) (string-index s #\/))

;; tmfs-car
;; 获取 tmfs 路径的第一个组件（"/" 之前的部分）。
;;
;; 语法
;; ----
;; (tmfs-car s)
;;
;; 参数
;; ----
;; s : string
;; tmfs 路径字符串。
;;
;; 返回值
;; ----
;; string or #f
;; 第一个路径组件；若不存在 "/" 则返回 #f。
;;
;; 示例
;; ----
;; (tmfs-car "foo/bar") => "foo"
(define-public (tmfs-car s)
  (with i (string-index s #\/) (and i (substring s 0 i)))
) ;define-public

;; tmfs-cdr
;; 获取 tmfs 路径的剩余部分（"/" 之后的部分）。
;;
;; 语法
;; ----
;; (tmfs-cdr s)
;;
;; 参数
;; ----
;; s : string
;; tmfs 路径字符串。
;;
;; 返回值
;; ----
;; string or #f
;; "/" 之后的路径；若不存在 "/" 则返回 #f。
;;
;; 示例
;; ----
;; (tmfs-cdr "foo/bar") => "bar"
(define-public (tmfs-cdr s)
  (with i (string-index s #\/) (and i (substring s (+ i 1) (string-length s))))
) ;define-public

;; tmfs->list
;; 将 tmfs 路径字符串递归分解为组件列表。
;;
;; 语法
;; ----
;; (tmfs->list s)
;;
;; 参数
;; ----
;; s : string
;; tmfs 路径字符串。
;;
;; 返回值
;; ----
;; list
;; 路径组件列表，如 ("foo" "bar" "baz")。
;;
;; 示例
;; ----
;; (tmfs->list "foo/bar/baz") => ("foo" "bar" "baz")
;; (tmfs->list "plain") => ("plain")
(define-public (tmfs->list s)
  (if (not (tmfs-pair? s)) (list s) (cons (tmfs-car s) (tmfs->list (tmfs-cdr s))))
) ;define-public

;; list->tmfs
;; 将路径组件列表组合为 tmfs 路径字符串。
;;
;; 语法
;; ----
;; (list->tmfs l)
;;
;; 参数
;; ----
;; l : list
;; 路径组件列表。
;;
;; 返回值
;; ----
;; string
;; 以 "/" 连接的 tmfs 路径字符串。
;;
;; 示例
;; ----
;; (list->tmfs '("foo" "bar")) => "foo/bar"
(define-public (list->tmfs l) (apply string-append (list-intersperse l "/")))

;; strip-colon
;; 去除 Windows 盘符路径中的冒号，将 "C:/" 转换为 "C//"。
;;
;; 语法
;; ----
;; (strip-colon s)
;;
;; 参数
;; ----
;; s : string
;; Windows 路径字符串。
;;
;; 返回值
;; ----
;; string
;; 处理后的路径字符串。
;;
;; 注意
;; ----
;; 仅当字符串以单个字母后跟 ":/" 开头时才会处理。
(define-public (strip-colon s)
  (if (and (>= (string-length s) 3)
        (string-alpha? (substring s 0 1))
        (== (substring s 1 3) ":/")
      ) ;and
    (string-append (substring s 0 1) (substring s 2 (string-length s)))
    s
  ) ;if
) ;define-public

;; url->tmfs-string
;; 将 TeXmacs url 转换为 tmfs 字符串表示。
;;
;; 语法
;; ----
;; (url->tmfs-string u)
;;
;; 参数
;; ----
;; u : url
;; 需要转换的 TeXmacs url。
;;
;; 返回值
;; ----
;; string
;; tmfs 格式的字符串，如 "tm/..."、"here/..."、"file/..." 或 "protocol/..."。
;;
;; 注意
;; ----
;; 若 URL 位于 TeXmacs 安装路径下，则使用相对路径表示为 "tm/..."。
(define-public (url->tmfs-string u)
  (if (and (url-descends? u (get-texmacs-path)) (!= (url->url u) (get-texmacs-path)))
    (with base
      (url-append (get-texmacs-path) "x")
      (string-append "tm/" (url->unix (url-delta base u)))
    ) ;with
    (let* ((protocol (url-root u)) (file (url->unix (url-unroot u))))
      (cond ((== protocol "") (string-append "here/" file))
            ((== protocol "default")
             (if (os-mingw?)
               (string-append "file/" (strip-colon file))
               (string-append "file/" file)
             ) ;if
            ) ;
            (else (string-append protocol "/" file))
      ) ;cond
    ) ;let*
  ) ;if
) ;define-public

;; tmfs-string->url
;; 将 tmfs 字符串表示转换为 TeXmacs url。
;;
;; 语法
;; ----
;; (tmfs-string->url s)
;;
;; 参数
;; ----
;; s : string
;; tmfs 格式的字符串，如 "tm/..."、"here/..."、"file/..."。
;;
;; 返回值
;; ----
;; url
;; 转换后的 TeXmacs url。
;;
;; 注意
;; ----
;; "tm" 协议映射到 TeXmacs 安装路径，"here" 映射为相对路径。
;; 支持 "http"、"https"、"ftp"、"tmfs" 等特殊协议。
(define-public (tmfs-string->url s)
  (if (not (tmfs-pair? s))
    (unix->url s)
    (let* ((protocol (tmfs-car s)) (file (unix->url (tmfs-cdr s))))
      (cond ((== protocol "tm") (url-append (get-texmacs-path) file))
            ((== protocol "here") file)
            ((== protocol "file")
             (if (os-mingw?)
               (string->url (string-append "/" (tmfs-cdr s)))
               (url-append (root->url "default") file)
             ) ;if
            ) ;
            ((in? protocol '("http" "https" "ftp" "tmfs"))
             (url-append (root->url protocol) file)
            ) ;
            (else (url-append (root->url "default") s))
      ) ;cond
    ) ;let*
  ) ;if
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Macros for defining handlers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public-macro (tmfs-load-handler head . body)
  (with (type what)
    head
    `(tmfs-handler ,(symbol->string type) 'load (lambda (,what) ,@body))
  ) ;with
) ;define-public-macro

(define-public-macro (tmfs-save-handler head . body)
  (with (type what doc)
    head
    `(tmfs-handler ,(symbol->string type) 'save (lambda (,what ,doc) ,@body))
  ) ;with
) ;define-public-macro

(define-public-macro (tmfs-autosave-handler head . body)
  (with (type what suf)
    head
    `(tmfs-handler ,(symbol->string type)
       'autosave
       (lambda (,what ,suf) ,@body))
  ) ;with
) ;define-public-macro

(define-public-macro (tmfs-remove-handler head . body)
  (with (type what)
    head
    `(tmfs-handler ,(symbol->string type) 'remove (lambda (,what) ,@body))
  ) ;with
) ;define-public-macro

(define-public-macro (tmfs-wrap-handler head . body)
  (with (type what)
    head
    `(tmfs-handler ,(symbol->string type) 'wrap (lambda (,what) ,@body))
  ) ;with
) ;define-public-macro

(define-public-macro (tmfs-date-handler head . body)
  (with (type what)
    head
    `(tmfs-handler ,(symbol->string type) 'date (lambda (,what) ,@body))
  ) ;with
) ;define-public-macro

(define-public-macro (tmfs-title-handler head . body)
  (with (type what doc)
    head
    `(tmfs-handler ,(symbol->string type) 'title (lambda (,what ,doc) ,@body))
  ) ;with
) ;define-public-macro

(define-public-macro (tmfs-permission-handler head . body)
  (with (type what kind)
    head
    `(tmfs-handler ,(symbol->string type)
       'permission?
       (lambda (,what ,kind) ,@body))
  ) ;with
) ;define-public-macro

(define-public-macro (tmfs-master-handler head . body)
  (with (type what)
    head
    `(tmfs-handler ,(symbol->string type) 'master (lambda (,what) ,@body))
  ) ;with
) ;define-public-macro

(define-public-macro (tmfs-format-handler head . body)
  (with (type what)
    head
    `(tmfs-handler ,(symbol->string type) 'format (lambda (,what) ,@body))
  ) ;with
) ;define-public-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Simple example
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; id 类别的加载处理器示例。
;; 将名称包装为简单的 TeXmacs 文档树返回。
(tmfs-load-handler (id what)
  `(document (TeXmacs ,(texmacs-version))
     (style (tuple "generic"))
     (body (document ,what)))
) ;tmfs-load-handler

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Default handlers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 默认加载处理器。
;; 当没有匹配的类别处理器时，返回无效的 tmfs 文档提示。
(tmfs-handler #t
  'load
  (lambda (name)
    `(document (TeXmacs ,(texmacs-version))
       (style (tuple "generic"))
       (body (document "Invalid tmfs document.")))
  ) ;lambda
) ;tmfs-handler

;; 默认保存处理器。
;; 不做任何操作。
(tmfs-handler #t 'save (lambda (name doc) (noop)))

;; 默认自动保存处理器。
;; 若名称存在对应的包装 URL，则返回自动保存路径。
(tmfs-handler #t
  'autosave
  (lambda (name suf)
    (and-with u (tmfs-wrap name) (and (url-autosave u suf) (url-glue name suf)))
  ) ;lambda
) ;tmfs-handler

;; 默认删除处理器。
;; 若名称存在对应的包装 URL，则删除该 URL。
(tmfs-handler #t
  'remove
  (lambda (name) (and-with u (tmfs-wrap name) (url-remove u)))
) ;tmfs-handler

;; 默认包装处理器。
;; 返回 #f，表示没有包装 URL。
(tmfs-handler #t 'wrap (lambda (name) #f))

;; 默认日期处理器。
;; 若名称存在对应的包装 URL，则返回该 URL 的最后修改时间。
(tmfs-handler #t
  'date
  (lambda (name) (and-with u (tmfs-wrap name) (url-last-modified u)))
) ;tmfs-handler

;; 默认标题处理器。
;; 直接返回名称作为标题。
(tmfs-handler #t 'title (lambda (name doc) name))

;; 默认权限处理器。
;; 根据包装 URL 的文件系统权限判断读写权限。
(tmfs-handler #t
  'permission?
  (lambda (name kind)
    (with u
      (tmfs-wrap name)
      (cond ((not u) (== kind "read"))
            ((== kind "read") (url-test? u "r"))
            ((== kind "write") (url-test? u "w"))
            (else #f)
      ) ;cond
    ) ;with
  ) ;lambda
) ;tmfs-handler

;; 默认主 URL 处理器。
;; 直接返回原名称。
(tmfs-handler #t 'master (lambda (name) name))

;; 默认格式处理器。
;; 返回默认格式 "stm"。
(tmfs-handler #t 'format (lambda (name) "stm"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Auxiliary buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public aux-buffers (make-ahash-table))
(define-public aux-masters (make-ahash-table))

;; aux 类别的加载处理器。
;; 从 aux-buffers 表中获取缓冲区内容，若不存在则返回空文档。
(tmfs-load-handler (aux name)
  (or (ahash-ref aux-buffers name)
    `(document (TeXmacs ,(texmacs-version))
       (style (tuple "generic"))
       (body (document "")))
  ) ;or
) ;tmfs-load-handler

;; aux 类别的标题处理器。
;; 直接返回名称作为标题。
(tmfs-title-handler (aux name doc) name)

;; aux 类别的主 URL 处理器。
;; 返回 aux 对应的 tmfs URL，若已设置主文档则返回该文档。
(tmfs-master-handler (aux name)
  (or (ahash-ref aux-masters name) (unix->url (string-append "tmfs://aux/" name)))
) ;tmfs-master-handler

;; aux-name
;; 构造 aux 缓冲区的 tmfs URL。
;;
;; 语法
;; ----
;; (aux-name aux)
;;
;; 参数
;; ----
;; aux : string
;; 辅助缓冲区名称。
;;
;; 返回值
;; ----
;; url
;; 对应的 tmfs URL，如 "tmfs://aux/name"。
(define-public (aux-name aux) (unix->url (string-append "tmfs://aux/" aux)))

;; aux-set-document
;; 设置辅助缓冲区的文档内容。
;;
;; 语法
;; ----
;; (aux-set-document aux doc)
;;
;; 参数
;; ----
;; aux : string
;; 辅助缓冲区名称。
;;
;; doc : tree
;; 需要设置的文档树。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
(define-public (aux-set-document aux doc)
  (with name
    (aux-name aux)
    (buffer-set name doc)
    (ahash-set! aux-buffers aux (buffer-get name))
  ) ;with
) ;define-public

;; aux-set-master
;; 设置辅助缓冲区的主文档。
;;
;; 语法
;; ----
;; (aux-set-master aux master)
;;
;; 参数
;; ----
;; aux : string
;; 辅助缓冲区名称。
;;
;; master : url
;; 主文档 URL。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
(define-public (aux-set-master aux master)
  (with name
    (aux-name aux)
    (buffer-set-master name master)
    (ahash-set! aux-masters aux (buffer-get-master name))
  ) ;with
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Importation of files using a different format
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; tmfs-document
;; 将导入的树转换为标准的 TeXmacs 文档格式。
;;
;; 语法
;; ----
;; (tmfs-document t)
;;
;; 参数
;; ----
;; t : tree
;; 需要转换的树。
;;
;; 返回值
;; ----
;; tree
;; 转换后的标准文档树，确保包含 (TeXmacs version) 头。
(define-public (tmfs-document t)
  (with doc
    (tm->stree t)
    (if (and (tm-func? doc 'document) (not (tm-func? (tm-ref doc 0) 'TeXmacs)))
      `(document (TeXmacs ,(texmacs-version)) ,@(cdr doc))
      doc
    ) ;if
  ) ;with
) ;define-public

;; import 类别的加载处理器。
;; 根据格式和路径导入外部文件并转换为 TeXmacs 文档。
(tmfs-load-handler (import name)
  (if (and (tmfs-pair? name) (tmfs-pair? (tmfs-cdr name)))
    (let* ((fm (tmfs-car name)) (u (tmfs-string->url (tmfs-cdr name))))
      (tmfs-document (tree-import u fm))
    ) ;let*
    `(document (TeXmacs ,(texmacs-version))
       (style (tuple "generic"))
       (body (document "")))
  ) ;if
) ;tmfs-load-handler

;; import 类别的标题处理器。
;; 生成 "filename - FORMAT" 格式的标题。
(tmfs-title-handler (import name doc)
  (if (and (tmfs-pair? name) (tmfs-pair? (tmfs-cdr name)))
    (let* ((fm (tmfs-car name))
           (u (tmfs-string->url (tmfs-cdr name)))
           (last (url->system (url-tail u)))
          ) ;
      (string-append last " - " (upcase-first fm))
    ) ;let*
    (url-tail name)
  ) ;if
) ;tmfs-title-handler
