
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : math-edit.scm
;; DESCRIPTION : editing mathematics
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (math math-edit)
  (:use (utils library tree)
        (utils library cursor)
        (utils edit auto-close)
        (math math-drd)
        (generic format-geometry-edit)
        (convert tools tmconcat)
        (generic generic-edit)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Some drd properties, which should go into table-drd.scm later on
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-group variant-tag (math-table-tag))
(define-group similar-tag (math-table-tag))

(define-group math-table-tag
  matrix det bmatrix Bmatrix)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Spaces
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (allow-space-after? b)
  (and b (not (tm-func? b 'big))
       (or (not (tm-atomic? b))
           (let* ((s (tm->string b))
                  (last (and (!= s "") (tmstring-reverse-ref s 0)))
                  (type (and last (math-symbol-type last))))
             (nin? type (list "prefix" "infix" "separator" "prefix-infix"
                              "opening-bracket" "middle-bracket"))))))

(tm-define (skip-decorations-leftwards t)
  (if (and (tree? t)
           (tree-in? t '(rsub rsup rprime suppressed))
           (tree-ref t :previous))
      (skip-decorations-leftwards (tree-ref t :previous))
      t))

(tm-define (kbd-space-bar t shift?)
  (:require (and (tree-is-buffer? t) (in-math?)))
  (let* ((b (skip-decorations-leftwards (before-cursor)))
         (p (get-preference "math spacebar")))
    (cond ((== p "allow spurious spaces")
           (insert " "))
          ((and (== b " ") (== p "no spurious spaces"))
           (noop))
          ((== b " ")
           (remove-text #f)
           (make-space "1em"))
          ((and (tree? b) (tree-func? b 'space 1))
           (if (and (tree-atomic? (tree-ref b 0))
                    (string-ends? (tree->string (tree-ref b 0)) "em"))
               (make-space "1em")
               (geometry-horizontal b #t)))
          ((not (allow-space-after? b))
           (noop))
          (else
           (insert " ")))))

(tm-define (kbd-insert s)
  (:require (in-math?))
  (when (== (before-cursor) " ")
    (let* ((p (get-preference "math spacebar"))
           (type (if (string? s) (math-symbol-type s) "symbol")))
      (when (in? type (list "postfix" "infix" "separator" "prefix-infix"
                            "middle-bracket" "closing-bracket"))
        (remove-text #f))))
  (former s))

(tm-define (math-insert t)
  (insert t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Special customizations inside formulas and equations
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-enter t shift?)
  (:require (tree-is? t 'math))
  (go-end-of 'math)
  (insert-return))

(tm-define (kbd-enter t shift?)
  (:require (tree-is? t 'equation))
  (go-end-of 'equation)
  (insert-return))

(tm-define (kbd-enter t shift?)
  (:require (tree-is? t 'equation*))
  (go-end-of 'equation*)
  (insert-return))

(tm-define (label-insert t)
  (:require (tree-is? t 'eqnarray*))
  (go-end-line)
  (make 'label))

(tm-define (math-make-math)
  (if (inside? 'math)
      (go-end-of 'math)
      (set-message (string-append "Warning: already inside mathematics")
                   (string-append "make 'math'"))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines for moving punctuation symbols around
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (string-ref-nspace? s i)
  (!= (string-ref s i) #\space))

(define (string-ref-npunct? s i)
  (nin? (string-ref s i) '(#\space #\. #\, #\: #\;)))

(define (string-search-forwards s i n pred?)
  (cond ((>= i n) i)
        ((pred? s i) i)
        (else (string-search-forwards s (+ i 1) n pred?))))

(define (string-search-backwards s i b pred?)
  (cond ((<= i b) i)
        ((pred? s (- i 1)) i)
        (else (string-search-backwards s (- i 1) b pred?))))

(define (atomic-cut-left-until t pred?)
  (if (atomic-tree? t)
      (let* ((s (tree->string t))
             (n (string-length s))
             (i (string-search-forwards s 0 n pred?)))
        (if (> i 0)
            (with ss (substring s 0 i)
              (tree-remove! t 0 i)
              (tree-correct-old t)
              ss)
            ""))
      ""))

(define (atomic-cut-right-until t pred?)
  (if (atomic-tree? t)
      (let* ((s (tree->string t))
             (n (string-length s))
             (i (string-search-backwards s n 0 pred?)))
        (if (< i n)
            (with ss (substring s i n)
              (tree-remove! t i (- n i))
              (tree-correct-old t)
              ss)
            ""))
      ""))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Switching between inlined and displayed equations
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-macro (concat-isolate! t)
  `(cond ((not (tree-is? t :up 'concat)) (noop))
         ((not (tree-is? t :up :up 'document)) (noop))
         ((= (tree-arity (tree-up t)) 1) (tree-set! t :up t))
         ((< (tree-index t) (- (tree-arity (tree-up t)) 1))
          (tree-split (tree-up t 2)
                      (tree-index (tree-up t))
                      (+ (tree-index t) 1))
          (concat-isolate! t))
         (else
          (tree-split (tree-up t 2)
                      (tree-index (tree-up t))
                      (tree-index t))
          (concat-isolate! t))))

(define (math->equation* t)
  (let* ((c (and (tree-is? t :up 'concat) (tree-is? t :up :up 'document)))
         (r (and c (atomic-cut-left-until (tree-ref t :next)
                                          string-ref-npunct?)))
         (l (and c (atomic-cut-right-until (tree-ref t :previous)
                                           string-ref-nspace?))))
    (concat-isolate! t)
    (if (tree-is? t :up 'document)
        (begin
          (if (not r) (set! r ""))
          (if (not l) (set! l ""))
          (tree-set! t `(equation* (document ,(tree-ref t 0))))
          (while (string-ends? r " ")
            (set! r (string-drop-right r 1)))
          (with-cursor (tree->path t 0 0 :end)
            (insert r))))))

(define (equation*->math t)
  (if (or (not (tree-is? t 0 'document))
          (= (tree-arity (tree-ref t 0)) 1))
      (begin
        (if (tree-is? t 0 'document)
            (tree-set! t 0 (tree-ref t 0 0)))
        (tree-set! t `(math ,(tree-ref t 0)))
        (with r (atomic-cut-right-until (tree-end (tree-ref t 0))
                                        string-ref-npunct?)
          (with-cursor (tree->path t :start)
            (kbd-backspace)
            (if (and (!= (cursor-path) (cursor-after (go-start-paragraph)))
                     (!= (cursor-path) (cursor-after (go-end-paragraph))))
                (insert " ")))
          (with-cursor (tree->path t :end)
            (insert r)
            (kbd-delete)
            (if (and (!= (cursor-path) (cursor-after (go-start-paragraph)))
                     (!= (cursor-path) (cursor-after (go-end-paragraph))))
                (insert " ")))))))

(define (with-math-context? t)
  (match? t '(with "mode" "math" :%1)))

(tm-define (variant-circulate t forward?)
  (:require (with-math-context? t))
  (tree-set! t `(math ,(tree-ref t 2)))
  (math->equation* t))

(tm-define (variant-circulate t forward?)
  (:require (tree-is? t 'math))
  (math->equation* t))

(tm-define (variant-circulate t forward?)
  (:require (tree-in? t '(equation equation*)))
  (equation*->math t))

(tm-define (variant-formula t)
  (with-innermost t '(math equation equation*)
    (when (tree-in? t '(equation equation*))
      (equation*->math t))))

(tm-define (variant-equation t)
  (with-innermost t '(math equation equation*)
    (when (tree-in? t '(math))
      (math->equation* t))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Switching between displayed equations and equation arrays
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (search-labels t)
  (tree-search t (lambda (l) (tree-in? l (label-tag-list)))))

(define (cut-all t w)
  (cond ((tree-atomic? t) (noop))
        ((tree-is? t w) (tree-cut t))
        (else (for-each (cut cut-all <> w) (tree-children t)))))

(define (check-border? l empty?)
  (cond ((null? l) #t)
        ((and (not empty?) (nnull? (car l)) (!= (caar l) "")) #f)
        (else (check-border? (cdr l) (== (cAr (car l)) "")))))

(define (convertible-eqnarray? t)
  (let* ((rs (select (tm->stree t) '(:* row)))
         (rcs (map (lambda (r) (select r '(:* cell 0))) rs)))
    (check-border? rcs #t)))

(tm-define (eqnarray->equation t)
  (with labs (map tree-copy (search-labels t))
    (when (and (<= (length labs) 1) (convertible-eqnarray? t))
      (cut-all t 'label)
      (cut-all t 'eq-number)
      (let* ((l* (select t '(:* cell 0)))
             (l (if (null? labs) l* (cons (car labs) l*)))
             (c (apply tmconcat (map tree->stree l)))
             (n (if (null? labs) `(equation* ,c) `(equation ,c))))
        (tree-set! t n)
        (tree-go-to t 0 :end)))))

(define (binary-relations)
  (append (or (get-packrat-definition "std-symbols" "Relation-nolim-symbol")
              (list))
          (or (get-packrat-definition "std-symbols" "Assign-symbol")
              (list))))

(define (binary-relation? t)
  (in? (tm->stree t) (binary-relations)))

(define (atom-decompose t)
  (if (tree-atomic? t)
      (tmstring->list (tree->string t))
      (list t)))

(define (concat-decompose t)
  (cond ((tree-atomic? t) (atom-decompose t))
        ((tree-is? t 'concat)
         (apply append (map atom-decompose (tree-children t))))
        (else (list t))))

(define (make-eqn-row-sub l)
  (list "" (car l) (apply tmconcat (cdr l))))

(define (finalize-row l)
  `(row (cell ,(car l)) (cell ,(cadr l)) (cell ,(caddr l))))

(tm-define (equation->eqnarray t)
  (let* ((labs (map tm->stree (search-labels t)))
         (c* (tree-ref t 0))
         (c  (if (tree-func? c* 'document 1) (tree-ref c* 0) c*))
         (l0 (concat-decompose c))
         (l1 (list-filter l0 (lambda (x) (not (tm-is? x 'label)))))
         (l2 (list-scatter l1 binary-relation? #t)))
    (when (>= (length l2) 2)
      (cut-all t 'label)
      (let* ((l3 (cons (list (apply tmconcat (car l2))
                             (caadr l2)
                             (apply tmconcat (cdadr l2)))
                       (map make-eqn-row-sub (cddr l2))))
             (l4 (if (null? labs) l3
                     (rcons (cDr l3)
                            (rcons (cDr (cAr l3))
                                   (tmconcat (cAr (cAr l3))
                                             '(eq-number) (car labs))))))
             (l5 (map finalize-row l4))
             (r `(eqnarray* (document (tformat (table ,@l5))))))
        (tree-set! t r)
        (tree-go-to t 0 0 0 :last :last 0 :end)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Structured editing of roots
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (sqrt-toggle t)
  (when (tree-is? t 'sqrt)
    (cond ((== (tree-arity t) 1)
           (tree-insert! t 1 '(""))
           (tree-go-to t 1 0))
          ((== (tree-arity t) 2)
           (tree-remove! t 1 1)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Structured editing of scripts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (script-context? t)
  (tree-in? t '(lsub lsup rsub rsup)))

(tm-define (script-only-script? t)
  (and (tree-is? (tree-up t) 'concat)
       (not (or (and (tree-ref t :previous)
                     (script-context? (tree-ref t :previous)))
                (and (tree-ref t :next)
                     (script-context? (tree-ref t :next)))))))

(tm-define (variant-circulate t forward?)
  (:require (script-context? t))
  (when (script-only-script? t)
    (cond ((tree-is? t 'lsub) (variant-set t 'lsup))
          ((tree-is? t 'lsup) (variant-set t 'lsub))
          ((tree-is? t 'rsub) (variant-set t 'rsup))
          ((tree-is? t 'rsup) (variant-set t 'rsub)))))

(tm-define (structured-insert-vertical t downwards?)
  (:require (tree-in? t '(lsub rsub)))
  (when (and (not downwards?) (script-only-script? t))
    (tree-go-to t :end)
    (cond ((tree-is? t 'lsub) (make 'lsup))
          ((tree-is? t 'rsub) (make 'rsup)))))

(tm-define (structured-insert-vertical t downwards?)
  (:require (tree-in? t '(lsup rsup)))
  (when (and downwards? (script-only-script? t))
    (tree-go-to t :end)
    (cond ((tree-is? t 'lsup) (make 'lsub))
          ((tree-is? t 'rsup) (make 'rsub)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Structured editing of wide accents
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define wide-list-1
  '("~" "^" "<bar>" "<vect>" "<check>" "<breve>" "<invbreve>"))

(define wide-list-2
  '("<acute>" "<grave>" "<dot>" "<ddot>" "<dddot>" "<ddddot>" "<abovering>"))

(define wide-list-3
  '("<wide-overbrace>" "<wide-underbrace*>"
    "<wide-poverbrace>" "<wide-punderbrace*>"
    "<wide-sqoverbrace>" "<wide-squnderbrace*>"))

(define wide-list-4
  '("<wide-underbrace>" "<wide-overbrace*>"
    "<wide-punderbrace>" "<wide-poverbrace*>"
    "<wide-squnderbrace>" "<wide-sqoverbrace*>"))

(define wide-list-5
  '("<wide-varrightarrow>" "<wide-varleftarrow>"
    "<wide-varleftrightarrow>" "<wide-bar>"))

(tm-define (variant-circulate t forward?)
  (:require (tree-in? t '(wide wide*)))
  (when (tree-atomic? (tree-ref t 1))
    (with s (tree->string (tree-ref t 1))
      (and-with i (list-find-index wide-list-1 (lambda (x) (== x s)))
        (with j (modulo (+ i (if forward? 1 -1)) (length wide-list-1))
          (tree-set t 1 (list-ref wide-list-1 j))))
      (and-with i (list-find-index wide-list-2 (lambda (x) (== x s)))
        (with j (modulo (+ i (if forward? 1 -1)) (length wide-list-2))
          (tree-set t 1 (list-ref wide-list-2 j))))
      (and-with i (list-find-index wide-list-3 (lambda (x) (== x s)))
        (with j (modulo (+ i (if forward? 1 -1)) (length wide-list-3))
          (tree-set t 1 (list-ref wide-list-3 j))))
      (and-with i (list-find-index wide-list-4 (lambda (x) (== x s)))
        (with j (modulo (+ i (if forward? 1 -1)) (length wide-list-4))
          (tree-set t 1 (list-ref wide-list-4 j))))
      (and-with i (list-find-index wide-list-5 (lambda (x) (== x s)))
        (with j (modulo (+ i (if forward? 1 -1)) (length wide-list-5))
          (tree-set t 1 (list-ref wide-list-5 j)))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Wide arrows
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (make-long-arrow s)
  (when (and (string? s) (string-starts? s "<") (string-ends? s ">"))
    (with rs (string-append "<rubber-" (substring s 1 (string-length s)))
      (insert-go-to `(long-arrow ,rs "") '(1 0)))))

(tm-define (make-long-arrow* s)
  (when (and (string? s) (string-starts? s "<") (string-ends? s ">"))
    (with rs (string-append "<rubber-" (substring s 1 (string-length s)))
      (insert-go-to `(long-arrow ,rs "" "") '(2 0)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Modifying the shape of brackets
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define lbrackets
  '("(" "[" "{" "<langle>" "<llbracket>" "<lfloor>" "<lceil>" "|" "<||>" "/" "\\" "<nobracket>"))

(define mbrackets
  '("|" "<||>" "/" "\\"))

(define rbrackets
  '(")" "]" "}" "<rangle>" "<rrbracket>" "<rfloor>" "<rceil>" "|" "<||>" "\\" "/" "<nobracket>"))

(define (bracket-circulate t forward? brackets)
  (cond ((and (tree-in? t '(around around*))
              (== (tree-arity t) 3))
         (bracket-circulate (tree-ref t 0) forward? lbrackets)
         (bracket-circulate (tree-ref t 2) forward? rbrackets))
        ((and (tree-is? t 'left) (> (tree-arity t) 1))
         (bracket-circulate (tree-ref t 0) forward? lbrackets))
        ((and (tree-is? t 'mid) (> (tree-arity t) 1))
         (bracket-circulate (tree-ref t 0) forward? mbrackets))
        ((and (tree-is? t 'right) (> (tree-arity t) 1))
         (bracket-circulate (tree-ref t 0) forward? rbrackets))
        ((and (tree-atomic? t)
              (in? (tree->string t) brackets))
         (let* ((s (tree->string t))
                (i (list-find-index brackets (lambda (x) (== x s))))
                (j (modulo (+ i (if forward? 1 -1)) (length brackets)))
                (r (list-ref brackets j)))
           (tree-assign t r)))))

(tm-define (variant-circulate t forward?)
  (:require (tree-in? t '(left mid right around around*)))
  (bracket-circulate t forward? mbrackets))

(define bigops
  '("<int>" "<intlim>" "<oint>" "<ointlim>"
    "<sum>" "<prod>" "<amalg>"
    "<cap>" "<cup>" "<sqcap>" "<sqcup>"
    "<vee>" "<wedge>" "<curlyvee>" "<curlywedge>"
    "<odot>" "<otimes>" "<oplus>"
    "<triangleup>" "<triangledown>"
    "<box>" "<parallel>" "<interleave>"))

(tm-define (variant-circulate t forward?)
  (:require (tree-is? t 'big-around))
  (when (and (== (tree-arity t) 2)
             (tree-atomic? (tree-ref t 0)))
    (with s (tree->string (tree-ref t 0))
      (when (in? s bigops)
        (let* ((i (list-find-index bigops (lambda (x) (== x s))))
               (j (modulo (+ i (if forward? 1 -1)) (length bigops)))
               (ns (list-ref bigops j)))
          (tree-assign (tree-ref t 0) ns))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Modifying the dimension of brackets
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bracket-size-increase t by)
  (cond ((and (tree-in? t '(left mid right))
              (>= (tree-arity t) 2))
         (bracket-size-increase (tree-ref t 1) by))
        ((and (tree-in? t '(around around*))
              (== (tree-arity t) 3))
         (when (tree-atomic? (tree-ref t 0))
           (tree-set t 0 `(left ,(tree-ref t 0) "0")))
         (when (tree-atomic? (tree-ref t 2))
           (tree-set t 2 `(right ,(tree-ref t 2) "0")))
         (bracket-size-increase (tree-ref t 0) by)
         (bracket-size-increase (tree-ref t 2) by)
         (when (tm-equal? (tree-ref t 0 1) "0")
           (tree-set t 0 (tree-ref t 0 0)))
         (when (tm-equal? (tree-ref t 2 1) "0")
           (tree-set t 2 (tree-ref t 2 0))))
        ((tree-integer? t)
         (let* ((old (tree->number t))
                (new (+ old by)))
           (tree-set t (number->string new))))
        ((tm-length? t)
         (length-increase t by))))

(tm-define (geometry-vertical t down?)
  (:require (tree-in? t '(left mid right around around*)))
  (with inc (if down? -1 1)
    (bracket-size-increase t inc)))

(define (bracket-size-reset t)
  (when (and (tree-in? t '(left mid right)) (>= (tree-arity t) 1))
    (tree-set t (tree-ref t 0))))

(tm-define (geometry-default t)
  (:require (tree-in? t '(around around*)))
  (when (== (tree-arity t) 3)
    (bracket-size-reset (tree-ref t 0))
    (bracket-size-reset (tree-ref t 2))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Not necessarily matching brackets
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (find-non-bracket t)
  (cond ((not (tree-up t)) t)
        ((tm-in? t '(around around* big-around temp-slot))
         (find-non-bracket (tree-up t)))
        ((tm-in? (tree-up t) '(concat around around* big-around temp-slot))
         (find-non-bracket (tree-up t)))
        (else t)))

(define (find-and-remove-temp-slot x)
  ;;(display* "x= " x "\n")
  (cond ((tree-atomic? x) #f)
        ((tree-is? x 'temp-slot)
         (tree-go-to x 0 0)
         (remove-structure-upwards))
        ((tree? x)
         (list-or (map find-and-remove-temp-slot (tree-children x))))
        (else #f)))

(tm-define (brackets-refresh)
  (insert-go-to '(temp-slot "") '(0 0))
  (let* ((t (find-non-bracket (cursor-tree)))
         (u (tree-downgrade-brackets t #t #f))
         (v (tree-upgrade-brackets u "math"))
         (w (tree-downgrade-big v)))
    ;;(tree-set! t v)
    (tree-set! t w)
    (find-and-remove-temp-slot t)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Match bracket with missing bracket
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (count-missing t open?)
  (cond ((tree-atomic? t) 0)
        ((tree-in? t '(around around*))
         (+ (count-missing (tree-ref t 1) open?)
            (if (deleted? t (if open? 0 2)) 1 0)))
        ((tree-is? t (if open? 'right 'left)) 1)
        ((tree-is? t 'concat)
         (with l (tree-children t)
           (apply + (map (lambda (x) (count-missing x open?)) l))))
        (else 0)))

(define (try-matching-insert open? which large?)
  (try-modification
    (let* ((nr (count-missing (find-non-bracket (cursor-tree)) open?))
           (tag (if large? 'around* 'around)))
      ;;(display* nr ", " (find-non-bracket (cursor-tree)) "\n")
      (if open?
          (insert-go-to (list tag which "" "<nobracket>") '(1 0))
          (insert-go-to (list tag "<nobracket>" "" which) '(1)))
      (brackets-refresh)
      ;;(display* (count-missing (find-non-bracket (cursor-tree)) open?) ", "
      ;;(find-non-bracket (cursor-tree)) "\n")
      (> nr (count-missing (find-non-bracket (cursor-tree)) open?)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Matching brackets
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (deleted? t i)
  (== (tm->stree (tree-ref t i)) "<nobracket>"))

;;(define (make-small s)
;;  (cond ((nstring? s) "<nobracket>")
;;        ((== s ".") "<nobracket>")
;;        ((<= (string-length s) 1) s)
;;        (else (string-append "<" s ">"))))

(define (find-adjacent-around del?)
  (let* ((ret #f)
         (p (cursor-path))
         (p* (cursor-path*)))
    (with t (tree-innermost '(around around*))
      (when t
        (when (== p (tree->path t 1 :start))
          (when (deleted? t 0)
            (set! ret t)))
        (when (== p (tree->path t 1 :end))
          (when (or (not del?) (deleted? t 2))
            (set! ret t)))))
    (when (not ret)
      (with t (path->tree (cDr p))
        (when (tree-in? t '(around around*))
          (when (== (cAr p) 0)
            (when (deleted? t 0)
              (set! ret t)))
          (when (== (cAr p) 1)
            (when (or (not del?) (deleted? t 2))
              (set! ret t))))))
    (when (and (not ret) (!= p p*))
      (with t (path->tree (cDr p*))
        (when (tree-in? t '(around around*))
          (when (== (cAr p*) 0)
            (when (deleted? t 0)
              (set! ret t))))))
    ret))

(tm-define (math-bracket-open lb rb large?)
  (when (== large? 'default)
    (set! large? (!= (get-preference "use large brackets") "off")))
  (when (== (get-preference "automatic brackets") "off")
    (make-bracket-open lb rb large?)
    (brackets-refresh))
  (when (!= (get-preference "automatic brackets") "off")
    (if (selection-active-normal?)
        (begin
          (clipboard-cut "temp")
          (insert-go-to `(,(if large? 'around* 'around) ,lb "" ,rb) '(1 0))
          (clipboard-paste "temp"))
        (let* ((t (find-adjacent-around #t))
               (u (find-adjacent-around #f)))
          (cond ((and t (deleted? t 2)
                      (or (not (deleted? t 0))
                          (tree-at-end? t)))
                 (tree-set t 2 lb)
                 (tree-go-to t :end))
                ((and t (deleted? t 0))
                 (tree-set t 0 lb)
                 (tree-go-to t 1 :start))
                ((and u (== lb rb)
                      (== (tree->stree (tree-ref u 2)) rb))
                 (tree-go-to u :end))
                ((and u (== rb "|")
                      (== (tree->stree (tree-ref u 0)) "<langle>"))
                 (tree-set u 2 rb)
                 (tree-go-to u :end))
                ((try-matching-insert #t lb large?)
                 (noop))
                (else
                  (insert-go-to `(,(if large? 'around* 'around) ,lb "" ,rb)
                                '(1 0))))))))

(tm-define (math-separator sep large?)
  (when (== large? 'default)
    (set! large? (!= (get-preference "use large brackets") "off")))
  (when (and (string? sep) (string-starts? sep "<") (string-ends? sep ">"))
    (set! sep (substring sep 1 (- (string-length sep) 1))))
  (when (== (get-preference "automatic brackets") "off")
    (make-separator sep large?)
    (brackets-refresh))
  (when (!= (get-preference "automatic brackets") "off")
    (make-separator sep large?)))

(tm-define (math-bracket-close rb lb large?)
  (when (== large? 'default)
    (set! large? (!= (get-preference "use large brackets") "off")))
  (when (== (get-preference "automatic brackets") "off")
    (make-bracket-close rb lb large?)
    (brackets-refresh))
  (when (!= (get-preference "automatic brackets") "off")
    (let* ((t (find-adjacent-around #t))
           (u (find-adjacent-around #f)))
      (cond ((and t (deleted? t 0)
                  (or (not (deleted? t 2))
                      (not (tree-at-end? t))))
             (tree-set t 0 rb)
             (tree-go-to t 1 :start))
            ((and t (deleted? t 2))
             (tree-set t 2 rb)
             (tree-go-to t :end))
            (u
             (tree-set u 2 rb)
             (tree-go-to u :end))
            ((try-matching-insert #f rb large?)
             (noop))
            (else
              (set-message "Error: bracket does not match"
                           (force-string rb)))))))

(tm-define (math-big-operator op)
  ;;(when (== (get-preference "automatic brackets") "off")
  ;;  (make-big-operator op)
  ;;  (brackets-refresh))
  ;;(when (!= (get-preference "automatic brackets") "off")
  ;;  (insert-go-to `(big-around ,(make-small op) "") '(1 0)))
  (insert `(big ,op)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Correction of mathematical formulas
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (manual-correct-math t)
  (with mode (tree->stree (get-env-tree-at "mode" (tree->path t :start)))
    (if (!= mode "math")
        (manual-correct t)
        (with r (manual-correct `(math ,t))
          (if (tm-func? r 'math 1) (tm-ref r 0) r)))))

(define (math-correct-tree t)
  (with r (manual-correct-math t)
    (when (!= r t)
      (tree-set! t r))))

(define (math-manually-correct-tree t)
  (with r (manual-correct-math t)
    (when (!= r t)
      (import-from (version version-compare))
      (let* ((t1 (tree->stree t))
             (t2 (tree->stree r))
             (tb (compare-versions t1 t2))
             (rt (stree->tree tb)))
        (tree-set! t rt)
        (tree-go-to t :start)
        (version-next-difference)))))

(tm-define (math-correct-all)
  (:synopsis "Correct selected formula or all formulas in document")
  (if (selection-active-any?)
      (if (== (selection-tree) (path->tree (selection-path)))
          (math-correct-tree (path->tree (selection-path)))
          (set-message "Only implemented for complete subtrees"
                       "correct formula"))
      (math-correct-tree (buffer-tree))))

(tm-define (math-correct-manually)
  (:synopsis "Manually correct selected formula or all formulas in document")
  (if (selection-active-any?)
      (if (== (selection-tree) (path->tree (selection-path)))
          (math-manually-correct-tree (path->tree (selection-path)))
          (set-message "Only implemented for complete subtrees"
                       "correct formula"))
      (math-manually-correct-tree (buffer-tree))))

#|
lambda-to-symbol
将按键绑定的 Lambda 过程转换为 UI 可显示的符号名称。
主要用于在 Tab 键循环补全菜单中显示那些绑定为函数（而非直接字符串）的数学符号。

语法
----
(lambda-to-symbol f)

参数
----
f : procedure
    需要分析的目标函数（过程对象）。通常是由 kbd-map 生成的闭包。

返回值
----
list | boolean
- 成功时：返回 `(symbol-completion "符号名")`，例如 `(symbol-completion "<big-int-2>")`。
- 失败时：返回 #f。

逻辑
----
1. 类型检查：确认输入 f 是否为合法的 procedure。
2. 源码获取：调用 (procedure-source f) 获取源代码。
3. 结构校验：
   - 检查源码是否为列表且长度至少为 3 (标准 lambda 结构)。
   - 提取函数体 (caddr src)。
   - 检查函数体是否为非空列表，且包含参数。
   - 检查第一个参数是否为字符串。
4. 符号解析：
   - 提取函数名 (func-name) 和参数 (arg)。
   - 使用 case 结构匹配支持的函数名（目前支持 math-big-operator）。
   - 根据匹配结果拼接对应的 TeXmacs 内部符号名字符串。

注意
----
此函数采用了防御性编程风格，包含多层条件检查 (cond)，
以防止因读取到非标准结构的 Lambda 源码而导致编辑器崩溃。
目前主要用于处理 math-big-operator 生成的积分、求和等大运算符。
|#
(define (lambda-to-symbol f)
  (if (not (procedure? f))
      #f
      (let ((src (procedure-source f)))
        (cond
          ((not (pair? src)) #f)
          ;; 确保结构至少是 (lambda () body...)
          ((< (length src) 3) #f)
          (else
           (let ((body (caddr src)))
             (cond
               ((not (pair? body)) #f)
               ((null? (cdr body)) #f)
               (else
                (let ((func-name (car body)))
                  (case func-name
                    ;; Case 1: math-big-operator (处理积分、求和等大运算符)
                    ;; 转换目标: "<big-int-2>" (TeXmacs 内部字体图标名)
                    ((math-big-operator)
                     (and (string? (cadr body))
                          `(symbol-completion
                            ,(string-append "<big-" (cadr body) "-2>"))))
                    ;; Case 2: make-lprime (左上标/prime 符号)
                    ((make-lprime)
                     (and (string? (cadr body))
                          `(symbol-completion ,(cadr body))))
                    ;; Case 3: math-bracket-open (括号对)
                    ;; 提取左右括号并组合为 "lb…rb" 格式，确保每个变体唯一
                    ((math-bracket-open)
                     (and (>= (length (cdr body)) 2)
                          (string? (cadr body))
                          (string? (caddr body))
                          (let ((lb (cadr body))
                                (rb (caddr body)))
                            `(symbol-completion
                              ,(string-append lb rb)))))
                    ;; 预留位置：可以在此添加其他函数的处理逻辑
                    
                    (else #f)))))))))))

#|
function-to-symbol
Tab 循环补全中处理函数型绑定的入口分发器。

语法
----
(function-to-symbol val)

参数
----
val : pair
    键值绑定的内容。通常格式为 (command help-string) 或 (command)。
    其中 command 可能是字符串，也可能是过程 (procedure)。

返回值
----
list | boolean
- 成功时：返回 `(symbol-completion "符号名")`。
- 失败时：返回 #f。

逻辑
----
1. 检查 val 是否为列表 (pair)。
2. 检查 val 的第一个元素 (绑定的命令) 是否为过程对象 (procedure)。
3. 如果上述检查通过，调用 lambda-to-symbol 进行核心的源码解析和符号转换。
4. 任何步骤失败则直接返回 #f。

注意
----
此函数在 tabcycle-symbols 中作为过滤器使用。
它确保只有当按键绑定是函数时才尝试解析，如果是普通字符串或其他类型则跳过。
|#
(define (function-to-symbol val)
  (and (pair? val)
       (procedure? (car val))
       (lambda-to-symbol (car val))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tab cycling completion
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (tabcycle-symbols comb)
  ;; 根据按键序列获取数学符号Tab循环展示的列表
  ;; 输入:
  ;;     comb: 按键序列，类型为string，如"< tab tab = tab"
  ;; 处理过程:
  ;;     1. 递归删除末尾所有的 " tab"，得到基本序列
  ;;        例如: "< tab tab = tab tab" -> "< tab tab ="
  ;;     2. 查找该基本序列对应的符号绑定
  ;;     3. 如果绑定不存在，返回空列表
  ;;     4. 如果绑定存在，继续查找所有以该基本序列为前缀的符号绑定
  ;;     5. 返回找到的所有直接的符号绑定列表，按照键盘组合序列长度排序，
  ;;        例如((symbol-completion "<alpha>"))
  ;; 输出:
  ;;     符号列表，格式为((symbol-completion "符号") ...)
  (if (not (kbd-find-key-binding comb)) '()
    (let* ((tab-info (let loop ((s comb) (count 0))
                       (if (string-ends? s " tab")
                           (loop (substring s 0 (- (string-length s) 4))
                                 (+ count 1))
                           (cons s count))))
           (pre (car tab-info))
           (tab-iter (cdr tab-info))
           (pre (string-replace pre "space " ""))
           (kbd-res (kbd-find-key-binding pre))
           (kbd-sym (and (pair? kbd-res) (car kbd-res)))
           ;; primary-sym 是当前已输入的符号 (pre 的绑定)
           (primary-sym kbd-sym)
           (base (cond
                    ((string? primary-sym)
                     `((symbol-completion ,primary-sym)))
                    ((procedure? primary-sym)
                     (let ((sym (function-to-symbol kbd-res)))
                       (if sym (list sym) '())))
                    (else '())))
           ;; 使用新的 kbd-find-prefix-tab 获取所有 tab 切换候选
           (tab-pairs (kbd-find-prefix-tab pre)))
      (let ((others (filter-map (lambda (pair)
                                  (let ((val (cdr pair)))
                                    (if (and (pair? val) (string? (car val)))
                                        `(symbol-completion ,(car val))
                                        (function-to-symbol val))))
                                tab-pairs)))
        (if (not (null? base))
            (let* ((primary-name (and (pair? (car base))
                                      (= (length (car base)) 2)
                                      (cadr (car base))))
                   (filtered (filter (lambda (entry)
                                       (or (not (pair? entry))
                                           (not (= (length entry) 2))
                                           (not (string? (cadr entry)))
                                           (not primary-name)
                                           (not (string=? (cadr entry) primary-name))))
                                     others)))
              (append base filtered))
            '())))))

#|
highlight-tabcycle-symbols
高亮显示 Tab 循环菜单中当前选中的符号。

语法
----
(highlight-tabcycle-symbols lst comb)

参数
----
lst : list
    候选符号列表。通常格式为 ((symbol-completion "符号名") ...)。
comb : string
    当前的按键序列（例如 "< tab"）。

返回值
----
list
- 返回一个新的列表。
- 如果找到匹配项，该项的标签会被修改为 `symbol-completion*`（用于 UI 高亮渲染）。
- 列表其余部分保持不变。

逻辑
----
1. 预处理 comb 字符串（移除 "space "）。
2. 查询该按键序列对应的绑定命令。
3. 解析绑定命令以获取“目标符号名”：
   - 如果绑定是字符串，直接将其作为目标名。
   - 如果绑定是过程（procedure），调用 function-to-symbol 尝试解析出符号名。
4. 遍历 lst，将名称与“目标符号名”一致的项标记为高亮状态。

注意
----
此函数已增强对函数型绑定的支持。它不仅能高亮普通的字符串映射符号，
也能通过反向解析高亮那些绑定到 Scheme 函数（如大运算符）的复杂符号。
|#
(define (highlight-tabcycle-symbols lst comb)
  ;; 高亮显示Tab循环符号列表中符合按键序列的符号
  (let* ((comb1 (string-replace comb "space " ""))
         (res (kbd-find-key-binding comb1)))
    (if (not res)
        '() ;; 如果查不到任何绑定，直接返回空
        (let* ((raw-cmd (car res))
               ;; 统一解析出字符串类型的符号名 (bind-name)
               (bind-name
                (cond
                  ;; 情况 A: 绑定直接就是字符串 (例如 "<leq>")
                  ((string? raw-cmd) raw-cmd)
                  ;; 情况 B: 绑定是函数 (例如 math-big-operator 的闭包)
                  ((procedure? raw-cmd)
                   ;; 利用 function-to-symbol 解析
                   (let ((parsed (function-to-symbol res))) 
                     ;; function-to-symbol 返回的是 (symbol-completion "名字")
                     (if (and (pair? parsed)
                              (eq? (car parsed) 'symbol-completion))
                         (cadr parsed)
                         #f)))
                  
                  (else #f))))
          
          (if (string? bind-name)
              (map (lambda (x) 
                     (if (and (pair? x)
                              (eq? (car x) 'symbol-completion)
                              ;; 用解析出来的 bind-name 进行比对
                              (string=? (cadr x) bind-name))
                         (list 'symbol-completion* bind-name)
                         x))
                   lst)
              '())))))

(tm-define (math-tabcycle-symbols comb)
  ;; 根据按键序列获取数学符号Tab循环展示的列表
  ;; 输入:
  ;;     comb: 按键序列，类型为string，如"b tab"
  ;; 输出:
  ;;     符号列表，格式为((symbol-completion "符号") (symbol-completion* "符号") ...)
  (if (in-math?)
    (highlight-tabcycle-symbols (tabcycle-symbols comb) comb)
    '()))

;; 全局栈，用于管理Tab循环状态
(define tab-cycle-stack '())

(define (should-clear-stack? comb)
  (let ((base (kbd-base-sequence comb)))
    (or (null? tab-cycle-stack)
        (not (string=? base (kbd-base-sequence (car tab-cycle-stack)))))))

;; 管理栈：如果需要清空则清空，如果不在栈中则入栈
(define (manage-tab-stack comb)
  (let ((should-clear? (should-clear-stack? comb))
        (already-in-stack? (member comb tab-cycle-stack)))
    (when should-clear?
      (set! tab-cycle-stack '()))
    (when (not already-in-stack?)
      (set! tab-cycle-stack (cons comb tab-cycle-stack)))))

(tm-define (math-tabcycle-menu-needed? comb)
  ;; 管理栈状态
  (manage-tab-stack comb)

  (let* ((size (length (math-tabcycle-symbols comb)))
         (stack-depth (length tab-cycle-stack)))
    (and (or (string-contains? comb "tab") (> stack-depth 1))
         (> size 1))))

(tm-define (math-variant comb)
  ;; 触发数学符号Tab循环的展示
  ;; 输入:
  ;;     comb: 按键序列，类型为string，如"b tab"
  ;; 效果:
  ;;     如果在数学模式下，且补全符号数大于1，
  ;;     则显示一个补全菜单，包含所有符合按键序列的符号
  (if (in-math?)
    (with symbols (math-tabcycle-symbols comb)
      (if (> (length symbols) 1)
        (set-auxiliary-widget
          (make-menu-widget
            `((tile 99 (link (lambda () ,symbols)))) 0)
          "Math")))
        (noop))
    (noop))

(define bracket-variant-alist
  ;; 括号变体统一映射表和辅助函数
  ;; 定义统一的变体映射表：每个变体对应左右括号符号
  '((parentheses      "(" ")")
    (brackets         "[" "]")
    (braces           "{" "}")
    (angle            "<langle>" "<rangle>")
    (double           "<llbracket>" "<rrbracket>")
    (floor            "<lfloor>" "<rfloor>")
    (ceiling          "<lceil>" "<rceil>")
    (vertical         "|" "|")
    (double-vertical  "<||>" "<||>")
    (slash            "/" "\\")
    (backslash        "\\" "/")
    (empty            "<nobracket>" "<nobracket>")))

(define (get-bracket-node child)
  ;; 获取括号节点中的实际符号节点
  (cond ((tree-atomic? child) child)
        ((and (tree-in? child '(left right))
              (> (tree-arity child) 1))
         (tree-ref child 0))
        (else #f)))

;; 辅助函数：
(define (set-bracket-node! child sym)
  ;; 设置括号节点中的符号
  (with node (get-bracket-node child)
    (when node (tree-assign node sym))))

(define (bracket-set-pair t lb rb)
  ;; 括号设置函数
  ;; 实际修改 around/around* 节点的左右括号符号
  (when (== (tree-arity t) 3)
    (set-bracket-node! (tree-ref t 0) lb)
    (set-bracket-node! (tree-ref t 2) rb)))

(define (get-bracket-symbol child)
  ;; 括号符号提取器
  ;; 从子节点中提取括号符号字符串
  (with node (get-bracket-node child)
    (if node (tree->string node) "")))

(define (bracket-pair->variant lb rb)
  ;; 符号到标识的映射器
  ;; 将具体的括号符号对映射到伪变体标识符
  (let loop ((alist bracket-variant-alist))
    (cond ((null? alist) #f)
          ((and (== lb (cadar alist))
                (== rb (caddar alist)))
           (caar alist))
          (else (loop (cdr alist))))))

#|
get-bracket-variant
从 around/around* 节点检测当前括号类型，返回伪变体标识

语法
----
(get-bracket-variant t)

参数
----
t : tree
必须是 around 或 around* 节点，表示一个括号结构

返回值
----
symbol | boolean
- 成功时：返回对应的变体标识符（如 parentheses, brackets, braces 等）
- 失败时：返回 #f（当 t 不是 around/around* 节点或 arity 不为 3 时）

逻辑
----
1. 检查 t 是否为 around 或 around* 节点（通过 :require 条件）
2. 验证节点的 arity 是否为 3（左括号、内容、右括号）
3. 通过 get-bracket-symbol 提取左右括号的实际符号字符串
4. 调用 bracket-pair->variant 将符号对映射到预定义的变体标识

注意
----
此函数是括号变体系统的一部分，用于检测当前括号的显示样式。
变体标识定义在 bracket-variant-alist 中，如 parentheses, brackets, braces 等
|#
(tm-define (get-bracket-variant t)
  ;; 当前括号类型检测器
  ;; 从 around 节点检测当前括号类型，返回伪变体标识
  (:require (tree-in? t '(around around*)))
  (if (== (tree-arity t) 3)
      (with lb (get-bracket-symbol (tree-ref t 0))
        (with rb (get-bracket-symbol (tree-ref t 2))
          (bracket-pair->variant lb rb)))
      #f))

#|
variant-set
变体切换系统的重载函数，将伪变体标识映射到具体的括号符号对

语法
----
(variant-set t v)

参数
----
t : tree
必须是 around 或 around* 节点，表示一个括号结构
v : symbol
变体标识符，如 parentheses, brackets, braces 等（定义在 bracket-variant-alist 中）

返回值
----
不返回

逻辑
----
1. 在 bracket-variant-alist 中查找变体标识 v
2. 如果找到对应的括号符号对，调用 bracket-set-pair 设置节点的左右括号

注意
----
此函数是括号变体系统的一部分，用于切换括号的显示样式。
是通用变体系统 variant-set 针对 around/around* 节点的重载实现
|#
(tm-define (variant-set t v)
  ;; 变体切换系统的重载函数
  ;; 将伪变体标识映射到具体的括号符号对，调用 bracket-set-pair
  (:require (tree-in? t '(around around*)))
  (let ((pair (assoc v bracket-variant-alist)))
    (when pair
      (bracket-set-pair t (cadr pair) (caddr pair)))))

(tm-define (kbd-paste)
  (:require (in-math?))
  ;; 在数学模式下粘贴
  ;; 如果剪贴板中的内容是图像，调用 ocr-paste
  ;; 否则调用原来的 kbd-paste
  (if (string-starts? (qt-clipboard-format) "image")
      (ocr-paste)
      (clipboard-paste "primary")))
