
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : json-lang.scm
;; DESCRIPTION : JSON Language
;; COPYRIGHT   : (C) 2020  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (json json-lang) (:use (prog default-lang)))

(tm-define (parser-feature lan key)
  (:require (and (== lan "json") (== key "keyword")))
  `(,(string->symbol key) (constant "false" "true" "null"))
) ;tm-define

;; Ref: https://ecma-international.org/ecma-262/10.0/index.html#sec-update-expressions
(tm-define (parser-feature lan key)
  (:require (and (== lan "json") (== key "operator")))
  `(,(string->symbol key)
    (operator "+" "-" ":" ",")
    (operator_openclose "{" "[" "(" ")" "]" "}"))
) ;tm-define

;; Ref: https://ecma-international.org/ecma-262/10.0/index.html#sec-literals-numeric-literals
(tm-define (parser-feature lan key)
  (:require (and (== lan "json") (== key "number")))
  `(,(string->symbol key) (bool_features "sci_notation"))
) ;tm-define

(tm-define (parser-feature lan key)
  (:require (and (== lan "json") (== key "string")))
  `(,(string->symbol key)
    (bool_features "escape_char_after_backslash" "unicode_escape")
    (escape_sequences "\\" "/" "\"" "b" "f" "n" "r" "t" "u"))
) ;tm-define

(tm-define (parser-feature lan key)
  (:require (and (== lan "json") (== key "comment")))
  `(,(string->symbol key) (inline "//") (block_comment "/*" "*/"))
) ;tm-define

(define (notify-json-syntax var val)
  (syntax-read-preferences "json")
) ;define

(define-preferences ("syntax:json:none" "red" notify-json-syntax)
 ("syntax:json:comment" "comment-color" notify-json-syntax)
 ("syntax:json:error" "dark red" notify-json-syntax)
 ("syntax:json:constant" "json-constant-color" notify-json-syntax)
 ("syntax:json:constant_number" "json-number-color" notify-json-syntax)
 ("syntax:json:constant_string" "json-string-color" notify-json-syntax)
 ("syntax:json:constant_char" "json-string-color" notify-json-syntax)
 ("syntax:json:variable_identifier" "json-key-color" notify-json-syntax)
 ("syntax:json:operator" "json-operator-color" notify-json-syntax)
 ("syntax:json:operator_openclose" "json-openclose-color" notify-json-syntax)
 ("syntax:json:keyword" "json-constant-color" notify-json-syntax)
) ;define-preferences
