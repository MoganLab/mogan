
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : debug.scm
;; DESCRIPTION : debugging tools
;; COPYRIGHT   : (C) 2002  Joris van der Hoeven, David Allouche
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel boot debug))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Output
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (display* . l) "Display all objects in @l." (for-each display l))

(define-public (display-err x)
  "Display @x to the error port."
  (tm-errput (display-to-string x))
) ;define-public

(define-public (display-err* . l)
  "Display all objects in @l to the error port."
  (for-each display-err l)
) ;define-public

(define-public (tm-display-error . l)
  (apply display-err* `(,"TeXmacs] " ,@l ,"\n"))
) ;define-public

(define-public (write* . l)
  "Write all objects in @l to standard output."
  (for-each write l)
) ;define-public

(define-public (write-err x)
  "Write @x to the error port."
  (tm-errput (object->string x))
) ;define-public

(define-public (write-err* . l)
  "Write all objects in @l to the error port."
  (for-each write-err l)
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Various tools
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public footer-hook (lambda (s) s))

(define-macro (benchmark message . args)
  `(let ((start (texmacs-time)))
     (begin ,@args)
     (display* ,message ," " (- (texmacs-time) start) ,"msec\n"))
) ;define-macro

(define-public (write-diff t u)
  (cond ((== t u) (noop))
        ((or (not (and (pair? t) (pair? u))) (not (= (length t) (length u))))
         (display "< ")
         (write t)
         (display "\n> ")
         (write u)
         (display "\n")
        ) ;
        (else (write-diff (car t) (car u)) (write-diff (cdr t) (cdr u)))
  ) ;cond
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; TeXmacs errors and assertions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (scm-error* type caller message . opt)
  (apply error type caller message opt)
) ;define

(define-public (texmacs-error where message . args)
  (scm-error* 'texmacs-error where message args #f)
) ;define-public

(define-public (check-arg-type pred arg caller)
  (if (pred arg)
    arg
    (scm-error* 'wrong-type-arg caller "Wrong type argument: ~S" (list arg) '())
  ) ;if
) ;define-public

(define-public (check-arg-number pred num caller)
  (if (pred num)
    num
    (scm-error* 'wrong-number-of-args
      caller
      "Wrong number of arguments: ~A"
      (list num)
      '()
    ) ;scm-error*
  ) ;if
) ;define-public

(define-public (check-arg-range pred arg caller)
  (if (pred arg)
    arg
    (scm-error* 'out-of-range caller "Argument out of range: ~S" (list arg) '())
  ) ;if
) ;define-public

(define-public (syntax-error where message . args)
  (scm-error* 'syntax-error where message args #f)
) ;define-public

(define-public (former . l) (texmacs-error "former" "no next method"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Deprecated features
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define deprecated-done (list))

(define-public (deprecated-function old . l)
  (if (not (member old deprecated-done))
    (begin
      (set! deprecated-done (cons old deprecated-done))
      (display* "TeXmacs] warning, deprecated function '" old "'\n")
      (if (not (null? l))
        (begin
          (display* "       ] please reimplement using '" (car l) "'")
          (for-each (lambda (x) (display* ", '" x "'")) (cdr l))
          (display* "\n")
        ) ;begin
      ) ;if
    ) ;begin
  ) ;if
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Debugging
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (wrap-catch proc)
  ;; Wrap a procedure in a closure which displays and passes exceptions.
  (lambda args
    (lazy-catch #t
      (lambda () (apply proc args))
      (lambda err (tm-display-error "Guile error: " (list err)) (apply throw err))
    ) ;lazy-catch
  ) ;lambda
) ;define-public

(define-public (wrap-catch-list expr)
  ;; Similar to wrap-catch for a scheme expression in list form.
  `(lazy-catch ,#t
     (lambda ,() ,expr)
     (lambda err
       (tm-display-error "Guile error: " (list err))
       (apply throw err)))
) ;define-public

(define trace-level 0)

(define (trace-indent)
  ;; Produce the string to be used to indent trace output.
  (let rec
    ((n trace-level) (s '()))
    (if (equal? 0 n) (apply string-append s) (rec (1- n) (cons "| " s)))
  ) ;let
) ;define

(define-public (trace-display . args)
  ;; As display but also print trace indentation.
  (display (trace-indent))
  (for-each (lambda (a) (display (if (string? a) a (object->string a))) (display " "))
    args
  ) ;for-each
  (newline)
) ;define-public

(define-public-macro (trace-variables . vars)
  ;; Use trace-display to show the name and value of some variables.
  (define (trace-one-variable v)
    `(trace-display (string-append ,(symbol->string v)
                      ,": "
                      (object->string ,v)))
  ) ;define
  `(begin ,@(map trace-one-variable vars))
) ;define-public-macro


;;   Trace levels
;; Display parameters and return value of a function.
;; Increase the trace indentation to show the call hierarchy.
;; Do not preserve tail recursion.

(define-public (wrap-trace name lam)
  (lambda args
    (trace-display (if (null? args)
                     (string-append "[" name "]")
                     (apply string-append
                       `(,"["
                         ,name
                         ,@(map (lambda (x)
                                  (string-append " " (object->string x)))
                             args)
                         ,"]")
                     ) ;apply
                   ) ;if
    ) ;trace-display
    (set! trace-level (1+ trace-level))
    (lazy-catch #t
      (lambda ()
        (let ((res (apply lam args)))
          (set! trace-level (1- trace-level))
          (trace-display (object->string res))
          res
        ) ;let
      ) ;lambda
      (lambda err (set! trace-level (1- trace-level)) (apply throw err))
    ) ;lazy-catch
  ) ;lambda
) ;define-public

(define-public-macro (set-trace-level! . names)
  ;; Make each function a trace-level. Functions can be set multiple
  ;; times, only the first application is effective.
  ;; Parameters are function names
  `(begin
     ,@(map (lambda (name)
              `(if (not (procedure-property ,name 'trace-wrapped))
                 (begin
                   (set! ,name (wrap-trace ,(symbol->string name) ,name))
                   (set-procedure-property! ,name 'trace-wrapped ,#t))))
         names))
) ;define-public-macro

;;   Trace points
;; Display parameters of a function when it is called.
;; Preserve tail recursion.

(define-public (wrap-trace-point lam msg)
  (lambda args
    (trace-display (string-append "[" msg " " (object->string args) "]"))
    (apply lam args)
  ) ;lambda
) ;define-public

(define-public-macro (set-trace-point! name . opt)
  ;; Make one trace point.
  ;; Care must be taken of net setting the same function multiple times.
  (let ((msg (if (null? opt) (symbol->string name) (car opt))))
    `(set! ,name (wrap-trace-point ,name ,msg))
  ) ;let
) ;define-public-macro
