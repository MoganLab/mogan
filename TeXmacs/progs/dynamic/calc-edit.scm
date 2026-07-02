
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : calc-edit.scm
;; DESCRIPTION : low level linking and evaluation routines for spread sheets
;; COPYRIGHT   : (C) 2012  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (dynamic calc-edit)
  (:use (text text-drd)
    (text text-structure)
    (link locus-edit)
    (version version-edit)
    (dynamic session-edit)
    (dynamic scripts-edit)
    (dynamic calc-drd)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Spreadsheet evaluation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define calc-input (make-ahash-table))
(tm-define calc-output (make-ahash-table))
(tm-define calc-invalid (make-ahash-table))
(tm-define calc-todo (make-ahash-table))

(tm-define (calc-updated? t)
  (cond ((tree-atomic? t) #t)
        ((tree-is? t 'calc-ref)
         (with var (texmacs->string (tree-ref t 0)) (not (ahash-ref calc-invalid var)))
        ) ;
        (else (list-and (map calc-updated? (tree-children t))))
  ) ;cond
) ;tm-define

(tm-define (calc-get-input t) (tree-ref t 1))

(tm-define (calc-prefix-input prefix t)
  (cond ((tree-atomic? t) t)
        ((tree-is? t 'cell-ref)
         (with name
           (texmacs->string (tree-ref t 0))
           (tm->tree `(calc-ref ,(string-append prefix name)))
         ) ;with
        ) ;
        (else (tree-map-children (cut calc-prefix-input prefix <>) t))
  ) ;cond
) ;tm-define

(define (calc-var t prefix)
  (let* ((raw-var (texmacs->string (tree-ref t 0)))
         (pre (if (calc-cell-context? t) prefix ""))
         (var (string-append pre raw-var))
        ) ;
    var
  ) ;let*
) ;define

(tm-define (calc-update-inputs lan ses prefix t)
  (cond ((tree-atomic? t) (noop))
        ((calc-inert-context? t)
         (let* ((var (calc-var t prefix))
                (old-val (ahash-ref calc-output var))
                (new-val (tree->stree (tree-ref t 1)))
               ) ;
           (when (!= new-val old-val)
             (ahash-set! calc-output var new-val)
             (ahash-set! calc-invalid var #t)
           ) ;when
         ) ;let*
        ) ;
        ((or (calc-toggle-context? t)
           (calc-generate-context? t)
           (calc-answer-context? t)
           (tree-is? t 'calc-suggest)
         ) ;or
         (let* ((var (calc-var t prefix))
                (in-tree (calc-prefix-input prefix (calc-get-input t)))
                (in (tree->stree in-tree))
                (out-index (if (calc-answer-context? t) 3 2))
                (out-tree (tree-ref t out-index))
                (out (tree->stree out-tree))
                (todo (list lan ses var in-tree out-tree))
               ) ;
           (when (and (calc-generate-context? t) (!= out ""))
             (ahash-set! calc-input var in)
             (ahash-set! calc-output var out)
           ) ;when
           (when (or (!= (ahash-ref calc-input var) in)
                   (!= (ahash-ref calc-output var) out)
                   (not (calc-updated? in-tree))
                 ) ;or
             (ahash-set! calc-input var in)
             (ahash-set! calc-invalid var #t)
             (ahash-set! calc-todo var todo)
           ) ;when
         ) ;let*
        ) ;
        ((calc-check-context? t)
         (calc-update-inputs lan ses prefix (tree-ref t 4))
         (let* ((var (calc-var t prefix))
                (old-val (ahash-ref calc-output var))
                (new-val (tree->stree (tree-ref t 2)))
               ) ;
           (when (!= new-val old-val)
             (ahash-set! calc-output var new-val)
             (ahash-set! calc-invalid var #t)
           ) ;when
         ) ;let*
         (let* ((var (string-append (calc-var t prefix) "?"))
                (in-tree (calc-prefix-input prefix (calc-get-input t)))
                (in (tree->stree in-tree))
                (out-tree (tree-ref t 3))
                (out (tree->stree out-tree))
                (todo (list lan ses var in-tree out-tree))
               ) ;
           (when (or (!= (ahash-ref calc-input var) in)
                   (!= (ahash-ref calc-output var) out)
                   (not (calc-updated? in-tree))
                 ) ;or
             (ahash-set! calc-input var in)
             (ahash-set! calc-invalid var #t)
             (ahash-set! calc-todo var todo)
           ) ;when
         ) ;let*
        ) ;
        ((tree-is? t 'calc-table)
         (with x
           (texmacs->string (tree-ref t 0))
           (calc-update-inputs lan ses (string-append prefix x "-") (tree-ref t 1))
         ) ;with
        ) ;
        ((tree-is? t 'with)
         ;; FIXME: the test should be replaced by else,
         ;; but this is currently very slow
         (with update
           (lambda (i)
             (let* ((lan2 (tree-child-env t i "prog-scripts" lan))
                    (ses2 (tree-child-env t i "prog-session" ses))
                    (lan3 (texmacs->string lan2))
                    (ses3 (texmacs->string ses2))
                   ) ;
               (calc-update-inputs lan3 ses3 prefix (tree-ref t i))
             ) ;let*
           ) ;lambda
           (for-each update (.. 0 (tree-arity t)))
         ) ;with
        ) ;
        ((and (tree-is? t 'hidden) (tree-up t) (tree-is? (tree-up t) 'screens)) (noop))
        (else (for-each (cut calc-update-inputs lan ses prefix <>) (tree-children t)))
  ) ;cond
) ;tm-define

(tm-define (calc-repeat-update-inputs lan ses prefix t)
  (with n
    (ahash-size calc-invalid)
    ;; (display* "  Update inputs " n "\n")
    (calc-update-inputs lan ses prefix t)
    ;; (display* "  Update inputs " n " (bis)\n")
    (when (!= n (ahash-size calc-invalid))
      (calc-repeat-update-inputs lan ses prefix t)
    ) ;when
  ) ;with
) ;tm-define

(tm-define (calc-available? t)
  (cond ((tree-atomic? t) #t)
        ((tree-is? t 'calc-ref)
         (with var (texmacs->string (tree-ref t 0)) (not (ahash-ref calc-todo var)))
        ) ;
        (else (list-and (map calc-available? (tree-children t))))
  ) ;cond
) ;tm-define

(tm-define (calc-substitute t lan)
  (cond ((tree-atomic? t) t)
        ((tree-is? t 'calc-ref)
         (let* ((var (texmacs->string (tree-ref t 0))) (val (ahash-ref calc-output var)))
           (cond ((not val) t)
                 ((tm-equal? val "") (string->tree "0"))
                 ((== lan "scheme") (tm->tree val))
                 (else (tm->tree `(concat ,"(" ,val ,")")))
           ) ;cond
         ) ;let*
        ) ;
        (else (tree-map-children (cut calc-substitute <> lan) t))
  ) ;cond
) ;tm-define

(tm-define (calc-reevaluate-output lan ses var in out)
  (when (calc-available? in)
    ;; (display* "Reevaluate output " t "\n")
    ;; (display* "src= " (calc-substitute in lan) "\n")
    (let* ((src (tm->tree (calc-substitute in lan)))
           (dest (tm->tree (scheme-eval src :calc)))
          ) ;
      ;; (display* "var= " var "\n")
      ;; (display* "src= " src "\n")
      ;; (display* "dest= " dest "\n")
      (ahash-set! calc-output var dest)
      (ahash-remove! calc-todo var)
      (tree-set t 2 dest)
    ) ;let*
  ) ;when
) ;tm-define

(tm-define (calc-repeat-reevaluate-outputs)
  (with n
    (ahash-size calc-todo)
    (for (p (ahash-table->list calc-todo))
      (with (lan ses var in out) (cdr p) (calc-reevaluate-output lan ses var in out))
    ) ;for
    (when (!= n (ahash-size calc-todo))
      (calc-repeat-reevaluate-outputs)
    ) ;when
  ) ;with
) ;tm-define

(tm-define (calc-reevaluate lan ses prefix t)
  (calc-repeat-update-inputs lan ses prefix t)
  (calc-repeat-reevaluate-outputs)
  (set! calc-invalid (make-ahash-table))
  (set! calc-todo (make-ahash-table))
) ;tm-define

(tm-define (calc-scheme) (calc-reevaluate "scheme" "default" "" (buffer-tree)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Regenerate icourse exercises and display solutions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (exercise-context? t)
  (tree-in? t (exercise-tag-list))
) ;define

(define (math-context? t)
  (tree-in? t '(math equation equation* eqnarray eqnarray*))
) ;define

(define (icourse-invalidate t)
  (cond ((tree-in? t '(calc-generate calc-generate-command calc-suggest))
         (with var
           (calc-var t "")
           (tree-set (tree-ref t 2) "")
           (ahash-remove! calc-input var)
           (ahash-remove! calc-output var)
         ) ;with
        ) ;
        ((calc-answer-context? t) (tree-set (tree-ref t 2) ""))
        ((calc-check-context? t) (tree-set (tree-ref t 2) ""))
        ((tree-compound? t) (for-each icourse-invalidate (tree-children t)))
  ) ;cond
) ;define

(tm-define (calc-regenerate)
  (cond ((selection-active-any?)
         (with l (selection-trees) (for-each icourse-invalidate l))
        ) ;
        ((nnot (tree-innermost exercise-context?))
         (icourse-invalidate (tree-innermost exercise-context?))
        ) ;
        ((nnot (tree-innermost math-context?))
         (icourse-invalidate (tree-innermost math-context?))
        ) ;
        (else (icourse-invalidate (buffer-tree)))
  ) ;cond
  (calc)
) ;tm-define

(define (icourse-solutions t flag?)
  (cond ((calc-answer-context? t)
         (tree-set (tree-ref t 2) (if flag? (tree->stree (tree-ref t 3)) ""))
        ) ;
        ((calc-check-context? t)
         (tree-set (tree-ref t 2) (if flag? (tree->stree (tm-ref t 4 2)) ""))
        ) ;
        ((tree-compound? t)
         (for-each (cut icourse-solutions <> flag?) (tree-children t))
        ) ;
  ) ;cond
) ;define

(tm-define (calc-solutions flag?)
  (cond ((selection-active-any?)
         (with l (selection-trees) (for-each (cut icourse-solutions <> flag?) l))
        ) ;
        ((nnot (tree-innermost exercise-context?))
         (icourse-solutions (tree-innermost exercise-context?) flag?)
        ) ;
        ((nnot (tree-innermost math-context?))
         (icourse-solutions (tree-innermost math-context?) flag?)
        ) ;
        (else (icourse-solutions (buffer-tree) flag?))
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Continuous re-checking of answers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define check-busy? #f)

(define check-answer-table (make-ahash-table))

(tm-define (calc)
  (when (not check-busy?)
    ;; (display* "Recalculate\n")
    (set! check-busy? #t)
    (delayed (:idle 250) (calc-now) (set! check-busy? #f))
  ) ;when
) ;tm-define

(tm-define (calc-lazy-recheck v)
  (and-with t
    (tree-up v)
    (when (calc-check-context? t)
      (let* ((var (calc-var t "")) (answer (tree->stree (tree-ref t 2))))
        (when (not (ahash-ref check-answer-table var))
          (ahash-set! check-answer-table var answer)
        ) ;when
        (when (!= (ahash-ref check-answer-table var) answer)
          ;; (display* var " ~> " answer "\n")
          (ahash-set! check-answer-table var answer)
          (calc)
        ) ;when
      ) ;let*
    ) ;when
  ) ;and-with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Communication with the plug-in
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (calc-ready?)
  (let* ((lan (get-env "prog-scripts")) (ses (get-env "prog-session")))
    (and (connection-defined? lan) (scripts-defined? lan))
  ) ;let*
) ;tm-define

(tm-define (calc-feed lan ses var in out)
  (when (scripts-defined? lan)
    (tree-set! out '(script-busy))
    ;; (display* "Calc " var ", " lan ", " ses "\n")
    ;; (display* "Feed " in "\n")
    ;; (display* "Wait " out "\n")
    (with ptr
      (tree->tree-pointer out)
      (with ret
        (lambda (r)
          ;; (display* "r= " r "\n")
          (with check
            (tree-pointer->tree ptr)
            (tree-pointer-detach ptr)
            (when (== check out)
              (tree-set! out r)
              ;; (display* var " := " (tm->stree r) "\n")
              (ahash-set! calc-output var (tm->stree r))
              (ahash-remove! calc-todo var)
              (delayed (calc-continue))
            ) ;when
          ) ;with
        ) ;lambda
        ;; (display* var " := " (tm->stree in) " using " lan ", " ses "\n")
        (silent-feed* lan ses in ret '(:simplify-output))
      ) ;with
    ) ;with
  ) ;when
) ;tm-define

(tm-define (calc-continue-first l)
  (cond ((null? l)
         (set! calc-invalid (make-ahash-table))
         (set! calc-todo (make-ahash-table))
        ) ;
        ((with (lan ses var in out) (cdar l) (calc-available? in))
         (with (lan ses var in out)
           (cdar l)
           (with src (tm->tree (calc-substitute in lan)) (calc-feed lan ses var src out))
         ) ;with
        ) ;
        (else (calc-continue-first (cdr l)))
  ) ;cond
) ;tm-define

(tm-define (calc-continue)
  (with n
    (ahash-size calc-todo)
    (if (== n 0)
      (set! calc-invalid (make-ahash-table))
      (calc-continue-first (ahash-table->list calc-todo))
    ) ;if
  ) ;with
) ;tm-define

(tm-define (calc-now)
  (let* ((lan (get-init "prog-scripts")) (ses (get-init "prog-session")))
    ;; (display* "Dependencies...\n")
    (calc-repeat-update-inputs lan ses "" (buffer-tree))
    ;; (display* "Evaluating...\n")
    (calc-continue)
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keyboard interaction
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define calc-serial 0)

(define (calc-auto)
  (set! calc-serial (+ calc-serial 1))
  (string-append "field" (number->string calc-serial))
) ;define

(tm-define (make-calc-inert)
  (insert-go-to `(calc-inert ,(calc-auto) ,"") '(1 0))
) ;tm-define

(tm-define (make-calc-input)
  (insert-go-to `(calc-input ,(calc-auto) ,"" ,"") '(1 0))
) ;tm-define

(tm-define (make-calc-generate)
  (insert-go-to `(calc-generate-command ,(calc-auto) ,"" ,"") '(1 0))
) ;tm-define

(tm-define (make-calc-answer)
  (insert-go-to `(calc-answer-command ,(calc-auto) ,"" ,"" ,"") '(1 0))
) ;tm-define

(tm-define (make-calc-check)
  (with sug
    `(calc-suggest ,(calc-auto) ,"" ,"")
    (insert-go-to `(calc-check-predicate ,(calc-auto) ,"" ,"" ,"" ,sug) '(1 0))
  ) ;with
) ;tm-define

(tm-define (alternate-toggle t)
  (:require (tree-in? t '(calc-output calc-generate calc-answer calc-check)))
  (tree-assign-node! t (symbol-toggle-alternate (tree-label t)))
  (tree-go-to t 1 :end)
) ;tm-define

(tm-define (alternate-toggle t)
  (:require (tree-in? t
              '(calc-input calc-generate-command
                 calc-answer-command
                 calc-check-predicate)
            ) ;tree-in?
  ) ;:require
  (tree-assign-node! t (symbol-toggle-alternate (tree-label t)))
  (tree-go-to t 2 :end)
  (calc)
) ;tm-define

(tm-define (variant-circulate t forward?)
  (:require (calc-check-context? t))
  (cond ((or (and (tree-is? t 'calc-check-predicate) forward?)
           (and (tree-is? t 'calc-check) (not forward?))
         ) ;or
         (tree-assign-node! t 'calc-check-command)
         (tree-go-to t 4 2 :end)
        ) ;
        ((or (and (tree-is? t 'calc-check-command) forward?)
           (and (tree-is? t 'calc-check-predicate) (not forward?))
         ) ;or
         (tree-assign-node! t 'calc-check)
         (tree-go-to t 2 :end)
        ) ;
        ((or (and (tree-is? t 'calc-check) forward?)
           (and (tree-is? t 'calc-check-command) (not forward?))
         ) ;or
         (tree-assign-node! t 'calc-check-predicate)
         (tree-go-to t 1 :end)
        ) ;
  ) ;cond
) ;tm-define

(tm-define (kbd-enter t forwards?)
  (:require (and (calc-inert-context? t) (not (tree-is? t :up 'inactive))))
  (calc)
) ;tm-define

(tm-define (kbd-enter t forwards?)
  (:require (and (or (calc-toggle-context? t)
                   (calc-generate-context? t)
                   (calc-answer-context? t)
                 ) ;or
              (not (tree-is? t :up 'inactive))
            ) ;and
  ) ;:require
  (alternate-toggle t)
) ;tm-define

(tm-define (kbd-enter t forwards?)
  (:require (and (calc-check-context? t) (not (tree-is? t :up 'inactive))))
  (variant-circulate t #t)
) ;tm-define
