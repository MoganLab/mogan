;
; Copyright (C) 2024 The Goldfish Scheme Authors
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;

(define-library (liii case)
  (import (liii base))
  (export case*)
  (begin

    ; 0 clause BSD, from S7 repo case.scm
    (define case* 
      (let
        ((case*-labels (lambda (label)
                         (let ((labels ((funclet ((funclet 'case*) 'case*-helper)) 'labels)))
                           (labels (symbol->string label))) ; if ellipsis, this has been quoted by case*
                         ) ;let
         ) ;case*-labels

         (case*-match?
           (lambda* (matchee pattern (e (curlet)))
             (let ((matcher ((funclet ((funclet 'case*) 'case*-helper)) 'handle-sequence)))
               (or (equivalent? matchee pattern)
                   (and (or (pair? matchee) 
                          (vector? matchee))
                     (begin
                       (fill! ((funclet ((funclet 'case*) 'case*-helper)) 'labels) #f) ; clear labels
                       ((matcher pattern e) matchee))
                     ) ;begin
                   ) ;and
               ) ;or
             ) ;let
         ) ;case*-match?
         (case*-helper
          (with-let (unlet)
            (define labels (make-hash-table))

            (define (ellipsis? pat)
              (and (undefined? pat)
                (or (equal? pat #<...>)
                    (let ((str (object->string pat)))
                      (and (char-position #\: str)
                           (string=? "...>" (substring str (- (length str) 4)))
                      ) ;and
                    ) ;let
                ) ;or
              ) ;and
            ) ;define

            (define (ellipsis-pair-position pos pat)
              (and (pair? pat)
                (if (ellipsis? (car pat))
                    pos
                    (ellipsis-pair-position (+ pos 1) (cdr pat))
                ) ;if
              ) ;and
            ) ;define

            (define (ellipsis-vector-position pat vlen)
              (let loop ((pos 0))
                (and (< pos vlen)
                  (if (ellipsis? (pat pos))
                    pos
                    (loop (+ pos 1))
                  ) ;if
                ) ;and
              ) ;let
            ) ;define

            (define (splice-out-ellipsis sel pat pos e)
              (let
                ((sel-len (length sel))
                 (new-pat-len (- (length pat) 1))
                 (ellipsis-label (and (not (eq? (pat pos) #<...>))              
                                   (let* ((str (object->string (pat pos)))
                                          (colon (char-position #\: str)))
                                     (and colon
                                          (substring str 2 colon)))
                                     ) ;and
                                   ) ;let*
                 ) ;ellipsis-label
                (let
                  ((func (and (string? ellipsis-label)
                           (let ((comma (char-position #\, ellipsis-label)))
                             (and comma
                                  (let ((str (substring ellipsis-label (+ comma 1))))
                                    (set! ellipsis-label (substring ellipsis-label 0 comma))
                                    (let ((func-val (symbol->value (string->symbol str) e)))
                                      (if (undefined? func-val)
                                          (error 'unbound-variable "function ~S is undefined\n" func)
                                      ) ;if
                                      (if (not (procedure? func-val))
                                          (error 'wrong-type-arg "~S is not a function\n" func)
                                      ) ;if
                                      func-val)))
                                    ) ;let
                                  ) ;let
                             ) ;and
                           ) ;let
                  (if (pair? pat)
                      (cond ((= pos 0)               ; ellipsis at start of pattern
                             (if ellipsis-label
                               (set! (labels ellipsis-label) 
                                     (list 'quote (copy sel (make-list (- sel-len new-pat-len))))
                               ) ;set!
                             ) ;if
                             (values (list-tail sel (- sel-len new-pat-len))
                               (cdr pat)
                               (or (not func)
                                 (func (cadr (labels ellipsis-label)))) ; value is (quote ...) and we want the original list here
                               ) ;or
                             ) ;values

                        ((= pos new-pat-len)     ; ellipsis at end of pattern
                         (if ellipsis-label
                           (set! (labels ellipsis-label) 
                                 (list 'quote (copy sel (make-list (- sel-len pos)) pos))
                           ) ;set!
                         ) ;if
                         (values (copy sel (make-list pos))
                           (copy pat (make-list pos))
                           (or (not func) 
                             (func (cadr (labels ellipsis-label)))
                           ) ;or
                         ) ;values
                        ) ;

                        (else                    ; ellipsis somewhere in the middle
                         (let ((new-pat (make-list new-pat-len))
                               (new-sel (make-list new-pat-len)))
                           (if ellipsis-label
                             (set! (labels ellipsis-label) 
                               (list 'quote (copy sel (make-list (- sel-len new-pat-len)) pos))
                             ) ;set!
                           ) ;if
                           (copy pat new-pat 0 pos)
                           (copy pat (list-tail new-pat pos) (+ pos 1))
                           (copy sel new-sel 0 pos)
                           (copy sel (list-tail new-sel pos) (- sel-len pos))
                           (values new-sel new-pat
                             (or (not func) 
                               (func (cadr (labels ellipsis-label)))
                             ) ;or
                           ) ;values
                         ) ;let
                        ) ;else
                      ) ;cond

                      (cond
                        ((= pos 0)
                         (if ellipsis-label
                           (set! (labels ellipsis-label) 
                                 (list 'quote (copy sel (make-list (- sel-len new-pat-len))))
                           ) ;set!
                         ) ;if
                         (values (subvector sel (max 0 (- sel-len new-pat-len)) sel-len) ; was new-pat-len (max 0 (- sel-len new-pat-len))
                           (subvector pat 1 (+ new-pat-len 1))                     ;     new-pat-len 1
                           (or (not func) 
                             (func (cadr (labels ellipsis-label))))
                           ) ;or
                         ) ;values

                        ((= pos new-pat-len)
                         (if ellipsis-label
                           (set! (labels ellipsis-label) 
                             (list 'quote (copy sel (make-list (- sel-len new-pat-len)) pos))
                           ) ;set!
                         ) ;if
                         (values (subvector sel 0 new-pat-len)
                           (subvector pat 0 new-pat-len)
                           (or (not func) 
                         (func (cadr (labels ellipsis-label)))
                           ) ;or
                         ) ;values
                        ) ;

                        (else
                         (let ((new-pat (make-vector new-pat-len))
                           (new-sel (make-vector new-pat-len)))
                           (if ellipsis-label
                         (set! (labels ellipsis-label) 
                           (list 'quote (copy sel (make-list (- sel-len new-pat-len)) pos))
                         ) ;set!
                           ) ;if
                           (copy pat new-pat 0 pos)
                           (copy pat (subvector new-pat pos new-pat-len) (+ pos 1))       ; (- new-pat-len pos) pos)   copy: (+ pos 1))
                           (copy sel new-sel 0 pos)
                           (copy sel (subvector new-sel pos new-pat-len) (- sel-len pos))
                           ; (- new-pat-len pos) pos)  copy: (- sel-len pos))
                           (values new-sel new-pat
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

            (define (handle-regex x) #f)
            ;(define handle-regex
            ;  (let ((rg ((*libc* 'regex.make))) ; is this safe?
            ;  (local-regcomp (*libc* 'regcomp))
            ;  (local-regerror (*libc* 'regerror))
            ;  (local-regexec (*libc* 'regexec))
            ;  (local-regfree (*libc* 'regfree)))
            ;    (lambda (reg)
            ;(lambda (x)
            ;  (and (string? x)
            ; (let ((res (local-regcomp rg (substring reg 1 (- (length reg) 1)) 0)))
            ;   (unless (zero? res)
            ;     (error 'regex-error "~S~%" (local-regerror res rg)))
            ;   (set! res (local-regexec rg x 0 0))
            ;   (local-regfree rg)
            ;   (zero? res)))))))

            (define (undefined->function undef e)   ; handle the pattern descriptor ("undef") of the form #< whatever >, "e" = caller's curlet
              (let* ((str1 (object->string undef))
                     (str1-end (- (length str1) 1)))
                (if (not (char=? (str1 str1-end) #\>))
                  (error 'wrong-type-arg "pattern descriptor does not end in '>': ~S\n" str1)
                ) ;if
                (let ((str (substring str1 2 str1-end)))
                  (if (= (length str) 0)                                           ; #<> = accept anything
                      (lambda (x) #t)
                      (let ((colon (char-position #\: str)))
                        (cond (colon                                               ; #<label:...> might be #<label:> or #<label:func>
                                (let ((label (substring str 0 colon))               ; str is label:...
                                      (func (substring str (+ colon 1))))           ; func might be ""
                                  (cond ((labels label)                             ; see if we already have saved something under this label
                                         (lambda (sel)                              ;   if so, return function that will return an error
                                           (error 'syntax-error "label ~S is defined twice: old: ~S, new: ~S~%" label (labels label) sel))
                                         ) ;lambda

                                        ;; otherwise the returned function needs to store the current sel-item under label in labels
                                        ((zero? (length func))
                                         (lambda (x)
                                           (set! (labels label) x)                  ; #<label:>, set label, accept anything
                                           #t
                                         ) ;lambda
                                        ) ;

                                        ((char=? (func 0) #\")                      ; labelled regex, #<label:"regexp">
                                         (lambda (x)
                                           (set! (labels label) x)
                                           (handle-regex func)
                                         ) ;lambda
                                        ) ;

                                        (else                                       ; #<label:func>
                                         (let ((func-val (symbol->value (string->symbol func) e)))
                                           (if (undefined? func-val)
                                               (error 'unbound-variable "function ~S is undefined\n" func)
                                               (if (not (procedure? func-val))
                                                 (error 'wrong-type-arg "~S is not a function\n" func)
                                                 (lambda (x)                     ; set label and call func
                                                   (set! (labels label) x)
                                                   (func-val x))
                                                 ) ;lambda
                                               ) ;if
                                           ) ;if
                                         ) ;let
                                        ) ;else
                                  ) ;cond
                                ) ;let

                          ;; if no colon either #<label> or #<func> or #<"regexp"> -- label means match its saved expr, func = call func
                          ((char=? (str 0) #\")
                           (handle-regex str)
                          ) ;

                          (else                                                ; #<label> or #<func>
                           (let ((saved (labels str)))
                             (if saved                                         ; #<label>
                                 (lambda (x) (equivalent? x saved))
                                 (symbol->value (string->symbol str) e) ; #<func> using curlet=e passed in above
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
              (and (undefined? pat-item)      ; turn #<func> into func and call it on the current selector element
                (not (eq? pat-item #<undefined>))
                (let ((func (undefined->function pat-item e)))
                  (if (undefined? func)
                    (error 'unbound-variable "function ~S is undefined\n" pat-item)
                  ) ;if
                  (if (not (procedure? func))
                    (error 'wrong-type-arg "~S is not a function\n" func)
                  ) ;if
                  (func sel-item)
                ) ;let
              ) ;and
            ) ;define

            (define (handle-sequence pat e)
              (lambda (sel)
                ;(format *stderr* "~S ~S~%" sel pat)
                (and (eq? (type-of sel) (type-of pat))
                  (let ((func-ok #t))

                    (when (or (pair? pat)                           ; look for ellipsis
                            (vector? pat))
                      (if (pair? (cyclic-sequences pat))
                          (error 'wrong-type-arg "case* pattern is cyclic: ~S~%" pat)
                      ) ;if
                      (let ((pos (if (pair? pat)
                                   (ellipsis-pair-position 0 pat)
                                   (ellipsis-vector-position pat (length pat)))))
                        (when (and pos
                                (>= (length sel) (- (length pat) 1))) ; else pat without ellipsis is too long for sel
                          (let ((new-vars (list (splice-out-ellipsis sel pat pos e))))
                            (set! sel (car new-vars))
                            (set! pat (cadr new-vars))
                            (set! func-ok (caddr new-vars))
                          ) ;let
                        ) ;when
                      ) ;let
                    ) ;when

                    (and (= (length sel) (length pat))             ; march through selector and current target matching elements
                      func-ok
                      (call-with-exit
                       (lambda (return)
                         (for-each 
                          (lambda (sel-item pat-item)
                            (or (equivalent? sel-item pat-item) ; items match

                                (and (or (pair? pat-item)       ; recursive check (* (+ #<symbol?> 1) 2), pat-item: (+ #symbol?> 1)
                                       (vector? pat-item))    ; pat-item, not sel-item here so pat-item can cover anything (a list for example)
                                  ((handle-sequence pat-item e) sel-item)
                                ) ;and

                                (handle-pattern sel-item pat-item e)

                                (return #f)                   ; else give up (selector does not match target)
                            ) ;or
                          ) ;lambda
                          sel pat
                         ) ;for-each

                         ;; dotted list, check final cdr
                         (unless (or (not (pair? sel)) 
                                   (proper-list? sel))
                           (let ((sel-item (list-tail sel (abs (length sel))))
                                 (pat-item (list-tail pat (abs (length pat)))))
                             (return (or (equivalent? sel-item pat-item)
                                       (handle-pattern sel-item pat-item e))
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
              ;; walk body looking for a labelled pattern
              (or (undefined? tree)
                (and (pair? tree)
                     (or (find-labelled-pattern (car tree))
                       (find-labelled-pattern (cdr tree))
                     ) ;or
                ) ;and
                (and (vector? tree)
                     (let vector-walker ((pos 0))
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
              (if (null? body)
                (return select)
              ) ;if

              (when (find-labelled-pattern body) ; if labelled, remake the body substituting the labelled-exprs for the labels
                (set! body (let pair-builder ((tree body))
                             (cond
                               ((undefined? tree)
                                (let ((label (let ((str (object->string tree)))
                                               (substring str 2 (- (length str) 1)))))
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

                               (else tree))
                             ) ;cond
                ) ;set!
              ) ;when

              ;; evaluate the result (case* expands into a call on case*-helper; we need to evaluate the result expressions ourselves)
              (return (eval (if (null? (cdr body))
                              (car body)
                              (if (eq? (car body) '=>)
                                (list (cadr body) select)
                                (cons 'begin body))
                              ) ;if
                        e)
              ) ;return
            ) ;define

            ;; case*-helper
            (lambda (select clauses e)
              (call-with-exit
               (lambda (return)
                 (for-each
                  (lambda (clause)                                        ;((target...) body...)
                    (let ((targets (car clause))
                          (body (cdr clause)))
                      (fill! labels #f)                                   ; clear previous labels
                      (if (memq targets '(else #t))                       ; (else...) or (#t...)
                        (return (eval (cons 'begin body) e))
                        (for-each
                         (lambda (target)
                           (if (or (equivalent? target select)
                                 (and (undefined? target)              ; #<...>
                                   (not (eq? target #<undefined>))
                                   (let ((func (undefined->function target e)))
                                     ;(format *stderr* "func: ~S~%" func)
                                     ;; (if (undefined? func) (error 'unbound-variable "function ~A is undefined\n" str))
                                     ;; not the above check because we want to be able to pass patterns as selectors! (scase37 in s7test)
                                     ;;    this seems like a mistake: #<symbol?> won't work? 
                                     (and (procedure? func)
                                       (func select)
                                     ) ;and
                                   ) ;let
                                 ) ;and
                                 (and (sequence? target)
                                   ((handle-sequence target e) select))
                                 ) ;and
                             (handle-body select body return e)
                           ) ;if
                         ) ;lambda
                         targets
                        ) ;for-each
                      ) ;if
                    ) ;let
                  ) ;lambda
                  clauses)
                 ) ;for-each
               ) ;lambda
              ) ;call-with-exit
            ) ;lambda
          ) ;with-let
         ) ;case*-helper
        ;; case*
        (#_macro (selector . clauses)
          `(((#_funclet 'case*) 'case*-helper) ,selector ',clauses (#_curlet))
        ) ;_macro
      ) ;let
    ) ;define

  ) ;begin
) ;define-library

