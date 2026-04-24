
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 203_1.scm
;; DESCRIPTION : Tests for texout !option handling
;; COPYRIGHT   : (C) 2024  JimZhouZZY
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;; Test !option in top-level (should be ignored)
(check (serialize-latex '(!option "test")) => "")

;; Test !option with argument  
(check (serialize-latex '(section (!option "short") "title")) => "\\section[short]{title}")
(check (serialize-latex '(cite (!option "page 42") "reference")) => "\\cite[page 42]{reference}")

;; Test normal command
(check (serialize-latex '(section "title")) => "\\section{title}")

;; Test empty !option
(check (serialize-latex '(marginpar (!option "") "note")) => "\\marginpar[]{note}")

;; Test multiple !options
(check (serialize-latex '(somecommand (!option "opt1") (!option "opt2") "arg")) => "\\somecommand[opt1][opt2]{arg}")

;; Test !option as command name (should be ignored)
(check (serialize-latex '((!option "ignored") "arg")) => "{arg}")

;; Proof environment tests (Issue #3007)
(let ((out (serialize-latex '((!begin "proof") "Test content"))))
  (check (string-contains? out "\\begin{proof}") => #t)
  (check (string-contains? out "\\end{proof}") => #t)
  (check (string-contains? out "Test content") => #t)
  ;; Regression protection: Ensure no old formatting
  (check (string-contains? out "\\noindent\\textbf{Proof") => #f))

(let ((out (serialize-latex '((!begin "proof" (!option "Sketch")) "Test content"))))
  (check (string-contains? out "\\begin{proof}[Sketch]") => #t)
  (check (string-contains? out "\\end{proof}") => #t)
  (check (string-contains? out "Test content") => #t)
  ;; Regression protection: Ensure no old formatting
  (check (string-contains? out "\\noindent\\textbf{Proof") => #f))

(define (test_203_1)
  (check-report))
