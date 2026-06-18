
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tmtm-brackets.scm
;; DESCRIPTION : add missing brackets
;; COPYRIGHT   : (C) 2002  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (convert rewrite tmtm-brackets))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Transform into old-style brackets
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (large-bracket l s)
  (cond ((nstring? s) `(,l ,"."))
        ((<= (string-length s) 1) `(,l ,s))
        ((== s "<nobracket>") `(,l ,"."))
        ((and (string-starts? s "<") (string-ends? s ">"))
         `(,l ,(substring s 1 (- (string-length s) 1)))
        ) ;
        (else `(,l ,"."))
  ) ;cond
) ;tm-define

(tm-define (small-bracket s)
  (cond ((or (func? s 'left) (func? s 'mid) (func? s 'right) (func? s 'big))
         (small-bracket (cadr s))
        ) ;
        ((nstring? s) "<nobracket>")
        ((== s ".") "<nobracket>")
        ((<= (string-length s) 1) s)
        (else (string-append "<" s ">"))
  ) ;cond
) ;tm-define

(tm-define (downgrade-brackets t)
  (if (tree? t)
    (tree-downgrade-brackets t #f #f)
    (tree->stree (tree-downgrade-brackets t #f #f))
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Bracket matching
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (contains-tags? t l)
  (cond ((or (nlist? t) (null? t)) #f)
        ((in? (car t) l) #t)
        (else (with found?
                #f
                (for-each (lambda (x) (set! found? (or found? (contains-tags? x l)))) t)
                found?
              ) ;with
        ) ;else
  ) ;cond
) ;define

(define (tmtm-match-brackets-sub l level)
  ;; Returns (l' level') with
  ;;   l     : list to complete
  ;;   level : unmatched number of left brackets
  ;;   l'    : completed list
  ;;   level': unmatched number of right brackets
  (cond ((null? l) (list (make-list level '(right ".")) 0))
        ((func? (car l) 'left 1)
         (let ((result (tmtm-match-brackets-sub (cdr l) (+ level 1))))
           (list (cons (car l) (car result)) (cadr result))
         ) ;let
        ) ;
        ((and (func? (car l) 'right 1) (> level 0))
         (let ((result (tmtm-match-brackets-sub (cdr l) (- level 1))))
           (list (cons (car l) (car result)) (cadr result))
         ) ;let
        ) ;
        ((func? (car l) 'right 1)
         (let ((result (tmtm-match-brackets-sub (cdr l) 0)))
           (list (cons (car l) (car result)) (+ (cadr result) 1))
         ) ;let
        ) ;
        (else (let ((result (tmtm-match-brackets-sub (cdr l) level)))
                (list (cons (car l) (car result)) (cadr result))
              ) ;let
        ) ;else
  ) ;cond
) ;define

(define (tmtm-match-brackets-concat l)
  (let ((result (tmtm-match-brackets-sub l 0)))
    (append (make-list (cadr result) '(left ".")) (car result))
  ) ;let
) ;define

(define (tmtm-match-brackets-bis l)
  (cond ((npair? l) l)
        ((and (== (car l) 'concat) (or (contains-tags? l '(right left))))
         (let ((complete (tmtm-match-brackets-concat (cdr l))))
           (cons 'concat (map tmtm-match-brackets-bis complete))
         ) ;let
        ) ;
        ((or (func? l 'around 3) (func? l 'around* 3) (func? l 'big-around 2))
         (tmtm-match-brackets (downgrade-brackets l))
        ) ;
        (else (cons (car l) (map tmtm-match-brackets (cdr l))))
  ) ;cond
) ;define

(tm-define (tmtm-match-brackets l)
  "Add missing brackets to TeXmacs stree @l."
  (cond ((npair? l) l)
        ((func? l 'left 1) `(concat ,l (right ".")))
        ((func? l 'right 1) `(concat (left ".") ,l))
        (else (tmtm-match-brackets-bis l))
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Utility functions for big operators
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (big-name b)
  (and (func? b 'big-around 2) (cadr (large-bracket 'big (cadr b))))
) ;tm-define

(tm-define (big-subscript b)
  (and (func? b 'big-around 2)
    (with cc
      (caddr b)
      (and (func? cc 'concat)
        (>= (length cc) 3)
        (or (and (func? (cadr cc) 'rsub 1) (cadr (cadr cc)))
          (and (func? (cadr cc) 'rsup 1) (func? (caddr cc) 'rsub 1) (cadr (caddr cc)))
        ) ;or
      ) ;and
    ) ;with
  ) ;and
) ;tm-define

(tm-define (big-supscript b)
  (and (func? b 'big-around 2)
    (with cc
      (caddr b)
      (and (func? cc 'concat)
        (>= (length cc) 3)
        (or (and (func? (cadr cc) 'rsup 1) (cadr (cadr cc)))
          (and (func? (cadr cc) 'rsub 1) (func? (caddr cc) 'rsup 1) (cadr (caddr cc)))
        ) ;or
      ) ;and
    ) ;with
  ) ;and
) ;tm-define

(define (remove-scripts l)
  (cond ((null? l) l)
        ((func? (car l) 'rsub 1) (remove-scripts (cdr l)))
        ((func? (car l) 'rsup 1) (remove-scripts (cdr l)))
        (else l)
  ) ;cond
) ;define

(tm-define (big-body b)
  (and (func? b 'big-around 2)
    (with cc
      (caddr b)
      (if (not (func? cc 'concat))
        cc
        (with l
          (remove-scripts (cdr cc))
          (cond ((null? l) "")
                ((list-1? l) (car l))
                (else `(concat ,@l))
          ) ;cond
        ) ;with
      ) ;if
    ) ;with
  ) ;and
) ;tm-define
