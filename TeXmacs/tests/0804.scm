;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0804.scm
;; DESCRIPTION : Tests for Julia symbolic calculations and LaTeX formatting
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (scheme base)
        (scheme file)
        (liii check)
        (liii os)
        (liii string))

(load (string-append (getenv "TEXMACS_PATH") "/plugins/julia/progs/init-julia.scm"))

(check-set-mode! 'report-failed)

;; Helper function to read a physical file into a string
(define (read-physical-file filepath)
  (let ((port (open-input-file filepath)))
    (let loop ((chars '()) (c (read-char port)))
      (if (eof-object? c)
          (begin
            (close-input-port port)
            (list->string (reverse chars)))
          (loop (cons c chars) (read-char port))))))

;; Helper to check if a physical file exists
(define (physical-file-exists? filepath)
  (catch #t
    (lambda ()
      (let ((port (open-input-file filepath)))
        (close-input-port port)
        #t))
    (lambda args #f)))

;; Helper to remove a physical file
(define (physical-remove filepath)
  (catch #t
    (lambda ()
      (remove filepath)
      #t)
    (lambda args #f)))

;; Helper to resolve platform-specific absolute system path
(define (get-system-path relative-path)
  (url->system (system->url (string-append (getenv "TEXMACS_PATH") relative-path))))

;; Helper to execute commands cross-platform using the correct system shell
(define (run-shell-command cmd)
  (if (os-windows?)
      (os-call cmd)
      (os-call (string-append "/bin/sh -c '" cmd "'"))))

;; Helper to write lines physically using Scheme IO
(define (write-physical-input filepath lines first?)
  (if first?
      (when (physical-file-exists? filepath) (physical-remove filepath)))
  (let ((port (open-output-file filepath)))
    (let loop ((rest lines))
      (unless (null? rest)
        (display (car rest) port)
        (newline port)
        (loop (cdr rest))))
    (close-output-port port)))

;; Check if Symbolics Julia packages are available
(define (julia-packages-available?)
  (and (supports-julia?)
       (let* ((julia-path (url->system (find-binary-julia)))
              (julia-bin (if (os-windows?) julia-path (string-append "env -u LD_LIBRARY_PATH " julia-path))))
         (== (run-shell-command (string-append julia-bin " --startup-file=no -e \"using Symbolics, Latexify, LaTeXStrings\"")) 0))))

;; Session execution symbolic math tests
(define (test-julia-symbolics)
  (if (julia-packages-available?)
      (let* ((temp-dir (os-temp-dir))
             (input-path (url->system (system->url (string-append temp-dir "/0804_input.txt"))))
             (output-path (url->system (system->url (string-append temp-dir "/0804_output.txt"))))
             (julia-script (get-system-path "/plugins/julia/julia/julia.jl"))
             (input-lines (list "using Symbolics, Latexify, LaTeXStrings"
                                "<EOF>"
                                "@variables x"
                                "x^2"
                                "<EOF>"
                                "@variables a b c"
                                "formula = (a + b)^2 / c"
                                "<EOF>"
                                "@variables a b"
                                "[a^2, b]"
                                "<EOF>"
                                "@variables a b"
                                "(a^2, b)"
                                "<EOF>"
                                "@variables a b c d"
                                "M = [a b; c d]"
                                "<EOF>"
                                "@variables a b"
                                "Dict(a => b)"
                                "<EOF>"
                                "L\"\\int_{0}^{\\infty} e^{-x^2} dx = \\frac{\\sqrt{\\pi}}{2}\""
                                "<EOF>"
                                "@variables x"
                                "D = Differential(x)"
                                "expand_derivatives(D(sin(x) * x^2))"
                                "<EOF>"
                                "@variables x y"
                                "expr = (x^2 - y^2) / (x - y)"
                                "simplify(expr)"
                                "<EOF>"
                                "@variables x y"
                                "expr = (x + y)^2"
                                "expand(expr)"
                                "<EOF>"
                                "1 + 2"
                                "<EOF>"
                                "[1, 2, 3]"
                                "<EOF>")))
        
        ;; Clean up old files if they exist
        (when (physical-file-exists? input-path) (physical-remove input-path))
        (when (physical-file-exists? output-path) (physical-remove output-path))

        ;; Write input commands physically
        (write-physical-input input-path input-lines #t)

        ;; Execute the julia session via cross-platform shell command
        (let* ((julia-path (url->system (find-binary-julia)))
               (julia-bin (if (os-windows?) julia-path (string-append "env -u LD_LIBRARY_PATH " julia-path)))
               (cmd (string-append julia-bin " --startup-file=no " julia-script " < " input-path " > " output-path " 2>&1")))
          (run-shell-command cmd))

        ;; Read the output file physically
        (let ((output (read-physical-file output-path)))
          ;; Assertions based on "How to Test" in 0804.md
          (check (string-contains? output "latex:\\begin{equation*}") => #t)
          (check (string-contains? output "x^{2}") => #t)
          (check (string-contains? output "\\frac{\\left( a + b \\right)^{2}}{c}") => #t)
          (check (string-contains? output "a^{2} \\\\") => #t)
          (check (string-contains? output "b \\\\") => #t)
          (check (string-contains? output "a & b \\\\") => #t)
          (check (string-contains? output "c & d \\\\") => #t)
          (check (string-contains? output "latex:$\\rmfamily{\\int_{0}^{\\infty} e^{-x^2} dx = \\frac{\\sqrt{\\pi}}{2}}$") => #t)
          (check (string-contains? output "2 ~ x ~ \\sin\\left( x \\right)") => #t)
          (check (string-contains? output "x + y") => #t)
          (check (string-contains? output "x^{2} + 2 ~ x ~ y + y^{2}") => #t)
          (check (string-contains? output "verbatim:3") => #t)
          (check (string-contains? output "3-element Vector{Int64}:") => #t)
        ) ;let

        ;; Clean up
        (physical-remove input-path)
        (physical-remove output-path)
      ) ;let
      (display "Skipping physical Julia symbolic tests because Julia or required packages are not supported.\n")
  ) ;if
) ;define

;;; ========== 测试入口 ==========

(tm-define (test_0804)
  (test-julia-symbolics)
  (check-report))
