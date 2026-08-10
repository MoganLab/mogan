
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : goldfish-ast-lang.scm
;; DESCRIPTION : the Goldfish Scheme Language
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code goldfish-ast-lang)
  (:use (code default-ast-lang)
    (code r7rs-keyword)
    (code srfi-keyword)
    (code liii-keyword)
  ) ;:use
) ;texmacs-module

(tm-define (parser-feature lan key)
  (:require (and (== lan "goldfish") (== key "id")))
  `(,(string->symbol key) ,"scheme")
) ;tm-define

(tm-define (parser-feature lan key)
  (:require (and (== lan "goldfish") (== key "keytoken")))
  `(,(string->symbol key)
    (constant ,@(r7rs-keywords-constant)
      ,"pi"
      ,"*stdin*"
      ,"*stdout*"
      ,"*stderr*"
      ,"*load-hook*"
      ,"*autoload-hook*"
      ,"*error-hook*"
      ,"*read-error-hook*"
      ,"*rootlet-redefinition-hook*"
      ,"*unbound-variable-hook*"
      ,"*missing-close-paren-hook*")
    (declare_type ,@(r7rs-keywords-define) ,@(liii-keywords-define))
    (keyword ,@(r7rs-keywords-others)
      ,@(srfi-1-keywords)
      ,@(srfi-8-keywords)
      ,@(srfi-13-keywords)
      ,@(srfi-60-keywords)
      ,@(srfi-78-keywords)
      ,@(liii-keywords)
      ,"*load-path*"
      ,"*goldfish*"
      ,"*features*"
      ,"*libraries*"
      ,"*cload-directory*"
      ,"*#readers*"
      ,"with-input-from-string"
      ,"with-output-to-string"
      ,"equivalent?"
      ,"complex"
      ,"directory?"
      ,"getenv"
      ,"help"
      ,"bignum"
      ,"append"
      ,"copy"
      ,"procedure-source"
      ,"integer-decode-float"
      ,"random"
      ,"nan"
      ,"nan-payload"
      ,"format"
      ,"object->string"
      ,"immutable!"
      ,"immutable?"
      ,"make-hash-table"
      ,"hash-table"
      ,"hash-table?"
      ,"hash-table-ref"
      ,"hash-table-set!"
      ,"hash-table-entries"
      ,"hash-code")
    (error ,"syntax-error"
      ,"wrong-type-arg"
      ,"immutable-error"
      ,"out-of-range"
      ,"division-by-zero"
      ,"unbound-variable"
      ,"read-error"
      ,"format-error"
      ,"missing-method"
      ,"out-of-memory"
      ,"bad-result"
      ,"no-catch"
      ,"wrong-number-of-args"
      ,"io-error"
      ,"bignum-error"
      ,@(liii-keywords-error))
    (keyword_conditional ,@(r7rs-keywords-branch))
    (keyword_control ,@(r7rs-keywords-exception) ,"catch")
    (normal-brackets "{" "[" "(" ")" "]" "}")
    (operator "'" "," "#," "#'" ",@" "#,@" "#!" "#;")
    (string_quote "\""))
) ;tm-define

(tm-define (parser-feature lan key)
  (:require (and (== lan "goldfish") (== key "special_symbol")))
  `(,(string->symbol key) (tokenize "symbol" "first_symbol"))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences for syntax highlighting
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(tm-define (parser-feature lan key)
  (:require (and (== lan "goldfish") (== key "light_theme")))
  `(,(string->symbol key)
    ("#000000" "none")
    ("#4040c0" "constant")
    ("#204080" "keyword")
    ("#309090" "keyword_conditional" "keyword_control" "declare_type")
    ("#0000AB" "first_symbol")
    ("#4040c0" "number" "boolean")
    ("brown" "comment" "block_comment")
    ("#D32F2F" "string_content" "string_quote" "character")
    ("#BF2C2C" "escape_sequence")
    ("red" "error")
    ("#000000"
     "symbol"
     "operator"
     "normal-brackets"
     "vector_tag"
     "byte_vector_tag")
    ("#800080" "ERROR")
    ("#ff115f" "(0" ")0" "[0" "]0" "{0" "}0")
    ("#ff8805" "(1" ")1" "[1" "]1" "{1" "}1")
    ("#888800" "(2" ")2" "[2" "]2" "{2" "}2")
    ("#008700" "(3" ")3" "[3" "]3" "{3" "}3")
    ("#3689e6" "(4" ")4" "[4" "]4" "{4" "}4")
    ("#a100a1" "(5" ")5" "[5" "]5" "{5" "}5"))
) ;tm-define

(tm-define (parser-feature lan key)
  (:require (== key "brackets"))
  `(,(string->symbol key) (6 "(" ")") (6 "[" "]") (6 "{" "}"))
) ;tm-define
