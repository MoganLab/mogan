;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : minimal-test.scm
;; DESCRIPTION : Test suite for packrat using minimal language
;; COPYRIGHT   : (C) 2024  jingkaimori
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (language minimal-test) (:use (language minimal)))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-symbol symbol)
  (lambda (input)
    (let* ((tree (car input))
           (cursor (cadr input))
           (correct (packrat-correct? "minimal" symbol tree))
          ) ;
      (if correct
        `(,(packrat-parse "minimal" symbol tree)
          ,#t
          ,(packrat-context "minimal" symbol tree cursor))
        '(rejected #f ())
      ) ;if
    ) ;let*
  ) ;lambda
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for while(*) operator of packrat
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-while)
  (define f (test-symbol "Spc"))
  (check (f '("" (0))) => '((0) #t ()))
  (check (f '(" " (0))) => '((1) #t ()))
  (check (f '(" " (1))) => '((1) #t ()))
  (check (f '((concat " ") (1))) => '((0 1) #t ()))
  (check (f '("     " (2))) => '((5) #t ((Spc (0) (5)))))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for repeat(+) operator of packrat
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-repeat)
  (define f (test-symbol "Space"))
  (check (f '("" (0))) => '(rejected #f ()))
  (check (f '(" " (0))) => '((1) #t ()))
  (check (f '("     " (1))) => '((5) #t ((Space (0) (5)))))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for sequential of operators in packrat
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-sequential)
  (define f (test-symbol "End"))
  (check (f '(";" (0))) => '((1) #t ()))
  (check (f '(" ;" (1))) => '((2) #t ((End (0) (2)))))
  (check (f '("     ;" (1))) => '((6) #t ((Spc (0) (5)) (End (0) (6)))))
  (check (f '("     a" (0))) => '(rejected #f ()))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for prefix operator of packrat
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-combination)
  (define f (test-symbol "If-prefix"))
  (check (f '("if" (0))) => '(rejected #f ()))
  (check (f '("if " (0))) => '((3) #t ()))
  (check (f '("if " (1))) => '((3) #t ((If-prefix (0) (3)))))
  (check (f '("if     " (5))) => '((7) #t ((Space (2) (7)) (If-prefix (0) (7)))))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for or(or) operator of packrat
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-or)
  (define f (test-symbol "Relation-infix"))
  (check (f '("!=" (0))) => '((2) #t ()))
  (check (f '(" != " (0))) => '((4) #t ()))
  (check (f '(" = " (0))) => '((3) #t ()))
  (check (f '(" <less> " (0))) => '((8) #t ()))
  (check (f '((surround " " " " "<less>") (0))) => '((1 1) #t ()))
  (check (f '(" != " (3))) => '((4) #t ((Relation-infix (0) (4)))))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for except(except) operator of packrat
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-except)
  (define f (test-symbol "Error-curly"))
  (check (f '("char" (1))) => '((4) #t ((Error-curly (0) (4)))))
  (check (f '("{{char}}" (0))) => '((8) #t ()))
  (check (f '("{{char}}" (3)))
    =>
    '((8)
      #t
      ((Error-curly (2) (6)) (Error-curly (1) (7)) (Error-curly (0) (8))))
  ) ;check
  (check (f '("{{char}}}" (0))) => '(rejected #f ()))
  (check (f '("{{char}" (0))) => '(rejected #f ()))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for range(-) operator of packrat
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-range)
  (define f (test-symbol "Identifier"))
  (check (f '("char" (1))) => '((4) #t ((Identifier (0) (4)))))
  (check (f '("cHaR" (1))) => '((4) #t ((Identifier (0) (4)))))
  (check (f '("cH?aR" (1))) => '(rejected #f ()))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-minimal)
  (test-while)
  (test-repeat)
  (test-sequential)
  (test-combination)
  (test-or)
  (test-except)
  (test-range)
  (check-report)
) ;tm-define
