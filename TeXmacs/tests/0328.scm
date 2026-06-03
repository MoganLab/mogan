;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0328.scm
;; DESCRIPTION : Tests for search/replace auxiliary buffer command routing
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/progs/generic/search-widgets.scm")

(check-set-mode! 'report)

(define (test-search-command-target-buffer-routing)
  (let* ((master (buffer-new))
         (search-aux (string->url "tmfs://aux/search/test-0328/window"))
         (replace-aux (string->url "tmfs://aux/replace/test-0328/window"))
         (other-tmfs (string->url "tmfs://misc/test-0328/window"))
         (plain (buffer-new)))

    (check (search-or-replace-aux-buffer? search-aux) => #t)
    (check (search-or-replace-aux-buffer? replace-aux) => #t)
    (check (search-or-replace-aux-buffer? other-tmfs) => #f)

    (check (search-command-target-buffer* search-aux master) => master)
    (check (search-command-target-buffer* replace-aux master) => master)
    (check (search-command-target-buffer* other-tmfs master) => other-tmfs)
    (check (search-command-target-buffer* plain master) => plain)
  ) ;let*
) ;define

(tm-define (test_0328)
  (test-search-command-target-buffer-routing)
  (check-report))
