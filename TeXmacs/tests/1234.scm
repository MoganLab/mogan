;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1234.scm
;; DESCRIPTION : 回归测试：Alt+1 智能插入 section / section*
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1234] Alt+1（"text 1" 绑定）插入 section 时，按上下最近的同级标题
;;   决定是否有编号：
;;     - 上下都存在且都是 section*  → 插 section*
;;     - 只有一侧存在，看该侧（该侧为 section* 则插 section*）
;;     - 两侧都不存在，或任一侧为有编号 section → 插 section
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1234      # 真实 GUI，跑断言链
;;   xmake r 1234                       # headless 冒烟（进程不崩即可）
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/text/text-edit.scm")

(check-set-mode! 'report-failed)

;; 统计根文档里指定标签的顶层子树个数

(define (count-tier-label lab)
  (length (filter (lambda (t) (== (tree-label t) lab))
                  (tree-children (root-tree))))
) ;define

;; 造缓冲区：doc 为 tm 字符串，cursor-idx 为光标所在的顶层子树下标

(define (setup-doc doc cursor-idx)
  (new-document)
  (insert (string->tree doc))
  (tree-go-to (root-tree) cursor-idx 0)
) ;define

;; 在指定场景下按 Alt+1 语义插入，断言新出现的标签是 expected

(define (check-alt1 doc cursor-idx expected)
  (setup-doc doc cursor-idx)
  (let* ((before (count-tier-label expected))
         (unused (smart-insert-section))
         (after (count-tier-label expected)))
    (check after => (+ before 1))
  ) ;let*
) ;define

(define (test-alt1-both-unnumbered)
  ;; 上下都是 section* → 插 section*
  (check-alt1 "(document (section* (document \"A\")) \"x\" (section* (document \"B\")))"
    1 'section*)
) ;define

(define (test-alt1-up-numbered)
  ;; 上方为有编号 section → 插 section
  (check-alt1 "(document (section (document \"A\")) \"x\" (section* (document \"B\")))"
    1 'section)
) ;define

(define (test-alt1-down-numbered)
  ;; 下方为有编号 section → 插 section
  (check-alt1 "(document (section* (document \"A\")) \"x\" (section (document \"B\")))"
    1 'section)
) ;define

(define (test-alt1-only-up)
  ;; 只有上方、且为 section* → 插 section*
  (check-alt1 "(document (section* (document \"A\")) \"x\")" 1 'section*)
) ;define

(define (test-alt1-only-down)
  ;; 只有下方、且为有编号 section → 插 section
  (check-alt1 "(document \"x\" (section (document \"B\")))" 0 'section)
) ;define

(define (test-alt1-none)
  ;; 两侧都没有同级标题 → 插有编号 section
  (check-alt1 "(document \"x\")" 0 'section)
) ;define

(define (test-alt1-inside-section-block)
  ;; 光标位于 section* 块内部时，该块本身算上方最近的同级标题 → 插 section*
  (check-alt1 "(document (section* (document \"A\")) (section* (document \"B\")))"
    1 'section*)
) ;define

(tm-define (test_1234)
  (run-chain (list
    (cons "both unnumbered" test-alt1-both-unnumbered)
    (cons "up numbered" test-alt1-up-numbered)
    (cons "down numbered" test-alt1-down-numbered)
    (cons "only up, unnumbered" test-alt1-only-up)
    (cons "only down, numbered" test-alt1-only-down)
    (cons "no neighbors" test-alt1-none)
    (cons "cursor inside section block" test-alt1-inside-section-block)
    (cons "report + quit"
      (lambda () (check-report) (quit-TeXmacs)))
  ))
) ;tm-define
