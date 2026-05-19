
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tikz-test.scm
;; DESCRIPTION : Test suite for TikZ plugin
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;                   2026  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (tikz tikz-test))

(use-modules (binary tikz))
(use-modules (code tikz-lang))

(define (regtest-tikz-binary)
  (regression-test-group
    "tikz" "binary"
    (lambda (args) (boolean? (has-binary-tikz?))) :none
    (test "has-binary-tikz? returns boolean" '() #t))
)

(define (regtest-tikz-lang)
  (regression-test-group
    "tikz" "language"
    (lambda (args) (apply parser-feature args)) :none
    (test "parser-feature keyword"
          '("tikz" "keyword")
          '(keyword
             "draw" "fill" "path" "node" "coordinate"
             "tikzpicture" "end" "begin"
             "foreach" "def" "let" "if" "else" "fi"
             "scope" "endscope"
             "pgfdeclarelayer" "pgfsetlayers"
             "usetikzlibrary" "usepgflibrary"
             "definecolor" "colorlet"))
    (test "parser-feature string"
          '("tikz" "string")
          '(string (bool_features)
             (escape_sequences "\\" "\"" "'" "b" "f" "n" "r" "t"))))
)

(tm-define (regtest-tikz)
  (let ((n (+ (regtest-tikz-binary)
              (regtest-tikz-lang))))
    (display* "Total: " (object->string n) " tests.\n")
    (display "Test suite of TikZ plugin: ok\n")))
