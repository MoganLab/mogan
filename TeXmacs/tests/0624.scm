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

(tm-define (test_0624) (test-latex-progress-bar-integration) (check-report))
