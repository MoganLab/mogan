
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : moonbit-lang.scm
;; DESCRIPTION : Moonbit Language
;; COPYRIGHT   : (C) 2025 (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code moonbit-lang)
  (:use (prog default-lang)))

;; https://docs.moonbitlang.cn/language/introduction.html
(tm-define (parser-feature lan key)
  (:require (and (== lan "moonbit") (== key "keyword")))
  `(,(string->symbol key)
    ;; Keywords based on the official MoonBit documentation.
    (constant
      "true" "false" "_")

    ;; Built-in Types and Type Definition Keywords
    (declare_type
      ;; Keywords for defining new types
      "type" "struct" "enum" "trait" "typealias" "traitalias"

      ;; Built-in primitive types
      "Bool" "Byte" "Char" "String"
      "Int" "Int64" "UInt" "UInt64" "BigInt"
      "Float" "Double" "Float64"
      "Unit"

      ;; Core functional types
      "Option" "Result"

      ;; Collection types
      "Array" "List" "Map" "Set" "StringBuilder"

      ;; Concurrency and state
      "Ref")

    ;; Declarations and Functions
    (declare_function "fn" "let" "const")

    ;; Control Flow & Expressions
    (keyword_conditional
      "if" "else" "while" "loop" "for" "in" "match")

    ;; Keywords for program structure and visibility
    (declare_module "import" "pub")

    ;; Other keywords from the official list
    (keyword
      "as" "extern" "fnalias" "typealias" "traitalias" "mut"
      "derive" "break" "continue" "return" "throw" "raise"
      "try" "catch" "with" "guard" "async" "is" "suberror" "and" "letrec"
      "enumview" "noraise" "defer" "test" "impl")))

(tm-define (parser-feature lan key)
  (:require (and (== lan "moonbit") (== key "operator")))
  `(,(string->symbol key)
    ;; Common operators in MoonBit
    (operator
      "+" "-" "/" "*" "%" ;; Arithmetic
      "|" "&" "^" "!"      ;; Bitwise and Logical NOT
      "&&" "||"            ;; Boolean
      "==" "!=" "<" ">" "<=" ">=") ;; Comparison
    (operator_assignment ;; Corrected assignment operators
      "=" "+=" "-=" "*=" "/=" "%=")
    (operator_special ;; Special operators and symbols used in MoonBit syntax
      ":" "->" "|" "." "::" "#")
    (operator_openclose "{" "[" "(" ")" "]" "}")))

(define (moonbit-number-suffix)
  ;; Suffixes for integer types in MoonBit
  `(suffix
    (long "L")         ;; For Int64
    (unsigned "U")     ;; For UInt
    (unsigned_long "UL") ;; For UInt64
    (bigint "N")))      ;; For BigInt

(tm-define (parser-feature lan key)
  (:require (and (== lan "moonbit") (== key "number")))
  `(,(string->symbol key)
    ;; MoonBit supports hex, binary, and octal prefixes, as well as scientific notation.
    (bool_features
     "prefix_0x" "prefix_0b" "prefix_0o"
     "sci_notation")
    ,(moonbit-number-suffix)))

(tm-define (parser-feature lan key)
  (:require (and (== lan "moonbit") (== key "string")))
  `(,(string->symbol key)
    (bool_features
      ;; MoonBit supports unicode escapes
      "unicode_escape_braces")
    ;; Standard C-style escape sequences
    (escape_sequences "\\" "\"" "'" "b" "f" "n" "r" "t")))

