;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0635.scm
;; DESCRIPTION : Integration tests for table cells with rowspan / compute_env_rects safety
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-multirow-tmu-loading)
  (display "Verifying loading of tmu file containing rowspan table cells...\n")
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0635.tmu") (dummy (load-buffer tmu-path)))
    ;; Verify that the buffer is loaded successfully and is a valid buffer
    (check (buffer-exists? tmu-path) => #t)
  ) ;let*
) ;define

(tm-define (test_0635) (test-multirow-tmu-loading) (check-report))
