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

(texmacs-module (code python-lang)
  (:use (prog default-lang)))

;; 定义解析器特性函数，用于获取Python语言的关键字定义
;; 当语言为"python"且键为"keyword"时返回关键字分类
(tm-define (parser-feature lan key)
  (:require (and (== lan "python") (== key "keyword")))
  `(,(string->symbol key)
    (extra_chars "_")  ;; 额外字符，关键字中允许包含下划线
    (constant          ;; 常量关键字
      "Ellipsis" "False" "None" "NotImplemented" "True" "__debug__" "__import__" "abs"
      "all" "any" "apply" "ascii" "basestring" "bin" "bool" "buffer" "__main__"
      "bytearray" "bytes" "callable" "chr" "classmethod" "cmp" "coerce" "compile"
      "complex" "delattr" "dict" "dir" "divmod" "enumerate" "eval" "execfile"
      "file" "filter" "float" "format" "frozenset" "getattr" "globals" "hasattr"
      "hash" "help" "hex" "id" "input" "int" "intern" "isinstance"
      "issubclass" "iter" "len" "list" "locals" "long" "map" "max"
      "memoryview" "min" "next" "nonlocal" "object" "oct" "open" "ord"
      "pow" "property" "range" "raw_input" "reduce" "reload" "repr" "reversed"
      "round" "set" "setattr" "slice" "sorted" "staticmethod" "str" "sum"
      "super" "tuple" "type" "unichr" "unicode" "vars" "xrange" "zip"
      "BaseException" "Exception" "ArithmeticError" "EnvironmentError" "LookupError"
      "StandardError" "AssertionError" "AttributeError" "BufferError" "EOFError"
      "FloatingPointError" "GeneratorExit" "IOError" "ImportError" "IndentationError"
      "IndexError" "KeyError" "KeyboardInterrupt" "MemoryError" "NameError"
      "NotImplementedError" "OSError" "OverflowError" "ReferenceError" "RuntimeError"
      "StopIteration" "SyntaxError" "SystemError" "SystemExit" "TabError"
      "TypeError" "UnboundLocalError" "UnicodeError" "UnicodeDecodeError" "UnicodeEncodeError"
      "UnicodeTranslateError" "ValueError" "VMSError" "WindowsError" "ZeroDivisionError"
      "BytesWarning" "DeprecationWarning" "FutureWarning" "ImportWarning" "PendingDeprecationWarning"
      "RuntimeWarning" "SyntaxWarning" "UnicodeWarning" "UserWarning" "Warning")
    (declare_function "def" "lambda")     ;; 函数声明关键字
    (declare_module "import")             ;; 模块声明关键字
    (declare_type "class")                ;; 类型声明关键字
    (keyword                              ;; 一般关键字
      "and" "not" "or" "as" "del" "from" "global" "in" "is" "with")
    (keyword_conditional                  ;; 条件语句关键字
      "break" "continue" "elif" "else" "for" "if" "while")
    (keyword_control                      ;; 控制流关键字
      "assert" "except" "exec" "finally" "pass" "print" "raise" "return"
      "try" "yield")))

;; 定义解析器特性函数，用于获取Python语言的运算符定义
;; 当语言为"python"且键为"operator"时返回运算符分类
(tm-define (parser-feature lan key)
  (:require (and (== lan "python") (== key "operator")))
  `(,(string->symbol key)
    (operator         ;; 基本运算符
      "+" "-" "/" "*" "**" "//" "%" "|" "&" "^"
      "<<" ">>" "==" "!=" "<>" "<" ">" "<=" ">="
      "=" "+=" "-=" "/=" "*=" "%=" "|=" "&=" "^="
      "**=" "//=" "<<=" ">>="
      "~")
   (operator_special ":")        ;; 特殊运算符
   (operator_decoration "@")     ;; 装饰器运算符
   (operator_field ".")          ;; 字段访问运算符
   (operator_openclose "{" "[" "(" ")" "]" "}")))  ;; 开闭运算符（括号）

;; 定义Python数字后缀函数，用于处理虚数单位
(define (python-number-suffix)
  `(suffix
    (imaginary "j" "J")))  ;; 虚数单位后缀

;; https://docs.python.org/3.8/reference/lexical_analysis.html#numeric-literals
;; 定义解析器特性函数，用于获取Python语言的数字字面量定义
;; 当语言为"python"且键为"number"时返回数字格式定义
(tm-define (parser-feature lan key)
  (:require (and (== lan "python") (== key "number")))
  `(,(string->symbol key)
   (bool_features     ;; 数字特性
     "prefix_0x" "prefix_0b" "prefix_0o" "no_suffix_with_box"
     "sci_notation")            ;; 支持十六进制、二进制、八进制前缀和科学计数法
   ,(python-number-suffix)      ;; 包含数字后缀定义
   (separator "_")))            ;; 数字分隔符

;; 定义解析器特性函数，用于获取Python语言的字符串定义
;; 当语言为"python"且键为"string"时返回字符串特性
(tm-define (parser-feature lan key)
  (:require (and (== lan "python") (== key "string")))
  `(,(string->symbol key)
    (bool_features 
     "hex_with_8_bits" "hex_with_16_bits"        ;; 支持8位和16位十六进制
     "hex_with_32_bits" "octal_upto_3_digits")   ;; 支持32位十六进制和最多3位八进制
    (escape_sequences "\\" "\"" "'" "a" "b" "f" "n" "r" "t" "v" "newline")))  ;; 转义序列

;; 定义解析器特性函数，用于获取Python语言的注释定义
;; 当语言为"python"且键为"comment"时返回注释格式
(tm-define (parser-feature lan key)
  (:require (and (== lan "python") (== key "comment")))
  `(,(string->symbol key)
    (inline "#")))  ;; 行内注释，以#开头

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences for syntax highlighting
;; 语法高亮偏好设置
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (notify-python-pref var val)
  (syntax-read-preferences "python"))

(define-preferences
  ("syntax:python:none" "syntax-python-none" notify-python-pref)
  ("syntax:python:comment" "syntax-python-comment" notify-python-pref)
  ("syntax:python:error" "syntax-python-error" notify-python-pref)
  ("syntax:python:constant" "syntax-python-constant" notify-python-pref)
  ("syntax:python:constant_number" "syntax-python-constant-number" notify-python-pref)
  ("syntax:python:constant_string" "syntax-python-constant-string" notify-python-pref)
  ("syntax:python:constant_char" "syntax-python-constant-char" notify-python-pref)
  ("syntax:python:declare_function" "syntax-python-declare-function" notify-python-pref)
  ("syntax:python:declare_type" "syntax-python-declare-type" notify-python-pref)
  ("syntax:python:declare_module" "syntax-python-declare-module" notify-python-pref)
  ("syntax:python:operator" "syntax-python-operator" notify-python-pref)
  ("syntax:python:operator_openclose" "syntax-python-operator-openclose" notify-python-pref)
  ("syntax:python:operator_field" "syntax-python-operator-field" notify-python-pref)
  ("syntax:python:operator_special" "syntax-python-operator-special" notify-python-pref)
  ("syntax:python:keyword" "syntax-python-keyword" notify-python-pref)
  ("syntax:python:keyword_conditional" "syntax-python-keyword-conditional" notify-python-pref)
  ("syntax:python:keyword_control" "syntax-python-keyword-control" notify-python-pref))
