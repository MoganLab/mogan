
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : r7rs-keyword.scm
;; DESCRIPTION : the Scheme Keyword defined in R7RS
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code r7rs-keyword))

(define (r7rs-scheme-base)
  (map symbol->string
    '(abs and append assoc assq assv binary-port? boolean=? boolean? bytevector bytevector-append bytevector-copy bytevector-copy! bytevector-length bytevector-u8-ref bytevector-u8-set! bytevector? caar cadr call-with-current-continuation call-with-port call-with-values call/cc car case cdar cddr cdr ceiling char->integer char-ready? char<=? char<? char=? char>=? char>? char? close-input-port close-output-port close-port complex? cond cond-expand cons current-error-port current-input-port current-output-port denominator do dynamic-wind else eof-object eof-object? eq? equal? eqv? error-object-irritants error-object-message error-object? even? exact exact-integer-sqrt exact-integer? exact? expt features file-error? floor floor-quotient floor-remainder floor/ flush-output-port for-each gcd get-output-bytevector get-output-string include include-ci inexact inexact? input-port-open? input-port? integer->char integer? lambda lcm length list list->string list->vector list-copy list-ref list-set! list-tail list? make-bytevector make-list make-parameter make-string make-vector map max member memq memv min modulo negative? newline not null? number->string number? numerator odd? open-input-bytevector open-input-string open-output-bytevector open-output-string or output-port-open? output-port? pair? parameterize peek-char peek-u8 port? positive? procedure? quasiquote quote quotient raise raise-continuable rational? rationalize read-bytevector read-bytevector! read-char read-error? read-line read-string read-u8 real? remainder reverse round set-car! set-cdr! sin sqrt string string->list string->number string->symbol string->utf8 string->vector string-append string-copy string-copy! string-fill! string-for-each string-length string-map string-ref string-set! string<=? string<? string=? string>=? string>? string? substring symbol->string symbol=? symbol? syntax-error syntax-rules textual-port? truncate truncate-quotient truncate-remainder truncate/ u8-ready? unless unquote unquote-splicing utf8->string vector vector->list vector->string vector-append vector-copy vector-copy! vector-fill! vector-for-each vector-length vector-map vector-ref vector-set! vector? write-bytevector write-char write-string write-u8 zero?)))

(define (r7rs-scheme-case-lambda)
  (map symbol->string '(case-lambda)))

(define (r7rs-scheme-char)
  (map symbol->string
    '(char-alphabetic? char-ci<=? char-ci<? char-ci=? char-ci>=? char-ci>? char-downcase char-foldcase char-lower-case? char-numeric? char-upcase char-upper-case? char-whitespace? digit-value string-ci<=? string-ci<? string-ci=? string-ci>=? string-ci>? string-downcase string-foldcase string-upcase)))

(define (r7rs-scheme-complex)
  (map symbol->string
    '(angle imag-part magnitude make-polar make-rectangular real-part)))

(define (r7rs-scheme-CxR)
  (map symbol->string
    '(caaaar caaadr caaar caadar caaddr caadr cadaar cadadr cadar caddar cadddr caddr cdaaar cdaadr cdaar cdadar cdaddr cdadr cddaar cddadr cddar cdddar cddddr cdddr)))

(define (r7rs-scheme-file)
  (map symbol->string
    '(call-with-input-file call-with-output-file delete-file file-exists? open-binary-input-file open-binary-output-file open-input-file open-output-file with-input-from-file with-output-to-file)))

(define (r7rs-scheme-inexact)
  (map symbol->string '(acos asin atan cos exp finite? infinite? log nan? sin sqrt tan)))

(define (r7rs-scheme-lazy)
  (map symbol->string '(delay delay-force force make-promise promise?)))

(define (r7rs-scheme-process-context)
  (map symbol->string
    '(command-line emergency-exit exit get-environment-variable get-environment-variables)))

(define (r7rs-scheme-read)
  (map symbol->string '(read)))

(define (r7rs-scheme-repl)
  (map symbol->string
    '(interaction-environment)))

(define (r7rs-scheme-time)
  (map symbol->string
    '(current-jiffy current-second jiffies-per-second)))

(define (r7rs-scheme-write)
  (map symbol->string
    '(display write write-shared write-simple)))

(tm-define (r7rs-keywords-others)
  `(,@(r7rs-scheme-base) ,@(r7rs-scheme-case-lambda) ,@(r7rs-scheme-char) ,@(r7rs-scheme-complex) ,@(r7rs-scheme-CxR) ,@(r7rs-scheme-file) ,@(r7rs-scheme-inexact) ,@(r7rs-scheme-lazy) ,@(r7rs-scheme-process-context) ,@(r7rs-scheme-read) ,@(r7rs-scheme-repl) ,@(r7rs-scheme-time) ,@(r7rs-scheme-write)))

(tm-define (r7rs-keywords-constant)
  (list "+inf.0" "-inf.0" "+nan.0" "-nan.0" "#t" "#true" "#f" "#false"))

(tm-define (r7rs-keywords-branch)
  (list "if" "cond" "else" "case" "when"))

(tm-define (r7rs-keywords-define)
  (list
    "define" "define-record-type" "define-syntax" "define-values" "set!" "lambda" "let" "let*" "let-values" "let*-values" "letrec" "letrec*" "let-syntax" "letrec-syntax" "apply" "eval" "environment" "load" "values" "begin" "import" "export" "define-library"))

(tm-define (r7rs-keywords-exception)
  (list "error" "guard" "with-exception-handler"))
