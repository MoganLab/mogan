
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : r7rs-lang.scm
;; DESCRIPTION : the Scheme Language defined in R7RS
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code r7rs-lang)
  (:use (prog default-lang)
        (code r7rs-keyword)))

(tm-define (parser-feature lan key)
  (:require (and (== lan "r7rs") (== key "identifier")))
  `(,(string->symbol key)
    (extra_chars "?" "+" "-" "." "!" "*" ">" "=" "<" "#")
    (start_chars "?" "+" "-" "." "!" "*" ">" "=" "<" "#")))

(tm-define (parser-feature lan key)
  (:require (and (== lan "r7rs") (== key "keyword")))
  `(,(string->symbol key)
    (extra_chars "?" "+" "-" "." "!" "*" ">" "=" "<" "#")
    (constant ,@(r7rs-keywords-constant))
    (keyword ,@(r7rs-keywords-others))
    (declare_type ,@(r7rs-keywords-define))
    (keyword_conditional ,@(r7rs-keywords-branch))
    (keyword_control ,@(r7rs-keywords-exception))))

(tm-define (parser-feature lan key)
  (:require (and (== lan "r7rs") (== key "operator")))
  `(,(string->symbol key)
    (operator "=" "+" "-" "*" "/" "=>" "->" ">" "<" ">=" "<=")
    (operator_special "@" "," "'" "`")
    (operator_openclose "{" "[" "(" ")" "]" "}")))

(define (r7rs-number-suffix)
  `(suffix
    (imaginary "i")))

(tm-define (parser-feature lan key)
  (:require (and (== lan "r7rs") (== key "number")))
  `(,(string->symbol key)
    (bool_features "prefix_#")
    (separator "_")
    ,(r7rs-number-suffix)))

(tm-define (parser-feature lan key)
  (:require (and (== lan "r7rs") (== key "string")))
  `(,(string->symbol key)
    (bool_features 
     "hex_with_8_bits" "hex_with_16_bits"
     "hex_with_32_bits" "octal_upto_3_digits")
    (escape_sequences "\\" "\"" "a" "b" "f" "n" "r" "t" "v")
    (pairs "\"")))

; See: https://r7rs.org/doc/v6.1.0/Single-Line-Comments.html
(tm-define (parser-feature lan key)
  (:require (and (== lan "r7rs") (== key "comment")))
  `(,(string->symbol key)
    (inline ";")))

(define (notify-r7rs-syntax var val)
  (syntax-read-preferences "r7rs"))

(define-preferences
  ("syntax:r7rs:none" "red" notify-r7rs-syntax)
  ("syntax:r7rs:comment" "brown" notify-r7rs-syntax)
  ("syntax:r7rs:declare_type" "#309090" notify-r7rs-syntax)
  ("syntax:r7rs:keyword_conditional" "#309090" notify-r7rs-syntax)
  ("syntax:r7rs:keyword_control" "#309090" notify-r7rs-syntax)
  ("syntax:r7rs:keyword" "#204080" notify-r7rs-syntax)
  ("syntax:r7rs:constant_number" "#4040c0" notify-r7rs-syntax)
  ("syntax:r7rs:constant_string" "dark grey" notify-r7rs-syntax)
  ("syntax:r7rs:constant_char" "#333333" notify-r7rs-syntax)
  ("syntax:r7rs:operator_special" "dark magenta" notify-r7rs-syntax)
  ("syntax:r7rs:operator_openclose" "dark" notify-r7rs-syntax)
  ("syntax:r7rs:variable_identifier" "#204080" notify-r7rs-syntax)
  ("syntax:r7rs:declare_category" "#d030d0" notify-r7rs-syntax))
