
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : plugin-cmd.scm
;; DESCRIPTION : Commanding applications from TeXmacs and vice versa
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils plugins plugin-cmd) (:use (utils plugins plugin-eval)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; serialization
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define plugin-serializer (make-ahash-table))

(tm-define (pre-serialize lan t)
  (cond ((func? t 'document 1) (pre-serialize lan (cadr t)))
        ((func? t 'math 1)
         (pre-serialize lan (plugin-math-input (list 'tuple lan (cadr t))))
        ) ;
        (else t)
  ) ;cond
) ;tm-define

(define (hacked-texmacs->code x)
  (with r (texmacs->code x) (string-replace r "`" "`"))
) ;define

(tm-define (verbatim-serialize lan t)
  (with u
    (pre-serialize lan t)
    (string-append (escape-verbatim (hacked-texmacs->code u)) "\n")
  ) ;with
) ;tm-define

(tm-define (generic-serialize lan t)
  (with u
    (pre-serialize lan t)
    (string-append (char->string #\x02)
      "verbatim:"
      (escape-generic (texmacs->code u))
      (char->string #\x05)
    ) ;string-append
  ) ;with
) ;tm-define

(tm-define (plugin-serialize lan t)
  (with fun
    (ahash-ref plugin-serializer lan)
    (if fun (fun lan t) (verbatim-serialize lan t))
  ) ;with
) ;tm-define

(tm-define (plugin-serializer-set! lan val)
  (ahash-set! plugin-serializer lan val)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define plugin-commander (make-ahash-table))

(define (default-format-command s)
  (string-append (char->string #\x10) s "\n")
) ;define

(tm-define (format-command lan s)
  (with fun
    (ahash-ref plugin-commander lan)
    (if fun (fun s) (default-format-command s))
  ) ;with
) ;tm-define

(tm-define (plugin-commander-set! lan val)
  (ahash-set! plugin-commander lan val)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Some subroutines for mathematical content
;; FIXME: these should be moved into table-edit.scm and math-edit.scm
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (cell-context-inside-sub? t which)
  (or (and (list? which) (tree-in? t which))
    (and (nlist? which) (tree-is? t which))
    (and (tree-in? t '(table tformat document))
      (cell-context-inside-sub? (tree-up t) which)
    ) ;and
  ) ;or
) ;define

(define (cell-context-inside? t which)
  (and (tree-is? t 'cell)
    (tree-is? t :up 'row)
    (cell-context-inside-sub? (tree-ref t :up :up) which)
  ) ;and
) ;define

(tm-define (formula-context? t)
  (with u
    (tree-up t)
    (and u
      (or (tree-in? u '(math equation equation*))
        (match? u '(with "mode" "math" :%1))
        (cell-context-inside? u '(eqnarray eqnarray*))
      ) ;or
    ) ;and
  ) ;with
) ;tm-define

(tm-define (in-var-math?)
  (let* ((t1 (tree-innermost formula-context? #t)) (t2 (tree-innermost 'text)))
    (and (nnot t1) (or (not t2) (tree-inside? t1 t2)))
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; tab completion
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define plugin-supports-completions (make-ahash-table))

(tm-define (plugin-supports-completions-set! key)
  (ahash-set! plugin-supports-completions key #t)
) ;tm-define

(tm-define (plugin-supports-completions? key)
  (ahash-ref plugin-supports-completions key)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; testing whether more input is needed
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define plugin-supports-input-done (make-ahash-table))

(tm-define (plugin-supports-input-done-set! key)
  (ahash-set! plugin-supports-input-done key #t)
) ;tm-define

(tm-define (plugin-supports-input-done? key)
  (ahash-ref plugin-supports-input-done key)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Command for numeric evaluation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define plugin-approx-command (make-ahash-table))

(tm-define (plugin-approx-command-set! key val)
  (ahash-set! plugin-approx-command key val)
) ;tm-define

(tm-define (plugin-approx-command-ref key)
  (ahash-ref plugin-approx-command key)
) ;tm-define
