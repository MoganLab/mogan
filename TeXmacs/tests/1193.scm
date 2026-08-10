;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1193.scm
;; DESCRIPTION : 回归测试：cite 插件模块命名空间规范化后的 cite-sort 排序逻辑。
;; COPYRIGHT   : (C) 2023  jingkaimori
;;               (C) 2026  Mogan STEM
;;
;; PURPOSE
;;   [1193] cite 插件模块从 (contrib cite cite-sort) 规范化为 (cite cite-sort)。
;;   本测试钉死 indice-sort 的排序与合并行为（合并 contiguous range、排序去重），
;;   挡 compare-cite-keys 忽略 comparator 形参直接调 compare-string 的 bug。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 1193        # headless，纯逻辑秒级反馈
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/plugins/cite/progs/cite/cite-sort.scm")

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for indice-sort
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-indice-sort)
  ;; 非连续键（1,2,4,5）：2 和 4 之间有 gap，不应合并。
  (check (indice-sort '(("1" (concat (write "bib1") (reference "bib1")))
                        ("2" (concat (write "bib2") (reference "bib2")))
                        ("4" (concat (write "bib4") (reference "bib4")))
                        ("5" (concat (write "bib5") (reference "bib5"))))
         ) ;indice-sort
    =>
    '((concat (write "bib1") (reference "bib1"))
      (concat (write "bib2") (reference "bib2"))
      (concat (write "bib4") (reference "bib4"))
      (concat (write "bib5") (reference "bib5")))
  ) ;check

  ;; 非连续键，无相邻关系。
  (check (indice-sort '(("1" (concat (write "bib1") (reference "bib1")))
                        ("3" (concat (write "bib3") (reference "bib3"))))
         ) ;indice-sort
    =>
    '((concat (write "bib1") (reference "bib1"))
      (concat (write "bib3") (reference "bib3")))
  ) ;check

  ;; 混合：3,4,5 应合并为范围 [3-5]，1 和 7 独立。
  (check (indice-sort '(("1" (concat (write "bib1") (reference "bib1")))
                        ("3" (concat (write "bib3") (reference "bib3")))
                        ("4" (concat (write "bib4") (reference "bib4")))
                        ("5" (concat (write "bib5") (reference "bib5")))
                        ("7" (concat (write "bib7") (reference "bib7"))))
         ) ;indice-sort
    =>
    '((concat (write "bib1") (reference "bib1"))
      (concat (write "bib3")
        (write "bib4")
        (write "bib5")
        (reference "bib3")
        "-"
        (reference "bib5"))
      (concat (write "bib7") (reference "bib7")))
  ) ;check

  ;; 连续键（1,2,3）：应合并为一个范围。
  (check (indice-sort '(("1" (concat (write "bib1") (reference "bib1")))
                        ("2" (concat (write "bib2") (reference "bib2")))
                        ("3" (concat (write "bib3") (reference "bib3"))))
         ) ;indice-sort
    =>
    '((concat (write "bib1")
        (write "bib2")
        (write "bib3")
        (reference "bib1")
        "-"
        (reference "bib3")))
  ) ;check

  ;; 乱序输入（3,4,2,5,1）：sort 后应为 1,2,3,4,5 连续，合并为一个范围。
  (check (indice-sort '(("1" (concat (write "bib1") (reference "bib1")))
                        ("3" (concat (write "bib3") (reference "bib3")))
                        ("4" (concat (write "bib4") (reference "bib4")))
                        ("2" (concat (write "bib2") (reference "bib2")))
                        ("5" (concat (write "bib5") (reference "bib5"))))
         ) ;indice-sort
    =>
    '((concat (write "bib1")
        (write "bib2")
        (write "bib3")
        (write "bib4")
        (write "bib5")
        (reference "bib1")
        "-"
        (reference "bib5")))
  ) ;check
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (test_1193)
  (test-indice-sort)
  (check-report)
  (quit-TeXmacs)
) ;tm-define
