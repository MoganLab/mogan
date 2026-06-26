;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : graphics-test.scm
;; DESCRIPTION : test suite for graphics-group
;; COPYRIGHT   : (C) 2023 Jia Zhang
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (graphics graphics-group-test) (:use (graphics graphics-group)))

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for get-paste-offset-by-pos
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-get-paste-offset-by-pos)
  (let* ((paste-offset-constant-property (get-preference "paste-offset-constant"))
         (c (if (== paste-offset-constant-property "default")
              0.3
              (string->float paste-offset-constant-property)
            ) ;if
         ) ;c
        ) ;
    (check (get-paste-offset-by-pos '(carc (point "-1" "1")
                                       (point "-2" "2")
                                       (point "-3" "3"))
           ) ;get-paste-offset-by-pos
      =>
      (list (+ c) (- c))
    ) ;check
    (check (get-paste-offset-by-pos '(carc (point "-1" "-1")
                                       (point "-2" "-2")
                                       (point "-3" "-3"))
           ) ;get-paste-offset-by-pos
      =>
      (list (+ c) (+ c))
    ) ;check
    (check (get-paste-offset-by-pos '(carc (point "1" "1")
                                       (point "2" "2")
                                       (point "3" "3"))
           ) ;get-paste-offset-by-pos
      =>
      (list (- c) (- c))
    ) ;check
    (check (get-paste-offset-by-pos '(carc (point "1" "-1")
                                       (point "2" "-2")
                                       (point "3" "-3"))
           ) ;get-paste-offset-by-pos
      =>
      (list (- c) (+ c))
    ) ;check
    (check (get-paste-offset-by-pos '(carc (point "-2" "0")
                                       (point "-1" "2")
                                       (point "3" "-2"))
           ) ;get-paste-offset-by-pos
      =>
      (list (- c) (- c))
    ) ;check
    (check (get-paste-offset-by-pos '(point "1" "0")) => (list (- c) (- c)))
    (check (get-paste-offset-by-pos '(point "0" "1")) => (list (- c) (- c)))
    (check (get-paste-offset-by-pos '(point "-1" "0")) => (list (+ c) (- c)))
    (check (get-paste-offset-by-pos '(point "0" "-1")) => (list (- c) (+ c)))
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-graphics-group)
  (test-get-paste-offset-by-pos)
  (check-report)
) ;tm-define
