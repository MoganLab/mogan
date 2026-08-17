
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : plugin-convert.scm
;; DESCRIPTION : Convert mathematical formulas to plugin input
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils plugins plugin-convert)
  (:use (convert rewrite tmtm-brackets))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main conversion routines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define current-plugin-input-stree "")

;; 捕获期间所有 handler 经 plugin-output 写往独立的 string port，
;; 与全局 cout（C++ 日志）隔离，避免日志混入插件输入

(define plugin-input-port (current-output-port))

(tm-define (plugin-output s) (display s plugin-input-port))

(define (call-with-plugin-output-string thunk)
  (call-with-output-string (lambda (p)
                             (let ((saved plugin-input-port))
                               (dynamic-wind (lambda () (set! plugin-input-port p))
                                 thunk
                                 (lambda () (set! plugin-input-port saved))
                               ) ;dynamic-wind
                             ) ;let
                           ) ;lambda
  ) ;call-with-output-string
) ;define

(tm-define (plugin-math-input l)
  (:synopsis "Convert mathematical input to a string")
  (:argument l "A list of the form @(tuple plugin expr)")
  (set! current-plugin-input-stree (caddr l))
  (set! plugin-input-current-plugin (cadr l))
  (call-with-plugin-output-string plugin-input-caller)
) ;tm-define

(define (plugin-input-caller)
  (plugin-input (tree-upgrade-big current-plugin-input-stree))
) ;define

(tm-define (plugin-input t)
  (cond ((string? t) (plugin-input-tmtokens (string->tmtokens t 0 (string-length t))))
        ((tree? t) (plugin-input (tree->stree t)))
        (else (let* ((f (car t)) (args (cdr t)) (im (plugin-input-ref f)))
                (cond ((!= im #f) (im args))
                      (else (noop))
                ) ;cond
              ) ;let*
        ) ;else
  ) ;cond
) ;tm-define

(tm-define (plugin-input-arg t)
  (if (and (string? t) (= (length (string->tmtokens t 0 (string-length t))) 1))
    (plugin-input t)
    (begin
      (plugin-output "(")
      (plugin-input t)
      (plugin-output ")")
    ) ;begin
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; conversion of strings
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (string-find-char s i n c)
  (cond ((>= i n) n)
        ((== (string-ref s i) c) i)
        (else (string-find-char s (+ i 1) n c))
  ) ;cond
) ;define

(define (string-find-end s i n pred)
  (cond ((>= i n) n)
        ((not (pred (string-ref s i))) i)
        (else (string-find-end s (+ i 1) n pred))
  ) ;cond
) ;define

(define (string->tmtokens s i n)
  (cond ((>= i n) '())
        ((== (string-ref s i) #\<)
         (let ((j (min n (+ (string-find-char s i n #\>) 1))))
           (cons (substring s i j) (string->tmtokens s j n))
         ) ;let
        ) ;
        ((char-alphabetic? (string-ref s i))
         (let ((j (string-find-end s i n char-alphabetic?)))
           (cons (substring s i j) (string->tmtokens s j n))
         ) ;let
        ) ;
        ((char-numeric? (string-ref s i))
         (let ((j (string-find-end s i n char-numeric?)))
           (cons (substring s i j) (string->tmtokens s j n))
         ) ;let
        ) ;
        (else (cons (substring s i (+ 1 i)) (string->tmtokens s (+ 1 i) n)))
  ) ;cond
) ;define

(define (plugin-input-tmtoken s)
  (let ((im (plugin-input-ref s)))
    (if (== im #f)
      (if (and (!= s "") (== (string-ref s 0) #\<))
        (plugin-output (substring s 1 (- (string-length s) 1)))
        (plugin-output s)
      ) ;if
      (if (procedure? im) (im s) (plugin-output im))
    ) ;if
  ) ;let
) ;define

(define (plugin-input-tmtokens l)
  (if (nnull? l)
    (begin
      (plugin-input-tmtoken (car l))
      (plugin-input-tmtokens (cdr l))
    ) ;begin
  ) ;if
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; conversion of other nodes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (plugin-input-with args)
  (if (null? (cdr args)) (plugin-input (car args)) (plugin-input-with (cdr args)))
) ;define

(define (plugin-input-concat-big op args)
  (let* ((i (list-find-index args (lambda (x) (== x '(big ".")))))
         (head (if i (sublist args 0 i) args))
         (tail (if i (sublist args (+ i 1) (length args)) '()))
         (bigop `(big-around ,(small-bracket op) (concat ,@head)))
        ) ;
    (plugin-input `(concat ,bigop ,@tail))
  ) ;let*
) ;define

(define (plugin-input-concat args)
  (cond ((null? args) (noop))
        ((and (func? (car args) 'big) (nnull? (cdr args)))
         (plugin-input-concat-big (car args) (cdr args))
        ) ;
        (else (plugin-input (car args)) (plugin-input-concat (cdr args)))
  ) ;cond
) ;define

(define (plugin-input-math args)
  (plugin-input (car args))
) ;define

(define (plugin-input-frac args)
  (plugin-output "(")
  (plugin-input-arg (car args))
  (plugin-output "/")
  (plugin-input-arg (cadr args))
  (plugin-output ")")
) ;define

(define (plugin-input-sqrt args)
  (if (= (length args) 1)
    (begin
      (plugin-output "sqrt(")
      (plugin-input (car args))
      (plugin-output ")")
    ) ;begin
    (begin
      (plugin-input-arg (car args))
      (plugin-output "^(1/")
      (plugin-input-arg (cadr args))
      (plugin-output ")")
    ) ;begin
  ) ;if
) ;define

(define (plugin-input-rsub args)
  (plugin-output "[")
  (plugin-input (car args))
  (plugin-output "]")
) ;define

(define (plugin-input-rsup args)
  (plugin-output "^")
  (plugin-input-arg (car args))
) ;define

(define (plugin-input-around args)
  (plugin-input (tree-downgrade-brackets (cons 'around args) #f #t))
) ;define

(define (plugin-input-around* args)
  (plugin-input (tree-downgrade-brackets (cons 'around* args) #f #t))
) ;define

(define (plugin-input-big-around args)
  (let* ((b `(big-around ,@args))
         (name (big-name b))
         (sub (big-subscript b))
         (sup (big-supscript b))
         (body (big-body b))
        ) ;
    (plugin-output name)
    (plugin-output "(")
    (when sub
      (plugin-input sub)
      (plugin-output ",")
    ) ;when
    (when (and sub sup)
      (plugin-input sup)
      (plugin-output ",")
    ) ;when
    (plugin-input body)
    (plugin-output ")")
  ) ;let*
) ;define

(define (plugin-input-large args)
  (plugin-output (car args))
) ;define

(define (plugin-input-script-assign args)
  (plugin-output ":=")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Conversion of matrices
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (plugin-input-descend-last args)
  (if (null? (cdr args))
    (plugin-input (car args))
    (plugin-input-descend-last (cdr args))
  ) ;if
) ;define

(define (plugin-input-det args)
  (plugin-output "matdet(")
  (plugin-input-descend-last args)
  (plugin-output ")")
) ;define

(define (rewrite-cell c)
  (if (and (list? c) (== (car c) 'cell)) (cadr c) c)
) ;define

(define (rewrite-row r)
  (if (null? r) r (cons (rewrite-cell (car r)) (rewrite-row (cdr r))))
) ;define

(define (rewrite-table t)
  (if (null? t) t (cons (rewrite-row (cdar t)) (rewrite-table (cdr t))))
) ;define

(define (plugin-input-row r)
  (if (null? (cdr r))
    (plugin-input (car r))
    (begin
      (plugin-input (car r))
      (plugin-output ", ")
      (plugin-input-row (cdr r))
    ) ;begin
  ) ;if
) ;define

(define (plugin-input-var-rows t)
  (if (nnull? t)
    (begin
      (plugin-output "; ")
      (plugin-input-row (car t))
      (plugin-input-var-rows (cdr t))
    ) ;begin
  ) ;if
) ;define

(define (plugin-input-rows t)
  (plugin-output "[")
  (plugin-input-row (car t))
  (plugin-input-var-rows (cdr t))
  (plugin-output "]")
) ;define

(define (plugin-input-table args)
  (let ((t (rewrite-table args)))
    (plugin-input (cons 'rows t))
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Lazy input converters
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define lazy-input-converter-table (make-ahash-table))

(tm-define-macro (lazy-input-converter module plugin)
  (lazy-input-converter-force plugin)
  (ahash-set! lazy-input-converter-table plugin module)
  '(noop)
) ;tm-define-macro

(define (lazy-input-converter-force plugin2)
  (with plugin
    (if (string? plugin2) (string->symbol plugin2) plugin2)
    (with module
      (ahash-ref lazy-input-converter-table plugin)
      (if module
        (begin
          (ahash-remove! lazy-input-converter-table plugin)
          (module-load module)
        ) ;begin
      ) ;if
    ) ;with
  ) ;with
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Initialization subroutines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define plugin-input-current-plugin "generic")

(define (plugin-input-converters-rules name l)
  (if (null? l)
    '()
    (cons (let* ((rule (car l)) (key (car rule)) (im (list 'unquote (cadr rule))))
            (list (list 'plugin-input-converter% (list name key) im))
          ) ;let*
      (plugin-input-converters-rules name (cdr l))
    ) ;cons
  ) ;if
) ;define

(tm-define-macro (plugin-input-converters name2 . l)
  (let ((name (if (string? name2) name2 (symbol->string name2))))
    (lazy-input-converter-force name)
    (logic-group plugin-input-converters% ,name)
    `(logic-rules ,@(plugin-input-converters-rules name l))
  ) ;let
) ;tm-define-macro

(define (plugin-input-ref key)
  (lazy-input-converter-force plugin-input-current-plugin)
  (let ((im (logic-ref plugin-input-converter% (list plugin-input-current-plugin key)))
       ) ;
    (if im im (logic-ref plugin-input-converter% (list "generic" key)))
  ) ;let
) ;define

(tm-define (plugin-supports-math-input-ref key)
  (lazy-input-converter-force key)
  (logic-in? key plugin-input-converters%)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Initialization
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(plugin-input-converters generic
  (with plugin-input-with)
  (concat plugin-input-concat)
  (document plugin-input-concat)
  (math plugin-input-math)
  (frac plugin-input-frac)
  (sqrt plugin-input-sqrt)
  (rsub plugin-input-rsub)
  (rsup plugin-input-rsup)
  (around plugin-input-around)
  (around* plugin-input-around*)
  (big-around plugin-input-big-around)
  (left plugin-input-large)
  (middle plugin-input-large)
  (right plugin-input-large)
  (tabular plugin-input-descend-last)
  (tabular* plugin-input-descend-last)
  (block plugin-input-descend-last)
  (block* plugin-input-descend-last)
  (matrix plugin-input-descend-last)
  (det plugin-input-det)
  (bmatrix plugin-input-descend-last)
  (tformat plugin-input-descend-last)
  (table plugin-input-table)
  (rows plugin-input-rows)
  (script-assign plugin-input-script-assign)

  ("<longequal>" "==")
  ("<assign>" ":=")
  ("<plusassign>" "+=")
  ("<minusassign>" "-=")
  ("<timesassign>" "*=")
  ("<overassign>" "/=")
  ("<lflux>" "<less><less>")
  ("<gflux>" "<gtr><gtr>")

  ("<implies>" "=<gtr>")
  ("<Rightarrow>" "=<gtr>")
  ("<equivalent>" "<less>=<gtr>")
  ("<Leftrightarrow>" "<less>=<gtr>")
  ("<neg>" "not ")
  ("<wedge>" " and ")
  ("<vee>" " or ")
  ("<neq>" "!=")
  ("<less>" "<less>")
  ("<gtr>" "<gtr>")
  ("<leq>" "<less>=")
  ("<geq>" "<gtr>=")
  ("<leqslant>" "<less>=")
  ("<geqslant>" "<gtr>=")
  ("<ll>" "<less><less>")
  ("<gg>" "<gtr><gtr>")
  ("<into>" "-<gtr>")
  ("<mapsto>" "|-<gtr>")
  ("<rightarrow>" "-<gtr>")
  ("<transtype>" ":<gtr>")

  ("<um>" "-")
  ("<upl>" "+")
  ("<times>" "*")
  ("<ast>" "*")
  ("<cdot>" "*")
  ("<ldots>" "..")
  ("<colons>" "::")
  ("<sharp>" "#")
  ("<circ>" "@")

  ("<bbb-C>" "CC")
  ("<bbb-F>" "FF")
  ("<bbb-N>" "NN")
  ("<bbb-K>" "KK")
  ("<bbb-R>" "RR")
  ("<bbb-Q>" "QQ")
  ("<bbb-Z>" "ZZ")
  ("<mathe>" "(exp(1))")
  ("<mathpi>" "(4*atan(1))")
  ("<mathi>" "(sqrt(-1))")

  ("<alpha>" "alpha")
  ("<beta>" "beta")
  ("<gamma>" "gamma")
  ("<delta>" "delta")
  ("<epsilon>" "epsilon")
  ("<varepsilon>" "epsilon")
  ("<zeta>" "zeta")
  ("<eta>" "eta")
  ("<theta>" "theta")
  ("<vartheta>" "theta")
  ("<iota>" "iota")
  ("<kappa>" "kappa")
  ("<lambda>" "lambda")
  ("<mu>" "mu")
  ("<nu>" "nu")
  ("<xi>" "xi")
  ("<omicron>" "omicron")
  ("<pi>" "pi")
  ("<varpi>" "pi")
  ("<rho>" "rho")
  ("<varrho>" "varrho")
  ("<sigma>" "sigma")
  ("<varsigma>" "sigma")
  ("<tau>" "tau")
  ("<upsilon>" "upsilon")
  ("<phi>" "phi")
  ("<varphi>" "phi")
  ("<chi>" "chi")
  ("<psi>" "psi")
  ("<omega>" "omega")

  ("<Alpha>" "Alpha")
  ("<Beta>" "Beta")
  ("<Gamma>" "Gamma")
  ("<Delta>" "Delta")
  ("<Epsilon>" "Epsilon")
  ("<Zeta>" "Zeta")
  ("<Eta>" "Eta")
  ("<Theta>" "Theta")
  ("<Iota>" "Iota")
  ("<Kappa>" "Kappa")
  ("<Lambda>" "Lambda")
  ("<Mu>" "Mu")
  ("<Nu>" "Nu")
  ("<Xi>" "Xi")
  ("<Omicron>" "Omicron")
  ("<Pi>" "Pi")
  ("<Rho>" "Rho")
  ("<Sigma>" "Sigma")
  ("<Tau>" "Tau")
  ("<Upsilon>" "Upsilon")
  ("<Phi>" "Phi")
  ("<Chi>" "Chi")
  ("<Psi>" "Psi")
  ("<Omega>" "Omega")
) ;plugin-input-converters
