
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : python-lang.scm
;; DESCRIPTION : Python Language
;; COPYRIGHT   : (C) 2014-2020  François Poulain, Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code python-lang) (:use (prog default-lang)))

;; 定义解析器特性函数，用于获取Python语言的关键字定义
;; 当语言为"python"且键为"keyword"时返回关键字分类
(tm-define (parser-feature lan key)
  (:require (and (== lan "python") (== key "keyword")))
  `(,(string->symbol key)
    (extra_chars "_")
    ;; 额外字符，关键字中允许包含下划线
    (constant ;; 常量关键字
      "Ellipsis"
      "False"
      "None"
      "NotImplemented"
      "True"
      "__debug__"
      "__import__"
      "abs"
      "all"
      "any"
      "apply"
      "ascii"
      "basestring"
      "bin"
      "bool"
      "buffer"
      "__main__"
      "bytearray"
      "bytes"
      "callable"
      "chr"
      "classmethod"
      "cmp"
      "coerce"
      "compile"
      "complex"
      "delattr"
      "dict"
      "dir"
      "divmod"
      "enumerate"
      "eval"
      "execfile"
      "file"
      "filter"
      "float"
      "format"
      "frozenset"
      "getattr"
      "globals"
      "hasattr"
      "hash"
      "help"
      "hex"
      "id"
      "input"
      "int"
      "intern"
      "isinstance"
      "issubclass"
      "iter"
      "len"
      "list"
      "locals"
      "long"
      "map"
      "max"
      "memoryview"
      "min"
      "next"
      "nonlocal"
      "object"
      "oct"
      "open"
      "ord"
      "pow"
      "property"
      "range"
      "raw_input"
      "reduce"
      "reload"
      "repr"
      "reversed"
      "round"
      "set"
      "setattr"
      "slice"
      "sorted"
      "staticmethod"
      "str"
      "sum"
      "super"
      "tuple"
      "type"
      "unichr"
      "unicode"
      "vars"
      "xrange"
      "zip"
      "BaseException"
      "Exception"
      "ArithmeticError"
      "EnvironmentError"
      "LookupError"
      "StandardError"
      "AssertionError"
      "AttributeError"
      "BufferError"
      "EOFError"
      "FloatingPointError"
      "GeneratorExit"
      "IOError"
      "ImportError"
      "IndentationError"
      "IndexError"
      "KeyError"
      "KeyboardInterrupt"
      "MemoryError"
      "NameError"
      "NotImplementedError"
      "OSError"
      "OverflowError"
      "ReferenceError"
      "RuntimeError"
      "StopIteration"
      "SyntaxError"
      "SystemError"
      "SystemExit"
      "TabError"
      "TypeError"
      "UnboundLocalError"
      "UnicodeError"
      "UnicodeDecodeError"
      "UnicodeEncodeError"
      "UnicodeTranslateError"
      "ValueError"
      "VMSError"
      "WindowsError"
      "ZeroDivisionError"
      "BytesWarning"
      "DeprecationWarning"
      "FutureWarning"
      "ImportWarning"
      "PendingDeprecationWarning"
      "RuntimeWarning"
      "SyntaxWarning"
      "UnicodeWarning"
      "UserWarning"
      "Warning")
    (declare_function "def" "lambda")
    ;; 函数声明关键字
    (declare_module "import")
    ;; 模块声明关键字
    (declare_type "class")
    ;; 类型声明关键字
    (keyword ;; 一般关键字
      "and"
      "not"
      "or"
      "as"
      "del"
      "from"
      "global"
      "in"
      "is"
      "with")
    (keyword_conditional ;; 条件语句关键字
      "break"
      "continue"
      "elif"
      "else"
      "for"
      "if"
      "while")
    (keyword_control ;; 控制流关键字
      "assert"
      "except"
      "exec"
      "finally"
      "pass"
      "print"
      "raise"
      "return"
      "try"
      "yield"))
) ;tm-define

;; 定义解析器特性函数，用于获取Python语言的运算符定义
;; 当语言为"python"且键为"operator"时返回运算符分类
(tm-define (parser-feature lan key)
  (:require (and (== lan "python") (== key "operator")))
  `(,(string->symbol key)
    (operator ;; 基本运算符
      "+"
      "-"
      "/"
      "*"
      "**"
      "//"
      "%"
      "|"
      "&"
      "^"
      "<<"
      ">>"
      "=="
      "!="
      "<>"
      "<"
      ">"
      "<="
      ">="
      "="
      "+="
      "-="
      "/="
      "*="
      "%="
      "|="
      "&="
      "^="
      "**="
      "//="
      "<<="
      ">>="
      "~")
    (operator_special ":")
    ;; 特殊运算符
    (operator_decoration "@")
    ;; 装饰器运算符
    (operator_field ".")
    ;; 字段访问运算符
    (operator_openclose "{" "[" "(" ")" "]" "}"))
) ;tm-define
;; 开闭运算符（括号）

;; 定义Python数字后缀函数，用于处理虚数单位

(define (python-number-suffix)
  '(suffix (imaginary "j" "J"))
) ;define
;; 虚数单位后缀

;; https://docs.python.org/3.8/reference/lexical_analysis.html#numeric-literals
;; 定义解析器特性函数，用于获取Python语言的数字字面量定义
;; 当语言为"python"且键为"number"时返回数字格式定义
(tm-define (parser-feature lan key)
  (:require (and (== lan "python") (== key "number")))
  `(,(string->symbol key)
    (bool_features ;; 数字特性
      "prefix_0x"
      "prefix_0b"
      "prefix_0o"
      "no_suffix_with_box"
      "sci_notation")
    ;; 支持十六进制、二进制、八进制前缀和科学计数法
    ,(python-number-suffix)
    ;; 包含数字后缀定义
    (separator "_"))
) ;tm-define
;; 数字分隔符

;; 定义解析器特性函数，用于获取Python语言的字符串定义
;; 当语言为"python"且键为"string"时返回字符串特性
(tm-define (parser-feature lan key)
  (:require (and (== lan "python") (== key "string")))
  `(,(string->symbol key)
    (bool_features "hex_with_8_bits"
      "hex_with_16_bits"
      ;; 支持8位和16位十六进制
      "hex_with_32_bits"
      "octal_upto_3_digits")
    ;; 支持32位十六进制和最多3位八进制
    (escape_sequences "\\" "\"" "'" "a" "b" "f" "n" "r" "t" "v" "newline"))
) ;tm-define
;; 转义序列

;; 定义解析器特性函数，用于获取Python语言的注释定义
;; 当语言为"python"且键为"comment"时返回注释格式
(tm-define (parser-feature lan key)
  (:require (and (== lan "python") (== key "comment")))
  `(,(string->symbol key) (inline "#"))
) ;tm-define
;; 行内注释，以#开头

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences for syntax highlighting
;; 语法高亮偏好设置
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 通知Python语法更改的函数

(define (notify-python-syntax var val)
  (syntax-read-preferences "python")
) ;define

;; 定义语法高亮颜色偏好设置
(define-preferences ("syntax:python:none" "#ff6b6b" notify-python-syntax)
  ;; 浅红色 (light red)
  ("syntax:python:comment" "#d4a373" notify-python-syntax)
  ;; 浅棕色 (light brown)
  ("syntax:python:error" "#ff4d4d" notify-python-syntax)
  ;; 亮红色 (bright red)
  ("syntax:python:constant" "#6a8cff" notify-python-syntax)
  ;; 浅蓝色 (light blue)
  ("syntax:python:constant_number" "#6a8cff" notify-python-syntax)
  ;; 浅蓝色 (light blue)
  ("syntax:python:constant_string" "#a9a9a9" notify-python-syntax)
  ;; 浅灰色 (light grey)
  ("syntax:python:constant_char" "#b0b0b0" notify-python-syntax)
  ;; 浅灰色 (light grey)
  ("syntax:python:declare_function" "#4d7eff" notify-python-syntax)
  ;; 浅蓝色 (light blue)
  ("syntax:python:declare_type" "#4d7eff" notify-python-syntax)
  ;; 浅蓝色 (light blue)
  ("syntax:python:declare_module" "#4d7eff" notify-python-syntax)
  ;; 浅蓝色 (light blue)
  ("syntax:python:operator" "#e066ff" notify-python-syntax)
  ;; 浅洋红色 (light magenta)
  ("syntax:python:operator_openclose" "#ff6666" notify-python-syntax)
  ;; 浅红色 (light red)
  ("syntax:python:operator_field" "#d1d1d1" notify-python-syntax)
  ;; 浅灰色 (light grey)
  ("syntax:python:operator_special" "#ffb347" notify-python-syntax)
  ;; 浅橙色 (light orange)
  ("syntax:python:keyword" "#4db8b8" notify-python-syntax)
  ;; 浅青色 (light teal/cyan)
  ("syntax:python:keyword_conditional" "#4db8b8" notify-python-syntax)
  ;; 浅青色 (light teal/cyan)
  ("syntax:python:keyword_control" "#4db8b8" notify-python-syntax)
) ;define-preferences
