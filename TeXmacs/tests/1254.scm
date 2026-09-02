;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1254.scm
;; DESCRIPTION : 集成测试：在 section/subsection/subsubsection 结构内插入同类结构
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;; USAGE
;;   xmake r 1254
;;   MOGAN_TEST_GUI=1 xmake r 1254
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1254))

(import (liii check))
(load "./TeXmacs/progs/text/text-edit.scm")

(check-set-mode! 'report-failed)

(define step-delay-ms 300)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[1254-step] ")
                           (display label)
                           (newline)
                           (catch #t
                             (lambda () (act))
                             (lambda args
                               (display "[1254-error] in step: ")
                               (display label)
                               (display " -> ")
                               (write args)
                               (newline)
                             ) ;lambda
                           ) ;catch
                           (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
                         ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

(define (get-heading-titles tag)
  (let ((st (tree->stree (buffer-tree))))
    (map caddr
         (filter (lambda (item)
                   (and (pair? item)
                        (== (car item) 'concat)
                        (pair? (cdr item))
                        (pair? (cadr item))
                        (== (caadr item) tag)))
                 (cdr st)))))

(tm-define (test_1254)
  (run-chain (list
    (cons "new document"
      (lambda ()
        (new-document)
        (init-env "style" "generic")
      ))
    (cons "insert first section"
      (lambda ()
        (smart-insert-heading 'section)
        (insert "Section 1")
        (check (get-heading-titles 'section) => '("Section 1"))
        (check-true (not (not (current-section-node))))
      ))
    (cons "insert section while cursor inside first section"
      (lambda ()
        ;; 光标在 Section 1 内部，此时插入 section
        (smart-insert-heading 'section)
        (insert "Section 2")
        (check (get-heading-titles 'section) => '("Section 1" "Section 2"))
      ))
    (cons "insert subsection while cursor inside section"
      (lambda ()
        ;; 光标在 Section 2 内部，此时插入 subsection
        (smart-insert-heading 'subsection)
        (insert "Subsection 2.1")
        (check (get-heading-titles 'section) => '("Section 1" "Section 2"))
        (check (get-heading-titles 'subsection) => '("Subsection 2.1"))
      ))
    (cons "insert subsection while cursor inside subsection"
      (lambda ()
        ;; 光标在 Subsection 2.1 内部，此时插入 subsection
        (smart-insert-heading 'subsection)
        (insert "Subsection 2.2")
        (check (get-heading-titles 'subsection) => '("Subsection 2.1" "Subsection 2.2"))
      ))
    (cons "insert subsubsection while cursor inside subsection"
      (lambda ()
        ;; 光标在 Subsection 2.2 内部，此时插入 subsubsection
        (smart-insert-heading 'subsubsection)
        (insert "Subsubsection 2.2.1")
        (check (get-heading-titles 'subsection) => '("Subsection 2.1" "Subsection 2.2"))
        (check (get-heading-titles 'subsubsection) => '("Subsubsection 2.2.1"))
      ))
    (cons "insert section while cursor inside subsubsection"
      (lambda ()
        ;; 光标在 Subsubsection 2.2.1 内部，此时插入 section
        (smart-insert-heading 'section)
        (insert "Section 3")
        (check (get-heading-titles 'section) => '("Section 1" "Section 2" "Section 3"))
        (check (get-heading-titles 'subsubsection) => '("Subsubsection 2.2.1"))
      ))
    (cons "report + quit"
      (lambda ()
        (check-report)
        (quit-TeXmacs)
      ))
  ))
) ;tm-define
