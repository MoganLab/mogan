
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : boot.scm
;; DESCRIPTION : some global variables, public macros, on-entry, on-exit and
;;               initialization of the TeXmacs module system
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define texmacs-user (current-module))

(define temp-module (current-module))

(define temp-value #f)

(define (guile-a?)
  (equal? (scheme-dialect) "guile-a")
) ;define

(define (guile-b?)
  (equal? (scheme-dialect) "guile-b")
) ;define

(define (guile-c?)
  (equal? (scheme-dialect) "guile-c")
) ;define

(define (guile-b-c?)
  (or (guile-b?) (guile-c?))
) ;define
(if (guile-c?) (use-modules (ice-9 rdelim) (ice-9 pretty-print)))

(define has-look-and-feel? (lambda (x) (== x "emacs")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Redirect standard output
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define original-display display)

(define original-write write)

(define (display . l)
  "display one object on the standard output or a specified port."
  (if (or (null? l) (not (null? (cdr l))))
    (apply original-display l)
    (tm-output (display-to-string (car l)))
  ) ;if
) ;define

(define (write . l)
  "write an object to the standard output or a specified port."
  (if (or (null? l) (not (null? (cdr l))))
    (apply original-write l)
    (tm-output (object->string (car l)))
  ) ;if
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Provide functions if not defined and public macros
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-macro (provide-public head . body)
  (if (or (and (symbol? head) (not (defined? head)))
        (and (pair? head) (symbol? (car head)) (not (defined? (car head))))
      ) ;or
    `(define-public ,head ,@body)
    '(noop)
  ) ;if
) ;define-macro

(if (guile-a?)
  (define-macro (define-public-macro head . body)
    `(define-public ,(car head)
       ;; FIXME: why can't we use procedure->macro
       ;; for a non-memoizing variant?
       (procedure->memoizing-macro (lambda (cmd env)
                                     (apply (lambda ,(cdr head) ,@body)
                                       (cdr cmd)))))
  ) ;define-macro
) ;if

(if (not (guile-a?))
  (define-macro (define-public-macro head . body)
    `(begin
       (define-macro ,(car head) (lambda ,(cdr head) ,@body))
       (export ,(car head)))
  ) ;define-macro
) ;if

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; On-entry and on-exit macros
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (quit-TeXmacs-scheme)
  (noop)
) ;define

(define-macro (on-entry . cmd) `(begin ,@cmd))

(define-macro (on-exit . cmd)
  `(set! quit-TeXmacs-scheme (lambda ,() ,@cmd (,quit-TeXmacs-scheme)))
) ;define-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Module switching
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-macro (with-module module . body)
  `(begin
     (set! temp-module (current-module))
     (set-current-module ,module)
     ,@body
     (set-current-module temp-module))
) ;define-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Module handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(if (guile-a?)
  (begin
    (define import-from use-modules)
    (define re-export export)
  ) ;begin
) ;if

(if (guile-b-c?)
  (begin
    (define-macro (import-from . modules)
      `(process-use-modules (list ,@(map (lambda (m)
                                           `(list ,@(compile-interface-spec m)))
                                      modules)))
    ) ;define-macro
    ;; FIXME: why does this not work?
    ;; (define-macro (import-from . modules)
    ;;   (define (import-from-body module)
    ;;     `(module-use! (current-module) (resolve-module ',module)))
    ;;   `(begin
    ;;     ,@(map import-from-body modules)))
  ) ;begin
) ;if

(define-macro (inherit-modules . which-list)
  (define (module-exports which)
    (let* ((m (resolve-module which)) (m-public (module-ref m '%module-public-interface)))
      (module-map (lambda (sym var) sym) m-public)
    ) ;let*
  ) ;define
  (let ((l (apply append (map module-exports which-list))))
    `(begin (use-modules ,@which-list) (re-export ,@l))
  ) ;let
) ;define-macro

(define-macro (texmacs-module name . options)
  (define (transform action)
    (cond ((not (pair? action)) (noop))
          ((equal? (car action) :use) (cons 'use-modules (cdr action)))
          ((equal? (car action) :inherit) (cons 'inherit-modules (cdr action)))
          ((equal? (car action) :export)
           (display "Warning] The option :export is no longer supported\n")
           (display "       ] Please use tm-define instead\n")
          ) ;
          (else '(noop))
    ) ;cond
  ) ;define
  (let ((l (map-in-order transform options)))
    (if (guile-b-c?)
      (set! l (cons `(module-use! (current-module) ,texmacs-user) l))
    ) ;if
    ;; (display "loading ") (display name) (display "\n")
    `(begin (define-module ,name) ,@l)
  ) ;let
) ;define-macro

(define-public (module-available? module-name)
  (catch #t
    (lambda () (resolve-interface module-name) #t)
    (lambda (key . args) #f)
  ) ;catch
) ;define-public

(define-public (module-provide m)
  (if (not (module-available? m)) (module-load m))
) ;define-public
