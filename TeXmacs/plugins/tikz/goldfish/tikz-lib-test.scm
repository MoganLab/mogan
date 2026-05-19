;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tikz-lib-test.scm
;; DESCRIPTION : Tests for TikZ library functions
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;                   2026  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (scheme process-context) (liii path) (liii string))
(load "./tikz-lib.scm")
(import (tikz lib))

(define passed 0)
(define failed 0)

(define (check-equal? name actual expected)
  (if (equal? actual expected)
    (begin
      (set! passed (+ passed 1))
      (display (string-append "PASS: " name "\n"))
    ) ;begin
    (begin
      (set! failed (+ failed 1))
      (display (string-append "FAIL: "
                 name
                 "\n  expected: "
                 (object->string expected)
                 "\n  actual:   "
                 (object->string actual)
                 "\n"
               ) ;string-append
      ) ;display
    ) ;begin
  ) ;if
) ;define

(define (check-true name expr)
  (check-equal? name expr #t)
) ;define

;; Test escape-string
(check-equal? "escape-string plain" (escape-string "hello") "hello")
(check-equal? "escape-string quote" (escape-string "a\"b") "a\\\"b")
(check-equal? "escape-string backslash" (escape-string "a\\b") "a\\\\b")
(check-equal? "escape-string mixed"
  (escape-string "\\\"hello\\\"")
  "\\\\\\\"hello\\\\\\\""
) ;check-equal?

;; Test goldfish-quote
(check-equal? "goldfish-quote plain" (goldfish-quote "hello") "\"hello\"")
(check-equal? "goldfish-quote with quote" (goldfish-quote "a\"b") "\"a\\\"b\"")
(check-equal? "goldfish-quote with backslash"
  (goldfish-quote "a\\b")
  "\"a\\\\b\""
) ;check-equal?

;; Test wrap-tikz-code
(let ((wrapped (wrap-tikz-code "\\draw (0,0) -- (1,1);")))
  (check-true "wrap-tikz-code contains documentclass"
    (string-contains? wrapped "\\documentclass[tikz,border=10pt]{standalone}")
  ) ;check-true
  (check-true "wrap-tikz-code contains usepackage"
    (string-contains? wrapped "\\usepackage{tikz}")
  ) ;check-true
  (check-true "wrap-tikz-code contains begin document"
    (string-contains? wrapped "\\begin{document}")
  ) ;check-true
  (check-true "wrap-tikz-code contains end document"
    (string-contains? wrapped "\\end{document}")
  ) ;check-true
  (check-true "wrap-tikz-code contains user code"
    (string-contains? wrapped "\\draw (0,0) -- (1,1);")
  ) ;check-true
) ;let

;; Test gen-temp-path
(let ((p (gen-temp-path)))
  (check-true "gen-temp-path is string" (string? p))
  (check-true "gen-temp-path is non-empty" (> (string-length p) 0))
  (check-true "gen-temp-path contains /tikz/" (string-contains? p "/tikz/"))
) ;let

(display (string-append "\nTotal: "
           (number->string (+ passed failed))
           " tests, "
           (number->string passed)
           " passed, "
           (number->string failed)
           " failed.\n"
         ) ;string-append
) ;display

(if (> failed 0) (exit 1) (exit 0))
