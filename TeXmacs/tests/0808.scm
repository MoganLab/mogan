;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0808.scm
;; DESCRIPTION : Tests for Julia Plots integration and PDF rendering
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

(define (string-contains-from str sub start)
  (let ((len-str (string-length str))
        (len-sub (string-length sub)))
    (let loop ((i start))
      (cond ((> (+ i len-sub) len-str) #f)
            ((string=? (substring str i (+ i len-sub)) sub) i)
            (else (loop (+ i 1)))))))

(define (count-occurrences str sub)
  (let ((len-sub (string-length sub)))
    (let loop ((count 0) (start 0))
      (let ((pos (string-contains-from str sub start)))
        (if pos
            (loop (+ count 1) (+ pos len-sub))
            count)))))

(define (extract-file-paths str)
  (let ((len-str (string-length str)))
    (let loop ((paths '()) (i 0))
      (let ((pos (string-contains-from str "file:" i)))
        (if pos
            (let* ((start (+ pos 5))
                   (end-pos (let find-end ((j start))
                              (if (or (>= j len-str)
                                      (char=? (string-ref str j) (integer->char 5))
                                      (char=? (string-ref str j) #\newline))
                                  j
                                  (find-end (+ j 1))))))
              (loop (cons (substring str start end-pos) paths) end-pos))
            (reverse paths))))))

;; Check if Plots Julia package is available
(define (julia-plots-available?)
  (and (supports-julia?)
       (let* ((julia-path (url->system (find-binary-julia)))
              (julia-bin (if (os-windows?) julia-path (string-append "env -u LD_LIBRARY_PATH -u QT_PLUGIN_PATH " julia-path))))
         (== (run-shell-command (string-append julia-bin " --startup-file=no -e \"using Plots\"")) 0))))

;; Session execution Plots tests
(define (test-julia-plots)
  (if (julia-plots-available?)
      (let* ((temp-dir (os-temp-dir))
             (input-path (url->system (system->url (string-append temp-dir "/0808_input.txt"))))
             (output-path (url->system (system->url (string-append temp-dir "/0808_output.txt"))))
             (julia-script (get-system-path "/plugins/julia/bin/julia.jl"))
             (input-lines (list "using Plots"
                                "<EOF>"
                                "plot(1:10, rand(10))"
                                "<EOF>"
                                "f(x) = x^2 + x * sin(x)"
                                "<EOF>"
                                "plot(f, -5, 5, label=\"f(x) = x^2 + x*sin(x)\", title=\"test_img\", lw=2)"
                                "<EOF>"
                                "x = range(-pi, pi, length=100)"
                                "<EOF>"
                                "y = sin.(x)"
                                "<EOF>"
                                "plot(x, y, label=\"sin(x)\", xlabel=\"x_axis\", ylabel=\"y_axis\", color=:green)"
                                "<EOF>")))

        ;; Clean up old files if they exist
        (when (physical-file-exists? input-path) (physical-remove input-path))
        (when (physical-file-exists? output-path) (physical-remove output-path))

        ;; Write input commands physically
        (write-physical-input input-path input-lines #t)

        ;; Execute the julia session via cross-platform shell command
        (let* ((julia-path (url->system (find-binary-julia)))
               (julia-bin (if (os-windows?) julia-path (string-append "env -u LD_LIBRARY_PATH -u QT_PLUGIN_PATH " julia-path)))
               (cmd (string-append julia-bin " --startup-file=no " julia-script " < " input-path " > " output-path " 2>&1")))
          (run-shell-command cmd))

        ;; Read the output file physically
        (let* ((output (read-physical-file output-path))
               (paths (extract-file-paths output)))
          ;; Assertions: should output using "file:" protocol exactly 3 times with pdf paths
          (check (count-occurrences output "file:") => 3)
          (check (count-occurrences output ".pdf") => 3)
          (check (length paths) => 3)
          (check (and (physical-file-exists? (car paths))
                      (physical-file-exists? (cadr paths))
                      (physical-file-exists? (caddr paths))) => #t)
          ;; Clean up generated PDF files
          (when (physical-file-exists? (car paths)) (physical-remove (car paths)))
          (when (physical-file-exists? (cadr paths)) (physical-remove (cadr paths)))
          (when (physical-file-exists? (caddr paths)) (physical-remove (caddr paths)))
        ) ;let

        ;; Clean up
        (physical-remove input-path)
        (physical-remove output-path)
      ) ;let
      (display "Skipping physical Julia Plots tests because Julia or Plots is not supported.\n")
  ) ;if
) ;define

;;; ========== 测试入口 ==========

(tm-define (test_0808)
  (test-julia-plots)
  (check-report))
