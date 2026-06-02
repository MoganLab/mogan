;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0636.scm
;; DESCRIPTION : Integration test for loading folded-comment without insecure script error
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-comment-tmu-loading)
  (display "Verifying loading of tmu file containing comment macros...\n")
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0636.tmu") (dummy (load-buffer tmu-path)))
    ;; Verify that the buffer is loaded successfully and is a valid buffer
    (check (buffer-exists? tmu-path) => #t)
    ;; Verify that the key comment functions are trusted/secure
    (check (secure? '(ext-comment-color "comment" "Jack")) => #t)
    (check (secure? '(ext-comment-bg-color)) => #t)
    (check (secure? '(ext-abbreviate-name "Jack")) => #t)
    (check (secure? '(ext-contains-shown-comments? "body")) => #t)
    (check (secure? '(mirror-initialize "body")) => #t)
  ) ;let*
) ;define

(tm-define (test_0636) (test-comment-tmu-loading) (check-report))
