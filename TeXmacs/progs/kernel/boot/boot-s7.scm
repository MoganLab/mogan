
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : boot-s7.scm
;; DESCRIPTION : some global variables, public macros, on-entry, on-exit and
;;               initialization of the TeXmacs module system
;; COPYRIGHT   : (C) 2020  Massimiliano Gubinelli
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define has-look-and-feel? (lambda (x) (== x "emacs")))

(define list? proper-list?)

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
;; Modules
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;; setup the main modules
(with-let (rootlet)
  (define *texmacs-module* (rootlet))
  (define *module-name* '(texmacs))
  (define *modules* (make-hash-table))
) ;with-let

(define *texmacs-user-module* (curlet))
(set! *current-module* *texmacs-user-module*)
(set! *module-name* '(texmacs-user))

(define *exports* '())

(set! (*modules* '(texmacs)) *texmacs-module*)
(set! (*modules* '(texmacs-user)) *texmacs-user-module*)

(define (current-module)
  *current-module*
) ;define

(define-macro (export . symbols)
  `(set! *exports* (append (quote ,symbols) *exports*))
) ;define-macro

(define-macro (with-module module . body)
  `(let ((m ,module))
     (with-let m
       (let-temporarily (((*texmacs-module* '*current-module*) (curlet)))
         ,@body)))
) ;define-macro

(define-macro (define-public head . body)
  `(begin (define ,head ,@body) (export ,(if (pair? head) (car head) head)))
) ;define-macro


(define-macro (provide-public head . body)
  (if (or (and (symbol? head) (not (defined? head)))
        (and (pair? head) (symbol? (car head)) (not (defined? (car head))))
      ) ;or
    `(define-public ,head ,@body)
    '(noop)
  ) ;if
) ;define-macro

(define-macro (define-public-macro head . body)
  `(begin
     (define-macro ,head ,@body)
     (export ,(if (pair? head) (car head) head)))
) ;define-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Module handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (module-available? module)
  (if (hash-table-ref *modules* module) #t #f)
) ;define

(define (list->module module)
  (let* ((aux (lambda (s) (string-append "/" (symbol->string s))))
         (name* (apply string-append (map aux module)))
         (name (substring name* 1 (string-length name*)))
         (u (url-unix "$GUILE_LOAD_PATH" (string-append name ".scm")))
         ;; FIXME: should use %load-path instead of $GUILE_LOAD_PATH
        ) ;
    (url-materialize u "r")
  ) ;let*
) ;define

(define (module-load module)
  (if (list? module)
    (let ((module-file (list->module module)) (loaded (hash-table-ref *modules* module)))
      (when (not loaded)
        ;; (display "TeXmacs] Loading module ") (display module) (display "\n")
        (with-module (sublet (hash-table-ref *modules* '(texmacs-user))
                       '*exports*
                       ()
                       '*module-file*
                       module-file
                     ) ;sublet
          (load *module-file* (curlet))
        ) ;with-module
      ) ;when
    ) ;let
  ) ;if
) ;define

(define (module-provide module)
  (if (not (module-available? module)) (module-load module))
) ;define

(define (resolve-module module)
  (module-provide module)
  (hash-table-ref *modules* module)
) ;define

(define-macro (use-modules . modules)
  `(map (lambda (module)
          (let* ((m (resolve-module module))
                 (ex (m '*exports*))
                 (exx (map (lambda (entry)
                             (if (member (car entry) ex) entry (values)))
                        m))
                 (en (apply inlet exx)))
            (varlet (*texmacs-module* '*current-module*) en)))
     (quote ,modules))
) ;define-macro

(define-macro (import-from . modules) `(use-modules ,@modules))

(define-macro (re-export . symbols) `(export ,@symbols))

(define-macro (inherit-modules . which-list)
  (define (module-exports which)
    (let* ((m (resolve-module which)))
      (m '*exports*)
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
  (let ((l (map transform options)))
    ;; (display "loading ") (display name) (display "\n")
    `(begin
       (define *module-name* (quote ,name))
       (define *exports* ())
       (hash-table-set! *modules* (quote ,name) (current-module))
       ,@l)
  ) ;let
) ;define-macro

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

(set! *load-path*
  (cons (string-append (url->system (get-texmacs-path)) "/plugins/goldfish/goldfish")
    *load-path*
  ) ;cons
) ;set!

(load "scheme/boot.scm")
(import (scheme base))
