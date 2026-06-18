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

(define (regtest-while)
  (regression-test-group "while(*) operator of packrat"
    "operators-packrat-while"
    (test-symbol "Spc")
    :none
    (test "no space" '("" (0)) '((0) #t ()))
    (test "one space, cursor before" '(" " (0)) '((1) #t ()))
    (test "one space, cursor after" '(" " (1)) '((1) #t ()))
    (test "one space as tree subnode, cursor after"
      '((concat " ") (1))
      '((0 1) #t ())
    ) ;test
    (test "multiple space, cursor in" '("     " (2)) '((5) #t ((Spc (0) (5)))))
  ) ;regression-test-group
) ;define

(define (regtest-repeat)
  (regression-test-group "repeat(+) operator of packrat"
    "operators-packrat-repeat"
    (test-symbol "Space")
    :none
    (test "no space" '("" (0)) '(rejected #f ()))
    (test "one space" '(" " (0)) '((1) #t ()))
    (test "multiple space" '("     " (1)) '((5) #t ((Space (0) (5)))))
  ) ;regression-test-group
) ;define

(define (regtest-sequential)
  (regression-test-group "sequential of operators in packrat"
    "operators-packrat-sequential"
    (test-symbol "End")
    :none
    (test "no space" '(";" (0)) '((1) #t ()))
    (test "one space, cursor after space" '(" ;" (1)) '((2) #t ((End (0) (2)))))
    (test "multiple space, cursor inside space"
      '("     ;" (1))
      '((6) #t ((Spc (0) (5)) (End (0) (6))))
    ) ;test
    (test "not end" '("     a" (0)) '(rejected #f ()))
  ) ;regression-test-group
) ;define

(define (regtest-combination)
  (regression-test-group "prefix operator of packrat"
    "operators-packrat-prefix"
    (test-symbol "If-prefix")
    :none
    (test "no space" '("if" (0)) '(rejected #f ()))
    (test "one space, cursor before" '("if " (0)) '((3) #t ()))
    (test "one space, cursor on if" '("if " (1)) '((3) #t ((If-prefix (0) (3)))))
    (test "multiple space, cursor on space"
      '("if     " (5))
      '((7) #t ((Space (2) (7)) (If-prefix (0) (7))))
    ) ;test
  ) ;regression-test-group
) ;define

(define (regtest-or)
  (regression-test-group "or(or) operator of packrat"
    "operators-packrat-or"
    (test-symbol "Relation-infix")
    :none
    (test "no space" '("!=" (0)) '((2) #t ()))
    (test "one space, cursor before" '(" != " (0)) '((4) #t ()))
    (test "one space, cursor before" '(" = " (0)) '((3) #t ()))
    (test "one space, cursor before" '(" <less> " (0)) '((8) #t ()))
    (test "one space as tree surround, cursor before"
      '((surround " " " " "<less>") (0))
      '((1 1) #t ())
    ) ;test
    (test "one space, cursor after"
      '(" != " (3))
      '((4) #t ((Relation-infix (0) (4))))
    ) ;test
  ) ;regression-test-group
) ;define

(define (regtest-except)
  (regression-test-group "except(except) operator of packrat"
    "operators-packrat-except"
    (test-symbol "Error-curly")
    :none
    (test "any char" '("char" (1)) '((4) #t ((Error-curly (0) (4)))))
    (test "matched curly" '("{{char}}" (0)) '((8) #t ()))
    (test "matched curly, cursor inside"
      '("{{char}}" (3))
      '((8)
        #t
        ((Error-curly (2) (6)) (Error-curly (1) (7)) (Error-curly (0) (8))))
    ) ;test
    (test "unmatched curly 1" '("{{char}}}" (0)) '(rejected #f ()))
    (test "unmatched curly 2" '("{{char}" (0)) '(rejected #f ()))
  ) ;regression-test-group
) ;define

(define (regtest-range)
  (regression-test-group "range(-) operator of packrat"
    "operators-packrat-range"
    (test-symbol "Identifier")
    :none
    (test "lower case" '("char" (1)) '((4) #t ((Identifier (0) (4)))))
    (test "mixed case" '("cHaR" (1)) '((4) #t ((Identifier (0) (4)))))
    (test "non alphabet" '("cH?aR" (1)) '(rejected #f ()))
  ) ;regression-test-group
) ;define

(tm-define (regtest-minimal)
  (let ((n (+ (regtest-while)
             (regtest-repeat)
             (regtest-sequential)
             (regtest-combination)
             (regtest-or)
             (regtest-except)
             (regtest-range)
           ) ;+
        ) ;n
       ) ;
    (display* "Total: " (object->string n) " tests.\n")
    (display "Test suite of packrat: ok\n")
  ) ;let
) ;tm-define
