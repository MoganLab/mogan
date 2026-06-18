
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : edu-edit.scm
;; DESCRIPTION : editing routines for educational purposes
;; COPYRIGHT   : (C) 2019  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (education edu-edit)
  (:use (dynamic fold-edit) (education edu-drd))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Context predicates
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (problem-context? t)
  (in? (tree-label t) (numbered-unnumbered-append (exercise-tag-list)))
) ;tm-define

(tm-define (solution-context? t)
  (in? (tree-label t) (numbered-unnumbered-append (solution-tag-list)))
) ;tm-define

(tm-define (short-question-context? t) (short-question-tag? (tree-label t)))

(tm-define (short-answer-context? t) (short-answer-tag? (tree-label t)))

(tm-define (short-question-or-answer-context? t)
  (or (short-question-context? t) (short-answer-context? t))
) ;tm-define

(tm-define (question-context? t)
  (or (problem-context? t) (short-question-context? t))
) ;tm-define

(tm-define (answer-context? t)
  (or (solution-context? t) (short-answer-context? t))
) ;tm-define

(tm-define (question-or-answer-context? t)
  (or (question-context? t) (answer-context? t))
) ;tm-define

(tm-define (question-context*? t)
  (or (question-context? t)
    (and (tree-func? t 'document 1) (question-context? (tree-ref t 0)))
  ) ;or
) ;tm-define

(tm-define (answer-context*? t)
  (or (answer-context? t)
    (and (tree-func? t 'document 1) (answer-context? (tree-ref t 0)))
  ) ;or
) ;tm-define

(tm-define (question-answer-context? t)
  (and (tree-in? t '(folded unfolded folded-reverse unfolded-reverse))
    (question-context*? (tree-ref t 0))
    (answer-context*? (tree-ref t 1))
  ) ;and
) ;tm-define

(tm-define (mc-field-context? t) (tree-is? t 'mc-field))

(tm-define (mc-context? t) (mc-tag? (tree-label t)))

(tm-define (mc-exposed-context? t) (mc-exposed-tag? (tree-label t)))

(tm-define (mc-popup-context? t) (mc-popup-tag? (tree-label t)))

(tm-define (mc-exclusive-context? t) (mc-exclusive-tag? (tree-label t)))

(tm-define (mc-plural-context? t) (mc-plural-tag? (tree-label t)))

(tm-define (with-button-context? t) (with-button-tag? (tree-label t)))

(tm-define (gap-context? t) (gap-tag? (tree-label t)))

(tm-define (gap-non-long-context? t)
  (or (gap-short-tag? (tree-label t)) (gap-wide-tag? (tree-label t)))
) ;tm-define

(tm-define (gap-long-context? t) (gap-long-tag? (tree-label t)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Operating on a tree
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (count doc)
  (cond ((or (tm-func? doc 'document) (tm-func? doc 'table))
         (apply + (map count (tm-children doc)))
        ) ;
        ((tm-compound? doc) (apply max (cons 1 (map count (tm-children doc)))))
        (else 1)
  ) ;cond
) ;define

(define (empty doc)
  `(document ,@(map (lambda (x) "") (.. 0 (count doc))))
) ;define

(define (edu-operate-document l mode)
  (cond ((null? l) (noop))
        ((and (nnull? (cdr l)) (question-context? (car l)) (answer-context? (cadr l)))
         (edu-operate-document (cddr l) mode)
         (and-let* ((que (car l))
                    (ans (cadr l))
                    (tag (cond ((== mode :question) 'folded)
                               ((== mode :answer) 'folded-reverse)
                               ((== mode :mixed) 'unfolded)
                               (else #f)
                         ) ;cond
                    ) ;tag
                   ) ;
           (tree-insert-node! que 0 (list tag))
           (tree-insert-node! ans 0 (list tag))
           (tree-join (tree-up que) (tree-index que))
         ) ;and-let*
        ) ;
        (else (edu-operate-document (cdr l) mode) (edu-operate (car l) mode))
  ) ;cond
) ;define

(tm-define (edu-operate t mode)
  (when (tree-compound? t)
    (if (tree-is? t 'document)
      (edu-operate-document (tree-children t) mode)
      (for-each (cut edu-operate <> mode) (tree-children t))
    ) ;if
    (when (question-answer-context? t)
      (cond ((== mode :mixed) (alternate-unfold t))
            ((and (== mode :question) (tree-is? t 'unfolded)) (alternate-fold t))
            ((and (== mode :answer) (tree-is? t 'unfolded-reverse)) (alternate-fold t))
            ((and (== mode :question) (tree-in? t '(folded-reverse unfolded-reverse)))
             (variant-set t 'folded)
            ) ;
            ((and (== mode :answer) (not (tree-in? t '(folded-reverse unfolded-reverse))))
             (variant-set t 'folded-reverse)
            ) ;
      ) ;cond
    ) ;when
    (when (mc-field-context? t)
      (with c
        (tree-ref t 0)
        (cond ((not (tree-in? c '(hide-simple show-simple)))
               (if (== mode :question) (tree-set! c `(hide-simple ,"false" ,c)))
              ) ;
              ((== mode :question)
               (when (not (tree-is? c 'hide-simple))
                 (variant-set c 'hide-simple)
               ) ;when
               (tree-set (tree-ref c 0) "false")
              ) ;
              ((!= mode :question) (tree-set! c (tree-ref c 1)))
        ) ;cond
      ) ;with
    ) ;when
    (when (gap-long-context? t)
      (with c
        (tree-ref t 0)
        (cond ((not (tree-in? c '(hide-simple show-simple)))
               (if (== mode :question) (tree-set! c `(hide-simple ,(empty c) ,c)))
              ) ;
              ((== mode :question)
               (when (not (tree-is? c 'hide-simple))
                 (variant-set c 'hide-simple)
               ) ;when
               (tree-set (tree-ref c 0) (empty (tree-ref c 1)))
              ) ;
              ((!= mode :question) (tree-set! c (tree-ref c 1)))
        ) ;cond
      ) ;with
    ) ;when
    (when (and (gap-context? t) (not (gap-long-context? t)))
      (with c
        (tree-ref t 0)
        (cond ((not (tree-in? c '(hide-reply show-reply)))
               (if (== mode :question) (tree-set! c `(hide-reply ,"" ,c)))
              ) ;
              ((== mode :question)
               (when (not (tree-is? c 'hide-reply))
                 (variant-set c 'hide-reply)
               ) ;when
               (tree-set (tree-ref c 0) "")
              ) ;
              ((!= mode :question) (tree-set! c (tree-ref c 1)))
        ) ;cond
      ) ;with
    ) ;when
  ) ;when
) ;tm-define

(tm-define (edu-set-mode mode) (edu-operate (buffer-tree) mode))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Questions and answers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-enter t shift?)
  (:require (and (short-question-or-answer-context? t) (not shift?)))
  (cond ((question-answer-context? (tree-up t))
         (let* ((f (tree-up t)) (q (tree-ref f 0)))
           (if (tree-func? q 'document 1) (set! q (tree-ref q 0)))
           (with l (tree-label q) (tree-go-to f :end) (make l))
         ) ;let*
        ) ;
        ((question-answer-context? (tree-up (tree-up t)))
         (let* ((f (tree-up (tree-up t))) (q (tree-ref f 0)))
           (if (tree-func? q 'document 1) (set! q (tree-ref q 0)))
           (with l (tree-label q) (tree-go-to f :end) (make l))
         ) ;let*
        ) ;
        (else (with l (tree-label t) (tree-go-to t :end) (make l)))
  ) ;cond
) ;tm-define

(tm-define (unanswered-question-context? t)
  (and (question-context? t)
    (tree-is? t :up 'document)
    (not (toggle-context? (tree-up (tree-up t))))
  ) ;and
) ;tm-define

(tm-define (alternate-toggle t)
  (:require (and (unanswered-question-context? t) (in-edu-text?)))
  (let* ((p (tree->path t))
         (a (cond ((tree-in? t '(exercise exercise* problem problem*)) 'solution*)
                  ((tree-in? t '(question question*)) 'answer*)
                  ((short-question-context? t) 'answer-item)
            ) ;cond
         ) ;a
        ) ;
    (tree-set! t `(unfolded ,t (,a (document ""))))
    (go-to (append p (list 1 0 0 0)))
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Multiple choice lists
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (make-mc env)
  (insert-go-to `(document (,env (mc-field "false" ""))) '(0 0 1 0))
) ;tm-define

(tm-define (make tag . opt-arity)
  (if (in? tag (mc-tag-list)) (make-mc tag) (apply former (cons tag opt-arity)))
) ;tm-define

(define (mc-test-select? plural?)
  (with-innermost t mc-context? (and t (xor plural? (mc-exclusive-context? t))))
) ;define

(tm-define (mc-select plural?)
  (:check-mark "*" mc-test-select?)
  (with-innermost t
    mc-context?
    (cond ((and plural? (mc-exclusive-context? t)) (alternate-toggle t))
          ((and (not plural?) (mc-plural-context? t))
           (clear-buttons t)
           (alternate-toggle t)
          ) ;
    ) ;cond
  ) ;with-innermost
) ;tm-define

(tm-define (mc-get-button-theme)
  (with t (tree-innermost with-button-context?) (and t (tree-label t)))
) ;tm-define

(tm-define (mc-get-pretty-button-theme)
  (with th
    (mc-get-button-theme)
    (cond ((== th #f) "Default")
          ((== th 'with-button-box) "Plain boxes")
          ((== th 'with-button-box*) "Crossed boxes")
          ((== th 'with-button-circle) "Plain circles")
          ((== th 'with-button-circle*) "Crossed circles")
          ((== th 'with-button-arabic) "1, 2, 3")
          ((== th 'with-button-alpha) "a, b, c")
          ((== th 'with-button-Alpha) "A, B, C")
          ((== th 'with-button-roman) "i, ii, iii")
          ((== th 'with-button-Roman) "I, II, III")
          ((== th 'with-button-ornament) "Wide colored")
          (else "Unknown")
    ) ;cond
  ) ;with
) ;tm-define

(define (mc-test-button-theme? th)
  (if (and (list-2? th) (== (car th) 'quote)) (set! th (cadr th)))
  (== (mc-get-button-theme) th)
) ;define

(tm-define (mc-set-button-theme th)
  (:check-mark "*" mc-test-button-theme?)
  (with t
    (tree-innermost with-button-context?)
    (cond ((and t th) (tree-assign-node! t th))
          ((and t (not th))
           (tree-remove-node! t 0)
           (when (tree-func? t 'document 1)
             (tree-remove-node! t 0)
           ) ;when
          ) ;
          ((and (not t) th) (with-innermost mc mc-context? (tree-set! mc `(,th
                                                                           ,mc))))
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (customizable-parameters t)
  (:require (mc-popup-context? t))
  (list (list "button-popup-activate" "Activate"))
) ;tm-define

(tm-define (parameter-choice-list var)
  (:require (in? var (list "button-popup-activate")))
  (list "click" "mouse-over" "focus")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Editing entries of multiple choice lists
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (structured-horizontal? t) (:require (mc-context? t)) #t)

(tm-define (focus-can-insert? t) (:require (mc-context? t)) #t)

(tm-define (focus-can-remove? t) (:require (mc-context? t)) #t)

(tm-define (mc-field-active? h)
  (and (tm-func? h 'mc-field 2)
    (or (tm-equal? (tm-ref h 0) "true")
      (and (tm-func? (tm-ref h 0) 'hide-simple 2) (tm-equal? (tm-ref h 0 0) "true"))
    ) ;or
  ) ;and
) ;tm-define

(tm-define (mc-field-set h val)
  (when (tm-func? h 'mc-field 2)
    (perform-set (tm-ref h 0) val)
  ) ;when
) ;tm-define

(tm-define (mc-switch t i)
  (when (mc-context? t)
    (let* ((p (tree->path t)) (c (tree-down-index t)))
      (if (== i :first) (set! i 0))
      (if (== i :previous) (set! i (max 0 (- c 1))))
      (if (== i :this) (set! i c))
      (if (== i :next) (set! i (min (+ c 1) (- (tm-arity t) 1))))
      (if (== i :last) (set! i (- (tm-arity t) 1)))
      (when (tm-ref t i)
        (clear-buttons t)
        (mc-field-set (tm-ref t i) "true")
        (tree-go-to t i 1 :end)
      ) ;when
    ) ;let*
  ) ;when
) ;tm-define

(define (mc-visible-sub l)
  (cond ((null? l) (noop))
        ((mc-field-active? (car l))
         (when (not (cursor-inside? (tm-ref (car l) 1)))
           (tree-go-to (car l) 1 :end)
         ) ;when
        ) ;
        (else (mc-visible-sub (cdr l)))
  ) ;cond
) ;define

(define (mc-visible-cursor)
  (and-with t
    (tree-innermost mc-popup-context?)
    (mc-visible-sub (tree-children t))
  ) ;and-with
) ;define

(define (insert-mc-field t forwards?)
  (let* ((p (tree->path t)) (i (tree-down-index t)) (d (if forwards? 1 0)))
    (cond ((mc-popup-context? t)
           (mc-field-set (tm-ref t i) "false")
           (tree-insert! t (+ i d) '((mc-field "true" "")))
          ) ;
          (else (tree-insert! t (+ i d) '((mc-field "false" ""))))
    ) ;cond
    (go-to (append p (list (+ i d) 1 0)))
  ) ;let*
) ;define

(tm-define (kbd-enter t shift?)
  (:require (mc-context? t))
  (if shift? (former t shift?) (insert-mc-field t #t))
) ;tm-define

(tm-define (structured-insert-horizontal t forwards?)
  (:require (mc-context? t))
  (insert-mc-field t forwards?)
) ;tm-define

(tm-define (structured-insert-vertical t downwards?)
  (:require (mc-context? t))
  (insert-mc-field t downwards?)
) ;tm-define

(define (remove-mc-field t forwards? structured?)
  (let* ((i (tree-down-index t)) (n (tree-arity t)))
    (cond ((> n 1)
           (cond ((and structured? (not forwards?) (> i 0)) (set! i (- i 1)))
                 ((and forwards? (< i (- n 1))) (tree-go-to t (+ i 1) :start))
                 ((and forwards? (== i (- n 1))) (tree-go-to t (+ i -) :end))
                 ((and (not forwards?) (> i 0)) (tree-go-to t (- i 1) :end))
                 ((and (not forwards?) (== i 0)) (tree-go-to t (+ i 1) :start))
           ) ;cond
           (tree-remove t i 1)
           (when (mc-popup-context? t)
             (mc-switch t :this)
           ) ;when
          ) ;
          ((with-button-context? (tree-up t)) (tree-cut (tree-up t)))
          (else (tree-cut t))
    ) ;cond
  ) ;let*
) ;define

(tm-define (kbd-backspace)
  (:require (and (== (cursor-tree) (tree ""))
              (tree-is? (tree-up (cursor-tree)) 'mc-field)
              (mc-context? (tree-up (tree-up (cursor-tree))))
            ) ;and
  ) ;:require
  (remove-mc-field (tree-up (tree-up (cursor-tree))) #f #f)
) ;tm-define

(tm-define (kbd-delete)
  (:require (and (== (cursor-tree) (tree ""))
              (tree-is? (tree-up (cursor-tree)) 'mc-field)
              (mc-context? (tree-up (tree-up (cursor-tree))))
            ) ;and
  ) ;:require
  (remove-mc-field (tree-up (tree-up (cursor-tree))) #t #f)
) ;tm-define

(tm-define (structured-remove-horizontal t forwards?)
  (:require (mc-context? t))
  (remove-mc-field t forwards? #t)
) ;tm-define

(tm-define (structured-remove-vertical t downwards?)
  (:require (mc-context? t))
  (remove-mc-field t downwards? #t)
) ;tm-define

(tm-define (kbd-incremental t down?)
  (:require (mc-context? t))
  (mc-switch t (if down? :next :previous))
) ;tm-define

(tm-define (kbd-extremal t last?)
  (:require (mc-context? t))
  (mc-switch t (if last? :last :first))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Scripts attached to input fields
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (edu-exec val cmd)
  (and-let* ((cmd-t (tm? cmd))
             (cmd-s (tm->stree cmd))
             (cmd-b (string? cmd-s))
             (cmd-o (string->object cmd-s))
            ) ;
    (delayed (:idle 1) (secure-eval `(with answer (quote ,val) ,cmd-o)))
  ) ;and-let*
) ;define

(define (mc-exec t cmd)
  (let* ((c (tm-children t))
         (f (list-filter c mc-field-active?))
         (v (map (lambda (x) (tm->stree (tm-ref x 1))) f))
        ) ;
    (when (mc-exclusive-context? t)
      (set! v (and (nnull? v) (car v)))
    ) ;when
    (edu-exec v cmd)
  ) ;let*
) ;define

(define (button-exec t cmd)
  (if (and-with p
        (tm-ref t :up)
        (and-with pp (tm-ref p :up) (and (tm-is? p 'mc-field) (mc-context? pp)))
      ) ;and-with
    (mc-exec (tm-ref t :up :up) cmd)
    (edu-exec (not (tm-equal? t "false")) cmd)
  ) ;if
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Toggling buttons
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (perform-set t val)
  (cond ((tree-is? t 'hide-simple) (perform-set (tree-ref t 0) val))
        ((tree-is? t 'show-simple) (perform-set (tree-ref t 1) val))
        (else (tree-set t (tree val)))
  ) ;cond
) ;define

(define (clear-buttons t)
  (cond ((tree-func? t 'mc-field 2) (perform-set (tree-ref t 0) "false"))
        ((tree-atomic? t) (noop))
        (else (for-each clear-buttons (tree-children t)))
  ) ;cond
) ;define

(define (handle-exclusive p i)
  (with t
    (path->tree p)
    (cond ((tree-atomic? t) (handle-exclusive (cDr p) (cAr p)))
          ((tree-is? t 'mc-field) (handle-exclusive (cDr p) (cAr p)))
          ((mc-exclusive-context? t)
           (let* ((l (tree-children t))
                  (n (tree-arity t))
                  (x (append (sublist l 0 i) (sublist l (+ i 1) n)))
                 ) ;
             (for-each clear-buttons x)
           ) ;let*
          ) ;
    ) ;cond
  ) ;with
) ;define

(define (perform-toggle t)
  (cond ((tree-is? t 'hide-simple) (perform-toggle (tree-ref t 0)) t)
        ((tree-is? t 'show-simple) (perform-toggle (tree-ref t 1)) t)
        ((tm-equal? t "true") (tree-set! t "false") t)
        ((tm-equal? t "false") (tree-set! t "true") t)
        (else t)
  ) ;cond
) ;define

(tm-define (mouse-toggle-button t cmd)
  (:type (-> void))
  (:synopsis "Toggle a button using the mouse")
  (:secure #t)
  (if (tree->path t) (handle-exclusive (tree->path t) #f))
  (set! t (perform-toggle t))
  (mc-visible-cursor)
  (button-exec t cmd)
) ;tm-define

(tm-define (popup-toggle-button type x y t cmd)
  (:secure #t)
  (when (== (tm->stree type) "select")
    (mouse-toggle-button t cmd)
    (close-tooltip)
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Confirming gap input
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (gap-exec t)
  (and-let* ((p (tree->path t :start))
             (cmd (get-env-tree-at "attached-script" p))
             (c (tm-ref t 0))
            ) ;
    (when (tm-func? c 'hide-simple 2)
      (set! c (tm-ref c 1))
    ) ;when
    (edu-exec (tm->stree c) cmd)
  ) ;and-let*
) ;define

(tm-define (kbd-enter t shift?)
  (:require (gap-non-long-context? t))
  (gap-exec t)
) ;tm-define

(tm-define (kbd-control-enter t shift?)
  (:require (gap-context? t))
  (gap-exec t)
) ;tm-define
