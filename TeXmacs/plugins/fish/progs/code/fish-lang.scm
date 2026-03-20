;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : fish-lang.scm
;; DESCRIPTION : fish language support
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
;;;   This module provides language support for fish within TeXmacs. It
;;;   defines language features such as keywords, operators, number formats,
;;;   string formats, and comment formats for proper syntax highlighting and
;;;   parsing of fish code.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 定义fish语言支持模块，使用默认语言支持模块
(texmacs-module (code fish-lang)
  (:use (prog default-lang)))

;;------------------------------------------------------------------------------
;; 关键字定义
;;

;; 定义fish语言的关键字分类
(tm-define (parser-feature lan key)
  (:require (and (== lan "fish") (== key "keyword")))
  `(,(string->symbol key)
    (constant
      "true" "false" "null")
    (declare_function
      "define" "struct" "structure")
    (declare_identifier
      "local" "global")
    (keyword_conditional
      "if" "else if" "else" "endif"
      "caseof" "case" "endcase")
    (keyword_control
      "loop" "endloop" "exit loop" "continue"
      "section" "endsection" "exit section"
      "command" "endcommand"
      "return" "exit" "end" "lock")))

;;------------------------------------------------------------------------------
;; 操作符定义
;;

;; 定义fish语言的操作符符号
(tm-define (parser-feature lan key)
  (:require (and (== lan "fish") (== key "operator")))
  `(,(string->symbol key)
    (operator
      ;; 算术运算符
      "+" "-" "*" "/" "^"
      ;; 关系运算符
      "==" "!=" "#" "<" ">" "<=" ">="
      ;; 逻辑运算符
      "and" "or" "not"
      ;; 其他运算符
      "=" "(" ")" "[" "]" "{" "}" ";" "," "." "@")))

;;------------------------------------------------------------------------------
;; 数字格式定义
;;

;; 定义fish语言的数字格式，支持科学计数法
(tm-define (parser-feature lan key)
  (:require (and (== lan "fish") (== key "number")))
  `(,(string->symbol key)
    (bool_features
      "prefix_0x"
      "sci_notation")))

;;------------------------------------------------------------------------------
;; 字符串格式定义
;;

;; 定义fish语言的字符串格式，支持转义字符
(tm-define (parser-feature lan key)
  (:require (and (== lan "fish") (== key "string")))
  `(,(string->symbol key)
    (bool_features
      "escape_char_after_backslash")
    (escape_sequences "\\" "\"" "n" "t" "b" "r" "f" "a" "v" "0")
    (double_escape "'")))

;;------------------------------------------------------------------------------
;; 注释格式定义
;;

;; 定义fish语言的注释格式，使用 ; 作为注释
(tm-define (parser-feature lan key)
  (:require (and (== lan "fish") (== key "comment")))
  `(,(string->symbol key)
    (inline ";")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences for syntax highlighting
;; 语法高亮偏好设置
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 通知fish语法更改的函数
(define (notify-fish-syntax var val)
  (syntax-read-preferences "fish"))

;; 定义fish语法高亮颜色偏好设置
;; 已验证可生效
(define-preferences
  ("syntax:fish:none" "red" notify-fish-syntax)
  ("syntax:fish:comment" "brown" notify-fish-syntax)
  ("syntax:fish:error" "dark red" notify-fish-syntax)
  ("syntax:fish:constant" "#4040c0" notify-fish-syntax)
  ("syntax:fish:constant_number" "#3030b0" notify-fish-syntax)
  ("syntax:fish:constant_string" "dark grey" notify-fish-syntax)
  ("syntax:fish:constant_char" "#333333" notify-fish-syntax)
  ("syntax:fish:declare_function" "#0000c0" notify-fish-syntax)
  ("syntax:fish:declare_type" "#0000c0" notify-fish-syntax)
  ("syntax:fish:declare_module" "#0000c0" notify-fish-syntax)
  ("syntax:fish:operator" "#8b008b" notify-fish-syntax)
  ("syntax:fish:operator_openclose" "#B02020" notify-fish-syntax)
  ("syntax:fish:operator_field" "#888888" notify-fish-syntax)
  ("syntax:fish:operator_special" "orange" notify-fish-syntax)
  ("syntax:fish:keyword" "#309090" notify-fish-syntax)
  ("syntax:fish:keyword_conditional" "#309090" notify-fish-syntax)
  ("syntax:fish:keyword_control" "#008080ff" notify-fish-syntax))
