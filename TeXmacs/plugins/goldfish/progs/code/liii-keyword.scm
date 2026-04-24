
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : liii-keyword.scm
;; DESCRIPTION : keywords for the Goldfish Scheme Language
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code liii-keyword))

(define (liii-base)
  (map symbol->string
    '(== != display* in? compose identity case-class? any?)))

(define (liii-check)
  (map symbol->string
    '(test check check-set-mode! check-catch check-report
      check-failed? check-true check-false)))

(define (liii-list)
  (map symbol->string
    '(list-view flat-map flatten list-null? list-not-null? not-null-list?)))

(define (liii-string)
  (map symbol->string
    '(string-starts? string-ends? string-remove-prefix string-remove-suffix)))

(define (liii-os)
  (map symbol->string
    '(os-arch os-type os-windows? os-linux? os-macos? os-temp-dir
      os-call system mkdir chdir rmdir getenv unsetenv getcwd
      listdir access getlogin getpid)))

(define (liii-queue)
  (map symbol->string
    '(queue queue? queue-empty? queue-size queue-front queue-back
      queue-pop! queue-push! queue->list)))

(define (liii-stack)
  (map symbol->string
    '(stack stack? stack-empty? stack-size stack-top
      stack-push! stack-pop! stack->list)))

(define (liii-sys)
  (map symbol->string '(argv)))

(define (liii-uuid)
  (map symbol->string '(uuid4)))

(define (liii-base64)
  (map symbol->string '(string-base64-encode bytevector-base64-encode base64-encode string-base64-decode bytevector-base64-decode base64-decode)))

(define (liii-bitwise)
  (map symbol->string '(bitwise-not bitwise-and 
  bitwise-ior bitwise-xor bitwise-or bitwise-nor bitwise-nand
  bit-count arithmetic-shift
  lognot logand logior logxor
  ash)))

(define (liii-lang)
  (map symbol->string
    '(box rich-integer rich-string rich-char rich-vector rich-hash-table)))

(tm-define (liii-methods)
  (map symbol->string
    '(:equals :to-string :is-instance-of
      :apply :get :get-or-else :or-else :collect 
      :exists :forall :contains :empty? :defined?
      :map :filter :flat-map :for-each :fold :fold-right :reduce :reduce-right
      :take :take-right :drop :drop-right :take-while :drop-while
      :sum :produce :min :max :min-by :max-by
      :sorted :sort-by :sortWith
      :union :intersect :diff :distinct
      :find :count :length :size
      :group-by :partition :zip :zip-with-index :unzip
      :par
      :make-string :to-list :to-vector :to-map :to-set)))

(tm-define (liii-keywords)
  `(,@(liii-base) ,@(liii-check) ,@(liii-list) ,@(liii-string) ,@(liii-os) ,@(liii-queue)
    ,@(liii-stack) ,@(liii-sys) ,@(liii-uuid) ,@(liii-base64) ,@(liii-bitwise) ,@(liii-lang)))

(tm-define (liii-keywords-define)
  (map symbol->string
    '(and-let* typed-lambda defined? define-macro define-constant
      autoload require provide define* lambda* eval-string case*
      typed-define define-case-class)))

(tm-define (liii-keywords-error)
  (map symbol->string
    '(os-error file-not-found-error not-a-directory-error file-exists-error
      timeout-error type-error value-error ??? not-implemented-error)))
