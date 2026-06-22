;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0801.scm
;; DESCRIPTION : Tests for Julia plugin integration and session execution
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
(load (string-append (getenv "TEXMACS_PATH") "/plugins/julia/progs/code/julia-lang.scm"))

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
      (os-call (string-append "cmd.exe /c \"" cmd "\""))
      (os-call (string-append "/bin/sh -c '" cmd "'"))))

;; Helper to write lines physically using cross-platform shell echo
(define (write-physical-input filepath lines first?)
  (unless (null? lines)
    (let ((op (if first? " > " " >> ")))
      (if (os-windows?)
          (os-call (string-append "cmd.exe /c \"echo " (car lines) op filepath "\""))
          (os-call (string-append "/bin/sh -c 'echo \"" (car lines) "\"" op filepath "'")))
      (write-physical-input filepath (cdr lines) #f))))

;; 1. Scheme-side unit tests
(define (test-julia-scheme-side)
  (check (julia-serialize "julia" "1 + 2") => "1 + 2\n<EOF>\n")
  (check (list? (parser-feature "julia" "keyword")) => #t)
  (check (list? (parser-feature "julia" "operator")) => #t)
)

;; 2. Session execution integration tests
(define (test-julia-session)
  (if (url-exists-in-path? "julia")
      (let* ((temp-dir (os-temp-dir))
             (input-path (url->system (system->url (string-append temp-dir "/0801_input.txt"))))
             (output-path (url->system (system->url (string-append temp-dir "/0801_output.txt"))))
             (julia-script (get-system-path "/plugins/julia/julia/julia.jl"))
             (input-lines (list "1 + 2"
                                "<EOF>"
                                "? readdir"
                                "<EOF>"
                                "[1.0 1.0; 1.0 1.0]"
                                "<EOF>"
                                "sqrt(-1.0)"
                                "<EOF>"
                                "nonexistent_variable_0801"
                                "<EOF>"
                                ";echo \\\"hello from shell\\\""
                                "<EOF>"
                                "using Markdown"
                                "Markdown.parse(\\\"hello **world**\\\")"
                                "<EOF>")))
        
        ;; Clean up old files if they exist
        (when (physical-file-exists? input-path) (physical-remove input-path))
        (when (physical-file-exists? output-path) (physical-remove output-path))

        ;; Write input commands physically using shell echo helper
        (write-physical-input input-path input-lines #t)

        ;; Execute the julia session via cross-platform shell command
        (let* ((julia-bin (if (os-windows?) "julia" "env -u LD_LIBRARY_PATH julia"))
               (cmd (string-append julia-bin " --startup-file=no " julia-script " < " input-path " > " output-path " 2>&1")))
          (run-shell-command cmd))

        ;; Read the output file physically
        (let ((output (read-physical-file output-path)))
          ; (display "ACTUAL SESSION OUTPUT (0801): ") (display output) (newline)
          ;; Assertions based on "How to Test" in 0801.md
          (check (string-contains? output "verbatim:3") => #t)
          (check (string-contains? output "verbatim:HELP:") => #t)
          (check (string-contains? output "readdir") => #t)
          (check (string-contains? output "Matrix{Float64}") => #t)
          (check (string-contains? output "DomainError") => #t)
          (check (string-contains? output "UndefVarError") => #t)
          (check (string-contains? output "hello from shell") => #t)
          (check (string-contains? output "hello <strong>world</strong>") => #t)
        ) ;let

        ;; Clean up
        (physical-remove input-path)
        (physical-remove output-path)
      ) ;let
      (display "Skipping physical Julia session tests because Julia is not in the path.\n")
  ) ;if
) ;define

;;; ========== 测试入口 ==========

(tm-define (test_0801)
  (test-julia-scheme-side)
  (test-julia-session)
  (check-report))
