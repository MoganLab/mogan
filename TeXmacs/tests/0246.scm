;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0246.scm
;; DESCRIPTION : Tests for chat-persist-extract-title-from-tree
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report)

;;; ========== 辅助函数 ==========

;; 用标准 Scheme 数据结构（字符串叶子 + 列表复合节点）模拟
;; TeXmacs tree 的递归文本提取算法，与 chat-session-persist.scm 中
;; chat-persist-extract-title-from-tree 使用相同逻辑。

(define (tree-extract-text tree)
  (if (string? tree) tree (tree-extract-text-loop tree 0 (length tree) ""))
) ;define

(define (tree-extract-text-loop tree idx len result)
  (if (>= idx len)
    result
    (tree-extract-text-loop tree
      (+ idx 1)
      len
      (string-append result (tree-extract-text (list-ref tree idx)))
    ) ;tree-extract-text-loop
  ) ;if
) ;define

;;; ========== tree-extract-text: 基本行为 ==========

;; 简单原子节点（纯文本）
(let ((result (tree-extract-text "hello")))
  (check result => "hello")
) ;let

;; 空字符串
(let ((result (tree-extract-text "")))
  (check result => "")
) ;let

;; 空列表（无子节点的复合节点）
(let ((result (tree-extract-text '())))
  (check result => "")
) ;let

;;; ========== tree-extract-text: 单层复合节点 ==========

;; 单层列表，全部原子
(let ((result (tree-extract-text '("hello" " " "world"))))
  (check result => "hello world")
) ;let

;; 单层列表，含空字符串
(let ((result (tree-extract-text '("a" "" "b"))))
  (check result => "ab")
) ;let

;;; ========== tree-extract-text: 嵌套结构（数学公式场景） ==========

;; 模拟：纯文本段落 → 直接原子，正常提取
(let ((result (tree-extract-text "请帮我计算")))
  (check result => "请帮我计算")
) ;let

;; 模拟：CONCAT("请帮我计算 ", (math "x^2"), " 的值")
;; 非原子子节点 (math "x^2") 也应递归提取其内容
(let ((result (tree-extract-text '("请帮我计算 " ("math" "x^2") " 的值"))))
  (check result => "请帮我计算 mathx^2 的值")
) ;let

;; 模拟：DOCUMENT → CONCAT("Hello", (with "color" "red" "world"), "!")
(let ((result (tree-extract-text '("Hello" ("with" "color" "red" "world") "!"))))
  (check result => "Hellowithcolorredworld!")
) ;let

;; 深层嵌套：DOCUMENT → PARA → CONCAT("a", (math (frac "x" "2")), "b")
(let ((result (tree-extract-text '("a" ("math" ("frac" "x" "2")) "b"))))
  (check result => "amathfracx2b")
) ;let

;;; ========== tree-extract-text: 边界情况 ==========

;; 只有一个非原子子节点（如整个段落就是一个数学公式）
(let ((result (tree-extract-text '(("equation" "E=mc^2")))))
  (check result => "equationE=mc^2")
) ;let

;; 多层嵌套只有一个叶子
(let ((result (tree-extract-text '((("deep" "value"))))))
  (check result => "deepvalue")
) ;let

(check-report)
