;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : matlab-lang.scm
;; DESCRIPTION : Matlab language support
;; COPYRIGHT   : (C) 2025  veista
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; DESCRIPTION:
;;;   This module provides language support for MATLAB within TeXmacs. It 
;;;   defines language features such as keywords, operators, number formats,
;;;   string formats, and comment formats for proper syntax highlighting and
;;;   parsing of MATLAB code.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 定义matlab语言支持模块，使用默认语言支持模块
(texmacs-module (code matlab-lang)
  (:use (prog default-lang)))

;;------------------------------------------------------------------------------
;; 关键字定义
;;

;; 定义MATLAB语言的关键字分类
;; 包括常量、函数声明、类型声明、模块导入、标识符声明、条件关键字、控制关键字等
;; src/System/Language/language.cpp中定义了关键字分类
(tm-define (parser-feature lan key)
  (:require (and (== lan "matlab") (== key "keyword")))
  `(,(string->symbol key)
    (constant
      "Inf" "NaN" "eps" "pi" "i" "j" "true" "false" "ans" "realmin" "realmax")
    (declare_function
      "function")
    (declare_type
      "classdef" "properties" "methods" "events" "enumerate" "struct" "attributes")
    (declare_module
      "import")
    (declare_identifier
      "global" "persistent")
    (keyword
      "nargin" "nargout" "varargin" "varargout" "parsing")
    (keyword_conditional
      "if" "else" "elseif" "switch" "case" "otherwise")
    (keyword_control
      "for" "while" "break" "continue" "return" "try" "catch" "end")))

;;------------------------------------------------------------------------------
;; 操作符定义
;;

;; 定义MATLAB语言的操作符符号
(tm-define (parser-feature lan key)
  (:require (and (== lan "matlab") (== key "operator")))
  `(,(string->symbol key)
    (operator
      ;; 算术运算符
      "+" "-" "*" "/" "\\" "^" ".^" ".*" "./" ".\\" "'" ".'"
      ;; 关系运算符
      "==" "~=" "<" ">" "<=" ">=" 
      ;; 逻辑运算符
      "&" "|" "~" "&&" "||"
      ;; 其他运算符
      ":" "(" ")" "[" "]" "{" "}" ";" "," "." ".." "..." "=" "!" "@")))

;;------------------------------------------------------------------------------
;; 数字格式定义
;;

;; 定义MATLAB语言的数字格式，包括前缀和后缀
;; 支持16进制(0x)和二进制(0b)前缀，科学计数法
;; 支持无符号整数、有符号整数和浮点数后缀
(tm-define (parser-feature lan key)
  (:require (and (== lan "matlab") (== key "number")))
  `(,(string->symbol key)
    (bool_features
      "prefix_0x" "prefix_0b"
      "sci_notation")
    (suffix
      (unsigned_integer 
        "u8" "u16" "u32" "u64")
      (signed_integer
        "s8" "s16" "s32" "s64")
      (float "f" "F"))))

;;------------------------------------------------------------------------------
;; 字符串格式定义
;;

;; 定义MATLAB语言的字符串格式，支持转义字符
(tm-define (parser-feature lan key)
  (:require (and (== lan "matlab") (== key "string")))
  `(,(string->symbol key)
    (bool_features
      "escape_char_after_backslash")
    (escape_sequences "\\" "\"" "n" "t" "b" "r" "f" "a" "v" "0")
    (double_escape "'")
    (start_disallow_after
      "alpha" "digit" "underscore" ")" "]" "}" "." "'")))

;;------------------------------------------------------------------------------
;; 注释格式定义
;;

;; 定义MATLAB语言的注释格式，支持行注释和块注释
(tm-define (parser-feature lan key)
  (:require (and (== lan "matlab") (== key "comment")))
  `(,(string->symbol key)
    (inline "%")
    (block_comment "%{" "%}")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences for syntax highlighting
;; 语法高亮偏好设置
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 通知MATLAB语法更改的函数
(define (notify-matlab-syntax var val)
  (syntax-read-preferences "matlab"))

;; 定义MATLAB语法高亮颜色偏好设置
;; 已验证可生效
(define-preferences
  ("syntax:matlab:none" "red" notify-matlab-syntax)
  ("syntax:matlab:comment" "brown" notify-matlab-syntax)
  ("syntax:matlab:error" "dark red" notify-matlab-syntax)
  ("syntax:matlab:constant" "#4040c0" notify-matlab-syntax)
  ("syntax:matlab:constant_number" "#3030b0" notify-matlab-syntax)
  ("syntax:matlab:constant_string" "dark grey" notify-matlab-syntax)
  ("syntax:matlab:constant_char" "#333333" notify-matlab-syntax)
  ("syntax:matlab:declare_function" "#0000c0" notify-matlab-syntax)
  ("syntax:matlab:declare_type" "#0000c0" notify-matlab-syntax)
  ("syntax:matlab:declare_module" "#0000c0" notify-matlab-syntax)
  ("syntax:matlab:operator" "#8b008b" notify-matlab-syntax)
  ("syntax:matlab:operator_openclose" "#B02020" notify-matlab-syntax)
  ("syntax:matlab:operator_field" "#888888" notify-matlab-syntax)
  ("syntax:matlab:operator_special" "orange" notify-matlab-syntax)
  ("syntax:matlab:keyword" "#309090" notify-matlab-syntax)
  ("syntax:matlab:keyword_conditional" "#309090" notify-matlab-syntax)
  ("syntax:matlab:keyword_control" "#008080ff" notify-matlab-syntax))
