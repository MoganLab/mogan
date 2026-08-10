;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : lua-lang.scm
;; DESCRIPTION : Lua Language Support for Syntax Highlighting
;; COPYRIGHT   : (C) 2025  Fanjie Meng
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code lua-lang)
  (:use (prog default-lang)))

(tm-define (parser-feature lan key)
  (:require (and (== lan "lua") (== key "keyword")))
  `(,(string->symbol key)
    (extra_chars "_")
    (constant
      "nil" "true" "false" "_G" "_VERSION" "arg" "self")
    (declare_function "function")
    (keyword
      "and" "break" "do" "else" "elseif" "end" "for" "if" "in"
      "local" "not" "or" "repeat" "return" "then" "until" "while")
    (keyword_control
      "break" "return" "do" "end" "while" "repeat" "until" "for")
    (keyword_conditional
      "if" "then" "else" "elseif")))

(tm-define (parser-feature lan key)
  (:require (and (== lan "lua") (== key "operator")))
  `(,(string->symbol key)
    (operator
      "+" "-" "*" "/" "%" "^" "==" "~=" "<=" ">=" "<" ">"
      "=" ".." "#")
    (operator_special ":")
    (operator_field ".")
    (operator_openclose "(" ")" "{" "}" "[" "]")))

(tm-define (parser-feature lan key)
  (:require (and (== lan "lua") (== key "number")))
  `(,(string->symbol key)
    (bool_features
      "prefix_0x" "sci_notation")
    (suffix
      (imaginary "i"))))

(tm-define (parser-feature lan key)
  (:require (and (== lan "lua") (== key "string")))
  `(,(string->symbol key)
    (bool_features
      "hex_with_8_bits" "hex_with_16_bits")
    (escape_sequences "\\" "\"" "'" "a" "b" "f" "n" "r" "t" "v")))

(tm-define (parser-feature lan key)
  (:require (and (== lan "lua") (== key "comment")))
  `(,(string->symbol key)
    (inline "--")
    (block "--[[" "]]")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences for syntax highlighting
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (notify-lua-syntax var val)
  (syntax-read-preferences "lua"))

(define-preferences
  ("syntax:lua:none" "black" notify-lua-syntax)
  ("syntax:lua:comment" "gray" notify-lua-syntax)
  ("syntax:lua:error" "dark red" notify-lua-syntax)
  ("syntax:lua:constant" "blue" notify-lua-syntax)
  ("syntax:lua:constant_number" "purple" notify-lua-syntax)
  ("syntax:lua:constant_string" "brown" notify-lua-syntax)
  ("syntax:lua:declare_function" "dark blue" notify-lua-syntax)
  ("syntax:lua:operator" "magenta" notify-lua-syntax)
  ("syntax:lua:operator_openclose" "red" notify-lua-syntax)
  ("syntax:lua:operator_field" "gray" notify-lua-syntax)
  ("syntax:lua:keyword" "dark green" notify-lua-syntax)
  ("syntax:lua:keyword_conditional" "dark green" notify-lua-syntax)
  ("syntax:lua:keyword_control" "dark green" notify-lua-syntax))