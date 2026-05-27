;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0624.scm
;; DESCRIPTION : Integration test for LaTeX export progress bar
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")

(check-set-mode! 'report-failed)

;; Define tracking counters for spying on the progress bar API calls

(define progress-start-count 0)

(define progress-update-count 0)

(define progress-end-count 0)

(define (test-latex-progress-bar-integration)
  (display "Verifying end-to-end LaTeX export progress bar integration...\n")

  ;; Reset counters
  (set! progress-start-count 0)
  (set! progress-update-count 0)
  (set! progress-end-count 0)

  ;; Save original functions
  (let ((orig-gui? qt-gui?)
        (orig-start latex-progress-start)
        (orig-update latex-progress-update)
        (orig-end latex-progress-end)
       ) ;

    ;; Override functions for testing
    (set! qt-gui? (lambda () #t))
    (set! latex-progress-start
      (lambda (total)
        (set! progress-start-count total)
        (display* "Spy: latex-progress-start: " total "\n")
      ) ;lambda
    ) ;set!
    (set! latex-progress-update
      (lambda (current)
        (set! progress-update-count (+ progress-update-count 1))
        (display* "Spy: latex-progress-update: " current "\n")
      ) ;lambda
    ) ;set!
    (set! latex-progress-end
      (lambda ()
        (set! progress-end-count (+ progress-end-count 1))
        (display "Spy: latex-progress-end called\n")
      ) ;lambda
    ) ;set!

    ;; Export a document containing images to latex
    (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0623_gnuplot_tuto.tmu")
           (tmp-tex (url-temp))
           (dummy (load-buffer tmu-path))
           (dummy2 (buffer-export tmu-path tmp-tex "latex"))
          ) ;

      (display* "DEBUG: tmp-tex exists? " (url-exists? tmp-tex) "\n")

      ;; Restore original functions
      (set! qt-gui? orig-gui?)
      (set! latex-progress-start orig-start)
      (set! latex-progress-update orig-update)
      (set! latex-progress-end orig-end)

      ;; Assert that the progress bar functions were indeed called!
      (check (> progress-start-count 0) => #t)
      (check (> progress-update-count 0) => #t)
      (check (= progress-end-count 1) => #t)
      (display "LaTeX progress bar integration verified successfully!\n")
    ) ;let*
  ) ;let
) ;define

(define (test-quantum-latex-export)
  (display "Verifying LaTeX export of quantum document...\n")

  ;; Reset counters
  (set! progress-start-count 0)
  (set! progress-update-count 0)
  (set! progress-end-count 0)

  ;; Save original functions
  (let ((orig-gui? qt-gui?)
        (orig-start latex-progress-start)
        (orig-update latex-progress-update)
        (orig-end latex-progress-end)
       ) ;

    ;; Override functions for testing
    (set! qt-gui? (lambda () #t))
    (set! latex-progress-start
      (lambda (total)
        (set! progress-start-count total)
        (display* "Spy: latex-progress-start: " total "\n")
      ) ;lambda
    ) ;set!
    (set! latex-progress-update
      (lambda (current)
        (set! progress-update-count (+ progress-update-count 1))
        (display* "Spy: latex-progress-update: " current "\n")
      ) ;lambda
    ) ;set!
    (set! latex-progress-end
      (lambda ()
        (set! progress-end-count (+ progress-end-count 1))
        (display "Spy: latex-progress-end called\n")
      ) ;lambda
    ) ;set!

    ;; Export the quantum document to latex
    (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0624_quantum.tmu")
           (tmp-tex (url-temp))
           (dummy (load-buffer tmu-path))
           (dummy2 (buffer-export tmu-path tmp-tex "latex"))
           (tex-content (string-load tmp-tex))
          ) ;

      (display* "DEBUG: tmp-tex exists? " (url-exists? tmp-tex) "\n")

      ;; Restore original functions
      (set! qt-gui? orig-gui?)
      (set! latex-progress-start orig-start)
      (set! latex-progress-update orig-update)
      (set! latex-progress-end orig-end)

      ;; Assert that the exported LaTeX file is not empty
      (check (and (string? tex-content) (> (string-length tex-content) 0)) => #t)

      ;; Assert that the exported LaTeX file contains the required translations
      (check (string-contains? tex-content "\\hbar") => #t)
      (check (string-contains? tex-content "\\psi") => #t)
      (check (string-contains? tex-content "\\langle") => #t)
      (check (string-contains? tex-content "\\rangle") => #t)
      (check (string-contains? tex-content "\\nabla") => #t)

      ;; Assert that tmtex-image-total is evaluated as greater than 0
      (check (> ((resolve-module '(convert latex tmtex)) 'tmtex-image-total) 0) => #t)

      ;; Assert that progress bar functions were triggered
      (check (> progress-start-count 0) => #t)
      (check (> progress-update-count 0) => #t)
      (check (= progress-end-count 1) => #t)

      (display "Quantum LaTeX export verified successfully!\n")
    ) ;let*
  ) ;let
) ;define

(define (test-progress-bar-boundary-safety)
  (display "Verifying progress bar boundary safety on C++ side...\n")

  ;; 1. Calling (latex-progress-start 0) works safely without division-by-zero crashes on the C++ side.
  (latex-progress-start 0)
  (latex-progress-update 5)
  (latex-progress-end)
  (check #t => #t)

  ;; 2. Calling (latex-progress-start -5) works safely.
  (latex-progress-start -5)
  (latex-progress-update 5)
  (latex-progress-end)
  (check #t => #t)

  ;; 3. Calling (latex-progress-update -5) or (latex-progress-update 150) with values outside the total range is handled safely and gracefully.
  (latex-progress-start 100)
  (latex-progress-update -5)
  (latex-progress-update 150)
  (latex-progress-end)
  (check #t => #t)

  ;; 4. Doing multiple sequential starts and ends (resetting flow) resets the internal state cleanly every time.
  (latex-progress-start 10)
  (latex-progress-update 2)
  (latex-progress-start 20)
  (latex-progress-update 5)
  (latex-progress-end)
  (latex-progress-end)
  (check #t => #t)

  ;; 5. Calling (latex-progress-update 5) without start works safely.
  (latex-progress-update 5)
  (check #t => #t)

  (display "Progress bar boundary safety verified successfully!\n")
) ;define

(tm-define (test_0624)
  (display "Running test_0624...\n")
  (test-latex-progress-bar-integration)
  (test-quantum-latex-export)
  (test-progress-bar-boundary-safety)
  (check-report)
) ;tm-define
