(define-library (liii case)
  (import (liii base))
  (export case*)
  (begin

    (define case*
      (let ((case*-labels (lambda (label)
                            (let ((labels ((funclet ((funclet 'case*) 'case*-helper)
                                           ) ;funclet
                                           'labels
                                          ) ;
                                  ) ;labels
                                 ) ;
                              (labels (symbol->string label))
                            ) ;let
                          ) ;lambda
            ) ;case*-labels

            (case*-match? (lambda* (matchee pattern (e (curlet)))
                            (let ((matcher ((funclet ((funclet 'case*) 'case*-helper)
                                            ) ;funclet
                                            'handle-sequence
                                           ) ;
                                  ) ;matcher
                                 ) ;
                              (or (equivalent? matchee pattern)
                                (and (or (pair? matchee) (vector? matchee))
                                  (begin
                                    (fill! ((funclet ((funclet 'case*) 'case*-helper)
                                            ) ;funclet
                                            'labels
                                           ) ;
                                      #f
                                    ) ;fill!
                                    ((matcher pattern e) matchee)
                                  ) ;begin
                                ) ;and
                              ) ;or
                            ) ;let
                          ) ;lambda*
            ) ;case*-match?
            (case*-helper (with-let (unlet)
                            (define labels (make-hash-table))

                            (define (ellipsis? pat)
                              (and (undefined? pat)
                                (or (equal? pat #<...>)
                                  (let ((str (object->string pat)))
                                    (and (char-position #\: str)
                                      (string=? "...>"
                                        (substring str (- (length str) 4))
                                      ) ;string=?
                                    ) ;and
                                  ) ;let
                                ) ;or
                              ) ;and
                            ) ;define

                            (define (ellipsis-pair-position pos pat)
                              (and (pair? pat)
                                (if (ellipsis? (car pat))
                                  pos
                                  (ellipsis-pair-position (+ pos 1)
                                    (cdr pat)
                                  ) ;ellipsis-pair-position
                                ) ;if
                              ) ;and
                            ) ;define

                            (define (ellipsis-vector-position pat vlen)
                              (let loop
                                ((pos 0))
                                (and (< pos vlen)
                                  (if (ellipsis? (pat pos))
                                    pos
                                    (loop (+ pos 1))
                                  ) ;if
                                ) ;and
                              ) ;let
                            ) ;define

                            (define (splice-out-ellipsis sel pat pos e)
                              (let ((sel-len (length sel))
                                    (new-pat-len (- (length pat) 1))
                                    (ellipsis-label (and (not (eq? (pat pos) #<...>))
                                                      (let* ((str (object->string (pat pos)))
                                                             (colon (char-position #\: str))
                                                            ) ;
                                                        (and colon (substring str 2 colon))
                                                      ) ;let*
                                                    ) ;and
                                    ) ;ellipsis-label
                                   ) ;
                                (let ((func (and (string? ellipsis-label)
                                              (let ((comma (char-position #\, ellipsis-label)
                                                    ) ;comma
                                                   ) ;
                                                (and comma
                                                  (let ((str (substring ellipsis-label (+ comma 1))
                                                        ) ;str
                                                       ) ;
                                                    (set! ellipsis-label
                                                      (substring ellipsis-label 0 comma)
                                                    ) ;set!
                                                    (let ((func-val (symbol->value (string->symbol str) e)
                                                          ) ;func-val
                                                         ) ;
                                                      (if (undefined? func-val)
                                                        (error 'unbound-variable
                                                          "function ~S is undefined\n"
                                                          func
                                                        ) ;error
                                                      ) ;if
                                                      (if (not (procedure? func-val))
                                                        (error 'wrong-type-arg
                                                          "~S is not a function\n"
                                                          func
                                                        ) ;error
                                                      ) ;if
                                                      func-val
                                                    ) ;let
                                                  ) ;let
                                                ) ;and
                                              ) ;let
                                            ) ;and
                                      ) ;func
                                     ) ;
                                  (if (pair? pat)
                                    (cond ((= pos 0)
                                           (if ellipsis-label
                                             (set! (labels ellipsis-label)
                                               (list 'quote
                                                 (copy sel
                                                   (make-list (- sel-len new-pat-len))
                                                 ) ;copy
                                               ) ;list
                                             ) ;set!
                                           ) ;if
                                           (values (list-tail sel (- sel-len new-pat-len))
                                             (cdr pat)
                                             (or (not func)
                                               (func (cadr (labels ellipsis-label)))
                                             ) ;or
                                           ) ;values
                                          ) ;

                                          ((= pos new-pat-len)
                                           (if ellipsis-label
                                             (set! (labels ellipsis-label)
                                               (list 'quote
                                                 (copy sel
                                                   (make-list (- sel-len pos))
                                                   pos
                                                 ) ;copy
                                               ) ;list
                                             ) ;set!
                                           ) ;if
                                           (values (copy sel (make-list pos))
                                             (copy pat (make-list pos))
                                             (or (not func)
                                               (func (cadr (labels ellipsis-label)))
                                             ) ;or
                                           ) ;values
                                          ) ;

                                          (else (let ((new-pat (make-list new-pat-len))
                                                      (new-sel (make-list new-pat-len))
                                                     ) ;
                                                  (if ellipsis-label
                                                    (set! (labels ellipsis-label)
                                                      (list 'quote
                                                        (copy sel
                                                          (make-list (- sel-len new-pat-len))
                                                          pos
                                                        ) ;copy
                                                      ) ;list
                                                    ) ;set!
                                                  ) ;if
                                                  (copy pat new-pat 0 pos)
                                                  (copy pat
                                                    (list-tail new-pat pos)
                                                    (+ pos 1)
                                                  ) ;copy
                                                  (copy sel new-sel 0 pos)
                                                  (copy sel
                                                    (list-tail new-sel pos)
                                                    (- sel-len pos)
                                                  ) ;copy
                                                  (values new-sel
                                                    new-pat
                                                    (or (not func)
                                                      (func (cadr (labels ellipsis-label)))
                                                    ) ;or
                                                  ) ;values
                                                ) ;let
                                          ) ;else
                                    ) ;cond

                                    (cond ((= pos 0)
                                           (if ellipsis-label
                                             (set! (labels ellipsis-label)
                                               (list 'quote
                                                 (copy sel
                                                   (make-list (- sel-len new-pat-len))
                                                 ) ;copy
                                               ) ;list
                                             ) ;set!
                                           ) ;if
                                           (values (subvector sel
                                                     (max 0 (- sel-len new-pat-len))
                                                     sel-len
                                                   ) ;subvector
                                             (subvector pat 1 (+ new-pat-len 1))
                                             (or (not func)
                                               (func (cadr (labels ellipsis-label)))
                                             ) ;or
                                           ) ;values
                                          ) ;

                                          ((= pos new-pat-len)
                                           (if ellipsis-label
                                             (set! (labels ellipsis-label)
                                               (list 'quote
                                                 (copy sel
                                                   (make-list (- sel-len new-pat-len))
                                                   pos
                                                 ) ;copy
                                               ) ;list
                                             ) ;set!
                                           ) ;if
                                           (values (subvector sel 0 new-pat-len)
                                             (subvector pat 0 new-pat-len)
                                             (or (not func)
                                               (func (cadr (labels ellipsis-label)))
                                             ) ;or
                                           ) ;values
                                          ) ;

                                          (else (let ((new-pat (make-vector new-pat-len))
                                                      (new-sel (make-vector new-pat-len))
                                                     ) ;
                                                  (if ellipsis-label
                                                    (set! (labels ellipsis-label)
                                                      (list 'quote
                                                        (copy sel
                                                          (make-list (- sel-len new-pat-len))
                                                          pos
                                                        ) ;copy
                                                      ) ;list
                                                    ) ;set!
                                                  ) ;if
                                                  (copy pat new-pat 0 pos)
                                                  (copy pat
                                                    (subvector new-pat pos new-pat-len)
                                                    (+ pos 1)
                                                  ) ;copy
                                                  (copy sel new-sel 0 pos)
                                                  (copy sel
                                                    (subvector new-sel pos new-pat-len)
                                                    (- sel-len pos)
                                                  ) ;copy
                                                  (values new-sel
                                                    new-pat
                                                    (or (not func)
                                                      (cadr (func (labels ellipsis-label)))
                                                    ) ;or
                                                  ) ;values
                                                ) ;let
                                          ) ;else
                                    ) ;cond
                                  ) ;if
                                ) ;let
                              ) ;let
                            ) ;define

                            (define (handle-regex x)
                              #f
                            ) ;define

                            (define (undefined->function undef e)
                              (let* ((str1 (object->string undef))
                                     (str1-end (- (length str1) 1))
                                    ) ;
                                (if (not (char=? (str1 str1-end) #\>))
                                  (error 'wrong-type-arg
                                    "pattern descriptor does not end in '>': ~S\n"
                                    str1
                                  ) ;error
                                ) ;if
                                (let ((str (substring str1 2 str1-end)))
                                  (if (= (length str) 0)
                                    (lambda (x) #t)
                                    (let ((colon (char-position #\: str)))
                                      (cond (colon (let ((label (substring str 0 colon))
                                                         (func (substring str (+ colon 1)))
                                                        ) ;
                                                     (cond ((labels label)
                                                            (lambda (sel)
                                                              (error 'syntax-error
                                                                "label ~S is defined twice: old: ~S, new: ~S~%"
                                                                label
                                                                (labels label)
                                                                sel
                                                              ) ;error
                                                            ) ;lambda
                                                           ) ;

                                                           ;; otherwise the returned function needs to store the current sel-item under label in labels
                                                           ((zero? (length func))
                                                            (lambda (x) (set! (labels label) x) #t)
                                                           ) ;

                                                           ((char=? (func 0) #\")
                                                            (lambda (x)
                                                              (set! (labels label) x)
                                                              (handle-regex func)
                                                            ) ;lambda
                                                           ) ;
                                                           (else (let ((func-val (symbol->value (string->symbol func) e)
                                                                       ) ;func-val
                                                                      ) ;
                                                                   (if (undefined? func-val)
                                                                     (error 'unbound-variable
                                                                       "function ~S is undefined\n"
                                                                       func
                                                                     ) ;error
                                                                     (if (not (procedure? func-val))
                                                                       (error 'wrong-type-arg
                                                                         "~S is not a function\n"
                                                                         func
                                                                       ) ;error
                                                                       (lambda (x)
                                                                         (set! (labels label) x)
                                                                         (func-val x)
                                                                       ) ;lambda
                                                                     ) ;if
                                                                   ) ;if
                                                                 ) ;let
                                                           ) ;else
                                                     ) ;cond
                                                   ) ;let
                                            ) ;colon
                                            ((char=? (str 0) #\")
                                             (handle-regex str)
                                            ) ;
                                            (else (let ((saved (labels str)))
                                                    (if saved
                                                      (lambda (x) (equivalent? x saved))
                                                      (symbol->value (string->symbol str) e)
                                                    ) ;if
                                                  ) ;let
                                            ) ;else
                                      ) ;cond
                                    ) ;let
                                  ) ;if
                                ) ;let
                              ) ;let*
                            ) ;define
                            (define (handle-pattern sel-item pat-item e)
                              (and (undefined? pat-item)
                                (not (eq? pat-item #<undefined>))
                                (let ((func (undefined->function pat-item e))
                                     ) ;
                                  (if (undefined? func)
                                    (error 'unbound-variable
                                      "function ~S is undefined\n"
                                      pat-item
                                    ) ;error
                                  ) ;if
                                  (if (not (procedure? func))
                                    (error 'wrong-type-arg
                                      "~S is not a function\n"
                                      func
                                    ) ;error
                                  ) ;if
                                  (func sel-item)
                                ) ;let
                              ) ;and
                            ) ;define
                            (define (handle-sequence pat e)
                              (lambda (sel)
                                (and (eq? (type-of sel) (type-of pat))
                                  (let ((func-ok #t))
                                    (when (or (pair? pat) (vector? pat))
                                      (if (pair? (cyclic-sequences pat))
                                        (error 'wrong-type-arg
                                          "case* pattern is cyclic: ~S~%"
                                          pat
                                        ) ;error
                                      ) ;if
                                      (let ((pos (if (pair? pat)
                                                   (ellipsis-pair-position 0 pat)
                                                   (ellipsis-vector-position pat
                                                     (length pat)
                                                   ) ;ellipsis-vector-position
                                                 ) ;if
                                            ) ;pos
                                           ) ;
                                        (when (and pos
                                                (>= (length sel) (- (length pat) 1))
                                              ) ;and
                                          (let ((new-vars (list (splice-out-ellipsis sel pat pos e)
                                                          ) ;list
                                                ) ;new-vars
                                               ) ;
                                            (set! sel (car new-vars))
                                            (set! pat (cadr new-vars))
                                            (set! func-ok (caddr new-vars))
                                          ) ;let
                                        ) ;when
                                      ) ;let
                                    ) ;when
                                    (and (= (length sel) (length pat))
                                      func-ok
                                      (call-with-exit (lambda (return)
                                                        (for-each (lambda (sel-item pat-item)
                                                                    (or (equivalent? sel-item pat-item)
                                                                      (and (or (pair? pat-item) (vector? pat-item))
                                                                       ((handle-sequence pat-item e) sel-item)
                                                                      ) ;and
                                                                      (handle-pattern sel-item pat-item e)
                                                                      (return #f)
                                                                    ) ;or
                                                                  ) ;lambda
                                                          sel
                                                          pat
                                                        ) ;for-each
                                                        (unless (or (not (pair? sel))
                                                                  (proper-list? sel)
                                                                ) ;or
                                                          (let ((sel-item (list-tail sel (abs (length sel)))
                                                                ) ;sel-item
                                                                (pat-item (list-tail pat (abs (length pat)))
                                                                ) ;pat-item
                                                               ) ;
                                                            (return (or (equivalent? sel-item pat-item)
                                                                      (handle-pattern sel-item pat-item e)
                                                                    ) ;or
                                                            ) ;return
                                                          ) ;let
                                                        ) ;unless
                                                        #t
                                                      ) ;lambda
                                      ) ;call-with-exit
                                    ) ;and
                                  ) ;let
                                ) ;and
                              ) ;lambda
                            ) ;define
                            (define (find-labelled-pattern tree)
                              (or (undefined? tree)
                                (and (pair? tree)
                                  (or (find-labelled-pattern (car tree))
                                    (find-labelled-pattern (cdr tree))
                                  ) ;or
                                ) ;and
                                (and (vector? tree)
                                  (let vector-walker
                                    ((pos 0))
                                    (and (< pos (length tree))
                                      (or (undefined? (tree pos))
                                        (and (pair? (tree pos))
                                          (find-labelled-pattern (tree pos))
                                        ) ;and
                                        (and (vector? (tree pos))
                                          (vector-walker (tree pos))
                                        ) ;and
                                        (vector-walker (+ pos 1))
                                      ) ;or
                                    ) ;and
                                  ) ;let
                                ) ;and
                              ) ;or
                            ) ;define
                            (define (handle-body select body return e)
                              (if (null? body) (return select))
                              (when (find-labelled-pattern body)
                                (set! body
                                  (let pair-builder
                                    ((tree body))
                                    (cond ((undefined? tree)
                                           (let ((label (let ((str (object->string tree)))
                                                          (substring str 2 (- (length str) 1))
                                                        ) ;let
                                                 ) ;label
                                                ) ;
                                             (or (labels label) tree)
                                           ) ;let
                                          ) ;
                                          ((pair? tree)
                                           (cons (pair-builder (car tree))
                                             (pair-builder (cdr tree))
                                           ) ;cons
                                          ) ;
                                          ((vector? tree)
                                           (vector (map pair-builder tree))
                                          ) ;
                                          (else tree)
                                    ) ;cond
                                  ) ;let
                                ) ;set!
                              ) ;when
                              (return (eval (if (null? (cdr body))
                                              (car body)
                                              (if (eq? (car body) '=>)
                                                (list (cadr body) select)
                                                (cons 'begin body)
                                              ) ;if
                                            ) ;if
                                        e
                                      ) ;eval
                              ) ;return
                            ) ;define
                            (lambda (select clauses e)
                              (call-with-exit (lambda (return)
                                                (for-each (lambda (clause)
                                                            (let ((targets (car clause))
                                                                  (body (cdr clause))
                                                                 ) ;
                                                              (fill! labels #f)
                                                              (if (memq targets '(else #t))
                                                                (return (eval (cons 'begin body) e))
                                                                (for-each (lambda (target)
                                                                            (if (or (equivalent? target select)
                                                                                  (and (undefined? target)
                                                                                    (not (eq? target #<undefined>))
                                                                                    (let ((func (undefined->function target e)))
                                                                                      (and (procedure? func) (func select))
                                                                                    ) ;let
                                                                                  ) ;and
                                                                                  (and (sequence? target)
                                                                                   ((handle-sequence target e) select)
                                                                                  ) ;and
                                                                                ) ;or
                                                                              (handle-body select body return e)
                                                                            ) ;if
                                                                          ) ;lambda
                                                                  targets
                                                                ) ;for-each
                                                              ) ;if
                                                            ) ;let
                                                          ) ;lambda
                                                  clauses
                                                ) ;for-each
                                              ) ;lambda
                              ) ;call-with-exit
                            ) ;lambda
                          ) ;with-let
            ) ;case*-helper
           ) ;
        (#_macro (selector . clauses)
          `(((#_funclet (#_quote case*)) (#_quote case*-helper)) ,selector (quote ,clauses) (#_curlet))
        ) ;#_macro
      ) ;let
    ) ;define
  ) ;begin
) ;define-library
