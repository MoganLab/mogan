
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : markup-funcs.scm
;; DESCRIPTION : additional rendering macros written in scheme
;; COPYRIGHT   : (C) 2001  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils misc markup-funcs) (:use (utils library tree)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; TeXmacs version and release
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (texmacs-version-release* t)
  (:secure #t)
  (texmacs-version-release (tree->string t))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Map
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (ext-map fun to)
  (:secure #t)
  (with (op . args)
    (tree->list to)
    (with f
      (lambda (x) (list 'compound fun x))
      (list 'quote (cons 'tuple (map f args)))
    ) ;with
  ) ;with
) ;tm-define

(tm-define (ext-concat-tuple tup sep fin)
  (:secure #t)
  (with (op . l)
    (tree->list tup)
    (cond ((null? l) "")
          ((null? (cdr l)) (car l))
          (else `(concat ,(car l)
                   ,@(map (lambda (x) (list 'concat sep x)) (cDdr l))
                   ,(if (tm-equal? fin '(uninit)) sep fin)
                   ,(cAr l))
          ) ;else
    ) ;cond
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Select
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (rewrite-select pat)
  (if (atomic-tree? pat)
    (with s
      (tree->string pat)
      (if (not (string-starts? s "(")) (string->object s) s)
    ) ;with
    (with (op . r)
      (tree->list pat)
      (cond ((== op 'pat-any) :%1)
            ((== op 'pat-any-repeat) :*)
            ((== op 'pat-or) (cons :or (map rewrite-select r)))
            ((== op 'pat-and) (cons :and (map rewrite-select r)))
            ((== op 'pat-group) (cons :group (map rewrite-select r)))
            ((== op 'pat-and-not) (cons :and-not (map rewrite-select r)))
            (else #f)
      ) ;cond
    ) ;with
  ) ;if
) ;define

(tm-define (ext-select body args)
  (:secure #t)
  (with (op body2 . pat)
    (tree->list args)
    (list 'quote (cons 'tuple (select body (map rewrite-select pat))))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Navigation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (decode-tm-arg a)
  (with s
    (and (tree? a) (tree-atomic? a) (tree->string a))
    (cond ((not s) :same)
          ((and (string->number s) (integer? (string->number s))) (string->number s))
          ((== s "up") :up)
          ((== s "next") :next)
          ((== s "previous") :previous)
          ((== s "first") :first)
          ((== s "last") :last)
          (else :same)
    ) ;cond
  ) ;with
) ;define

(tm-define (ext-tm-ref body args)
  (:secure #t)
  (let* ((a (map decode-tm-arg (tm-children args))) (r (apply tm-ref (cons body a))))
    (if (tree? r) r "false")
  ) ;let*
) ;tm-define

(tm-define (ext-tm-arity body args)
  (:secure #t)
  (let* ((a (map decode-tm-arg (tm-children args))) (r (apply tm-ref (cons body a))))
    (if (tree? r) (number->string (tree-arity r)) "false")
  ) ;let*
) ;tm-define

(tm-define (ext-tm-index body args)
  (:secure #t)
  (let* ((a (map decode-tm-arg (tm-children args))) (r (apply tm-ref (cons body a))))
    (if (tree? r) (number->string (tree-index r)) "false")
  ) ;let*
) ;tm-define

(tm-define (ext-tm-last? body args)
  (:secure #t)
  (let* ((a (map decode-tm-arg (tm-children args))) (r (apply tm-ref (cons body a))))
    (if (and (tree? r) (tree-up r) (== (tree-index r) (- (tree-arity (tree-up r)) 1)))
      "true"
      "false"
    ) ;if
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Language suffix
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (ext-language-suffix)
  (:secure #t)
  (with s
    (language-to-locale (get-output-language))
    (if (>= (string-length s) 2) (substring s 0 2) "en")
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Applying a macro recursively to paragraphs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define ext-apply-on-paragraphs-root #f)

(define (ext-mark r t var rew)
  (let* ((rp (tree->path r)) (tp (tree->path t)))
    (if (and rp tp (list-starts? tp rp))
      (let* ((p (list-drop tp (length rp))) (ss (map number->string p)))
        `(mark (arg ,var ,@ss) ,rew)
      ) ;let*
      rew
    ) ;if
  ) ;let*
) ;define

(define (ext-apply-on-paragraphs-sub macro-name t)
  (cond ((tree-is? t 'document)
         (with fun
           (cut ext-apply-on-paragraphs macro-name <>)
           `(document ,@(map fun (tm-children t)))
         ) ;with
        ) ;
        ((tree-multi-line? t)
         (with fun
           (cut ext-apply-on-paragraphs-sub macro-name <>)
           (with rew
             (cons (tm-label t) (map fun (tm-children t)))
             (ext-mark ext-apply-on-paragraphs-root t "body" rew)
           ) ;with
         ) ;with
        ) ;
        (else t)
  ) ;cond
) ;define

(tm-define (ext-apply-on-paragraphs macro-name t)
  (:secure #t)
  (set! ext-apply-on-paragraphs-root t)
  (cond ((tree-multi-line? t) (ext-apply-on-paragraphs-sub macro-name t))
        ((tree-atomic? macro-name) `(,(string->symbol (tree->string macro-name))
                                     ,t))
        (else t)
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Line numbering
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define ext-numbered-root #f)

(define (is-algo-macro? line)
  (or (tree-is? line 'algo-if)
    (tree-is? line 'algo-else-if)
    (tree-is? line 'algo-else)
    (tree-is? line 'algo-while)
    (tree-is? line 'algo-for)
    (tree-is? line 'algo-for-all)
    (tree-is? line 'algo-for-each)
    (tree-is? line 'algo-repeat)
    (tree-is? line 'algo-loop)
    (tree-is? line 'algo-procedure)
    (tree-is? line 'algo-function)
    (tree-is? line 'algo-body)
    (tree-is? line 'algo-begin)
    (tree-is? line 'algo-inputs)
    (tree-is? line 'algo-outputs)
    (tree-is? line 'algo-if-else-if)
  ) ;or
) ;define

(define (ext-numbered-sub t)
  (cond ((tree-is? t 'document) `(document ,@(map ext-numbered-line
                                               (tm-children t))))
        ((tree-multi-line? t)
         (with rew
           (cons (tm-label t) (map ext-numbered-sub (tm-children t)))
           (ext-mark ext-numbered-root t "body" rew)
         ) ;with
        ) ;
        (else t)
  ) ;cond
) ;define

(define (ext-numbered-line t)
  (if (tree-multi-line? t) (ext-numbered-sub t) `(numbered-line ,t))
) ;define

(define (wrap-algo-body body lang)
  (cond ((tm-func? body 'document)
         (apply append
           (map (lambda (line) (wrap-algo-body line lang)) (tm-children body))
         ) ;apply
        ) ;
        ((is-algo-macro? body) (wrap-algo-macro body lang))
        ((tree-multi-line? body) (list (ext-numbered-sub body)))
        (else (if lang
                (list `(numbered-line (with ,"mode"
                                        ,"prog"
                                        ,"prog-language"
                                        ,lang
                                        ,"font-family"
                                        ,"rm"
                                        ,body))
                ) ;list
                (list `(numbered-line ,body))
              ) ;if
        ) ;else
  ) ;cond
) ;define

(define (build-else-if args lang)
  (cond ((null? args) (list '(numbered-line (concat (render-end-if)
                                              (right-flush)))))
        ((== (length args) 1)
         (append (list '(numbered-line (concat (render-else) (no-page-break))))
           (indent-lines (wrap-algo-body (car args) lang))
           (list '(numbered-line (concat (render-end-if) (right-flush))))
         ) ;append
        ) ;
        (else (let ((cond (car args)) (body (cadr args)) (rest (cddr args)))
                (append (list `(numbered-line (concat (render-else)
                                                ," "
                                                (render-if)
                                                ," "
                                                ,cond
                                                ," "
                                                (render-then)
                                                (no-page-break)))
                        ) ;list
                  (indent-lines (wrap-algo-body body lang))
                  (build-else-if rest lang)
                ) ;append
              ) ;let
        ) ;else
  ) ;cond
) ;define

(define (build-if-else-if args lang)
  (if (< (length args) 2)
    (list '(numbered-line (concat (render-end-if) (right-flush))))
    (let ((cond (car args)) (body (cadr args)) (rest (cddr args)))
      (append (list `(numbered-line (concat (render-if)
                                      ," "
                                      ,cond
                                      ," "
                                      (render-then)
                                      (no-page-break)))
              ) ;list
        (indent-lines (wrap-algo-body body lang))
        (build-else-if rest lang)
      ) ;append
    ) ;let
  ) ;if
) ;define

(define (indent-lines lines)
  (map (lambda (x) `(indent* ,x)) lines)
) ;define

(define (wrap-algo-macro line lang)
  (define (arg0 line)
    (if (>= (tm-arity line) 1) (tm-ref line 0) "")
  ) ;define
  (define (arg1 line)
    (if (>= (tm-arity line) 2) (tm-ref line 1) "")
  ) ;define
  (define (arg2 line)
    (if (>= (tm-arity line) 3) (tm-ref line 2) "")
  ) ;define
  (define (algo-header prefix cond mid suffix)
    `(numbered-line (concat ,@(if (== prefix "") '() (list prefix " "))
                      ,cond
                      ,@(if (== mid "") '() (list " " mid " "))
                      ,@(if (== suffix "") '() (list suffix))
                      (no-page-break)))
  ) ;define
  (cond ((tree-is? line 'algo-if)
         (let ((cond-arg (arg0 line)) (body-arg (arg1 line)))
           (cons (algo-header '(render-if) cond-arg '(render-then) "")
             (indent-lines (wrap-algo-body body-arg lang))
           ) ;cons
         ) ;let
        ) ;
        ((tree-is? line 'algo-else-if)
         (let ((cond-arg (arg0 line)) (body-arg (arg1 line)))
           (cons (algo-header '(render-else) '(render-if) cond-arg '(render-then))
             (indent-lines (wrap-algo-body body-arg lang))
           ) ;cons
         ) ;let
        ) ;
        ((tree-is? line 'algo-else)
         (cons '(numbered-line (concat (render-else) (no-page-break)))
           (indent-lines (wrap-algo-body (arg0 line) lang))
         ) ;cons
        ) ;
        ((tree-is? line 'algo-while)
         (let ((cond-arg (arg0 line)) (body-arg (arg1 line)))
           (append (list (algo-header '(render-while) cond-arg '(render-do) ""))
             (indent-lines (wrap-algo-body body-arg lang))
             (list '(numbered-line (concat (render-end-while) (right-flush))))
           ) ;append
         ) ;let
        ) ;
        ((tree-is? line 'algo-for)
         (let ((cond-arg (arg0 line)) (body-arg (arg1 line)))
           (append (list (algo-header '(render-for) cond-arg '(render-do) ""))
             (indent-lines (wrap-algo-body body-arg lang))
             (list '(numbered-line (concat (render-end-for) (right-flush))))
           ) ;append
         ) ;let
        ) ;
        ((tree-is? line 'algo-for-all)
         (let ((cond-arg (arg0 line)) (body-arg (arg1 line)))
           (append (list (algo-header '(render-for-all) cond-arg '(render-do) ""))
             (indent-lines (wrap-algo-body body-arg lang))
             (list '(numbered-line (concat (render-end-for) (right-flush))))
           ) ;append
         ) ;let
        ) ;
        ((tree-is? line 'algo-for-each)
         (let ((cond-arg (arg0 line)) (body-arg (arg1 line)))
           (append (list (algo-header '(render-for-each) cond-arg '(render-do) ""))
             (indent-lines (wrap-algo-body body-arg lang))
             (list '(numbered-line (concat (render-end-for) (right-flush))))
           ) ;append
         ) ;let
        ) ;
        ((tree-is? line 'algo-repeat)
         (let ((cond-arg (arg0 line)) (body-arg (arg1 line)))
           (append (list '(numbered-line (concat (render-repeat)
                                           (no-page-break))))
             (indent-lines (wrap-algo-body body-arg lang))
             (list `(numbered-line (concat (render-until)
                                     ," "
                                     ,cond-arg
                                     (right-flush))))
           ) ;append
         ) ;let
        ) ;
        ((tree-is? line 'algo-loop)
         (append (list '(numbered-line (concat (render-loop) (no-page-break))))
           (indent-lines (wrap-algo-body (arg0 line) lang))
           (list '(numbered-line (concat (render-end-loop) (right-flush))))
         ) ;append
        ) ;
        ((tree-is? line 'algo-procedure)
         (append (list `(numbered-line (concat (render-procedure)
                                         ," "
                                         (with ,"font-shape"
                                           ,"small-caps"
                                           ,(arg0 line))
                                         ,"("
                                         ,(arg1 line)
                                         ,")"
                                         (no-page-break)))
                 ) ;list
           (indent-lines (wrap-algo-body (arg2 line) lang))
           (list '(numbered-line (concat (render-end-procedure) (right-flush))))
         ) ;append
        ) ;
        ((tree-is? line 'algo-function)
         (append (list `(numbered-line (concat (render-function)
                                         ," "
                                         (with ,"font-shape"
                                           ,"small-caps"
                                           ,(arg0 line))
                                         ,"("
                                         ,(arg1 line)
                                         ,")"
                                         (no-page-break)))
                 ) ;list
           (indent-lines (wrap-algo-body (arg2 line) lang))
           (list '(numbered-line (concat (render-end-function) (right-flush))))
         ) ;append
        ) ;
        ((tree-is? line 'algo-body)
         (cons '(numbered-line (concat (render-do) (no-page-break)))
           (indent-lines (wrap-algo-body (arg0 line) lang))
         ) ;cons
        ) ;
        ((tree-is? line 'algo-begin)
         (cons '(numbered-line (concat (render-begin) (no-page-break)))
           (indent-lines (wrap-algo-body (arg0 line) lang))
         ) ;cons
        ) ;
        ((tree-is? line 'algo-inputs)
         (cons '(numbered-line (concat (render-inputs) (no-page-break)))
           (indent-lines (wrap-algo-body (arg0 line) lang))
         ) ;cons
        ) ;
        ((tree-is? line 'algo-outputs)
         (cons '(numbered-line (concat (render-outputs) (no-page-break)))
           (indent-lines (wrap-algo-body (arg0 line) lang))
         ) ;cons
        ) ;
        ((tree-is? line 'algo-if-else-if)
         (let* ((raw-args (cond ((== (tm-arity line) 0) '())
                                ((== (tm-arity line) 1)
                                 (let ((first (tm-ref line 0)))
                                   (if (tm-func? first 'document) (tm-children first) (list first))
                                 ) ;let
                                ) ;
                                (else (tm-children line))
                          ) ;cond
                ) ;raw-args
                (args (filter (lambda (x) (not (or (tree-is? x 'next-line) (tree-is? x 'new-line))))
                        raw-args
                      ) ;filter
                ) ;args
               ) ;
           (build-if-else-if args lang)
         ) ;let*
        ) ;
        (else (list line))
  ) ;cond
) ;define

(tm-define (ext-numbered body)
  (:secure #t)
  (set! ext-numbered-root body)
  (if (tm-func? body 'document)
    `(numbered-block (document ,@(apply append
                                   (map (lambda (line)
                                          (cond ((is-algo-macro? line)
                                                 (wrap-algo-macro line #f))
                                                ((tree-multi-line? line)
                                                 (list (ext-numbered-sub line)))
                                                (else (list `(numbered-line ,line)))))
                                     (tm-children body)))))
    body
  ) ;if
) ;tm-define

;; Numbered function with programming language support
(tm-define (ext-numbered-prog body lang)
  (:secure #t)
  (set! ext-numbered-root body)
  (if (tm-func? body 'document)
    `(numbered-block (document ,@(apply append
                                   (map (lambda (line)
                                          (cond ((is-algo-macro? line)
                                                 (wrap-algo-macro line lang))
                                                ((tree-multi-line? line)
                                                 (list (ext-numbered-sub line)))
                                                (else (list `(numbered-line (with ,"mode"
                                                                              ,"prog"
                                                                              ,"prog-language"
                                                                              ,lang
                                                                              ,"font-family"
                                                                              ,"rm"
                                                                              ,line))))))
                                     (tm-children body)))))
    body
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Fancy listings
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; (define (ext-listing-row body row)
;;  `(row (cell (with "color" "dark grey" "prog-language" "verbatim"
;;                    ,(number->string (+ row 1))))
;;        (cell (document ,(tm-ref body row)))))

;; (tm-define (ext-listing body)
;;  (:secure #t)
;;  (if (tm-func? body 'document)
;;      `(tformat
;;         (twith "table-width" "1par")
;;         (twith "table-hmode" "exact")
;;         (twith "table-hyphen" "y")
;;         (cwith "1" "-1" "1" "1" "cell-halign" "r")
;;         (cwith "1" "-1" "1" "1" "cell-lsep" "0em")
;;         (cwith "1" "-1" "2" "2" "cell-halign" "l")
;;         (cwith "1" "-1" "2" "2" "cell-rsep" "0em")
;;         (cwith "1" "-1" "2" "2" "cell-hpart" "1")
;;         (cwith "1" "-1" "2" "2" "cell-hyphen" "t")
;;         (cwith "1" "-1" "1" "-1" "cell-background"
;;                (if (equal (mod (value "cell-row-nr") "2") "0") "#f4f4ff" ""))
;;        (table ,@(map (lambda (row) (ext-listing-row body row))
;;                       (.. 0 (tm-arity body)))))
;;      body))
