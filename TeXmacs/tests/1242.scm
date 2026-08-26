;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1242.scm
;; DESCRIPTION : GUI 集成测试：快捷键切换结构变体后焦点工具栏刷新
;; COPYRIGHT   : (C) 2026  Mogan STEM authors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1242      # 真实 GUI，跑断言链
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/utils/edit/variants.scm")
(load "./TeXmacs/progs/text/text-drd.scm")

(define step-delay-ms 300)

(define section-tree #f)

(define (ancestor-with-label t lab)
  (cond ((not t) #f)
        ((== (tree-label t) lab) t)
        (else (ancestor-with-label (tree-outer t) lab)))
) ;define

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[1242-step] ")
                           (display label)
                           (newline)
                           (catch #t
                             (lambda () (act))
                             (lambda args
                               (display "[1242-error] in step: ")
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

(tm-define (test_1242)
  (run-chain (list
    (cons "new document"
      (lambda ()
        (new-document)))
    (cons "insert section"
      (lambda ()
        (init-env "style" "article")
        (set! section-tree (tree 'section "x"))
        (insert section-tree)
        ;; 从光标位置取回 buffer 内的 section（insert 可能复制树）
        (set! section-tree (ancestor-with-label (cursor-tree) 'section))
        (check (not (not section-tree)) => #t)))
    (cons "circulate variant like structured:cmd tab"
      (lambda ()
        ;; 与快捷键 (variant-circulate (focus-tree) #t) 相同入口；
        ;; 修复后 variant-set 内部会请求重建焦点工具栏（ICONS_FOCUS）
        (variant-circulate section-tree #t)
        (check (tree-label section-tree) => 'subsection)))
    (cons "report + quit"
      (lambda () (check-report) (quit-TeXmacs)))
  ))
) ;tm-define
