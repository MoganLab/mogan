;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : r-lang.scm
;; DESCRIPTION : R Language
;; COPYRIGHT   : (C) 2026 Hongli Zha
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code r-lang)
  (:use (prog default-lang)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Parser Features
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; R关键字、函数、常量等
(tm-define (parser-feature lan key)
  (:require (and (== lan "r") (== key "keyword")))
  `(,(string->symbol key)
    (extra_chars "_") 
  ;; 变量/关键字可包含下划线
    (constant ;; 常量
      "NULL" "NA" "NaN" "Inf" "TRUE" "FALSE" "T" "F")
    (declare_function ;; 内置函数
      "abs" "sign" "sqrt" "ceiling" "floor" "trunc" "round"
      "exp" "log" "log10" "log2" "cos" "sin" "tan" "acos" "asin" "atan"
      "cosh" "sinh" "tanh" "gamma" "lgamma" "max" "min" "sum" "prod" "mean" "median" "var" "sd"
      "range" "cumsum" "cumprod" "cummax" "cummin" "c" "seq" "rep" "length" "unique" "sort" "order" "rank"
      "rev" "which" "any" "all" "is.na" "is.nan"
      "paste" "paste0" "substr" "nchar" "sprintf" "grep" "grepl" "sub" "gsub" "strsplit" "toupper" "tolower" "trimws"
      "list" "data.frame" "matrix" "array" "factor"
      "function" "args" "body" "environment" "formals" "quote" "eval" "do.call" "match.call" "get" "assign" "exists"
      "print" "cat" "read.table" "read.csv" "write.table" "write.csv" "scan" "readLines" "writeLines" "source"
      "detach" "search" "ls"
      "subset" "transform" "with" "within"
      "system" "Sys.time" "Sys.Date" "version" "setwd" "getwd"
      "sample" "set.seed" "duplicated")
    (declare_type ;; 类型
      "numeric" "integer" "logical" "character")
    (declare_module ;; 包/模块
      "library" "require" "attach" "detach")
    (keyword ;; 基本关键字
      "if" "else" "for" "while" "repeat" "break" "next" "in" "return" "...")
    (keyword_control ;; 控制/异常关键字
      "stop" "warning" "message" "switch" "try" "tryCatch")))

;; 运算符
(tm-define (parser-feature lan key)
  (:require (and (== lan "r") (== key "operator")))
  `(,(string->symbol key)
    (operator
      "+" "-" "*" "/" "^" ":" "%%" "%/%" "<" ">" "<=" ">=" "==" "!=" "&" "&&" "|" "||" "!" "~" "$" "@")
    (operator_special "::" ":::" "|>")
    (operator_openclose "(" "[" "{" ")" "]" "}")
    (operator_field "$" "@" "<-" "=" "<<-" "->" "->>")))

;; 数字特性
(tm-define (parser-feature lan key)
  (:require (and (== lan "r") (== key "number")))
  `(,(string->symbol key)
    (bool_features "decimal" "sci_notation" "prefix_0x") ; 支持 0x 十六进制
    (separator "_")
    (suffix
      (integer "L" "l")  ; 整数后缀
      (imag "i"))))       ; 复数后缀

;; 字符串特性
(tm-define (parser-feature lan key)
  (:require (and (== lan "r") (== key "string")))
  `(,(string->symbol key)
      (bool_features "single_quote" "double_quote" "multi_byte" "unicode_escape" "hex_escape")
      (escape_sequences "\\" "\"" "'" "a" "b" "f" "n" "r" "t" "v" "newline" "x" "u" "U")))

;; 注释格式
(tm-define (parser-feature lan key)
  (:require (and (== lan "r") (== key "comment")))
  `(,(string->symbol key)
    (inline "#")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences for syntax highlighting
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (notify-r-syntax var val)
  (syntax-read-preferences "r"))

(define-preferences
  ("syntax:r:none" "red" notify-r-syntax)
  ("syntax:r:comment" "brown" notify-r-syntax)
  ("syntax:r:error" "dark red" notify-r-syntax)
  ("syntax:r:constant" "#4040c0" notify-r-syntax)
  ("syntax:r:constant_number" "#4040c0" notify-r-syntax)
  ("syntax:r:constant_string" "dark grey" notify-r-syntax)
  ("syntax:r:constant_char" "#333333" notify-r-syntax)
  ("syntax:r:declare_function" "#0000c0" notify-r-syntax)
  ("syntax:r:declare_type" "#0000c0" notify-r-syntax)
  ("syntax:r:operator" "#8b008b" notify-r-syntax)
  ("syntax:r:operator_openclose" "#B02020" notify-r-syntax)
  ("syntax:r:operator_field" "#B02020" notify-r-syntax)
  ("syntax:r:operator_special" "orange" notify-r-syntax)
  ("syntax:r:keyword" "#309090" notify-r-syntax)
  ("syntax:r:keyword_control" "#309090" notify-r-syntax))
