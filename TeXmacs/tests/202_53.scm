;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 202_53.scm
;; DESCRIPTION : Test window decorations for word count statistics dialog
;; COPYRIGHT   : (C) 2025  TeXmacs Development Team
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (data tmu) (texmacs texmacs tm-tools))

(import (liii check) (liii path))

(tm-define (test_202_53)
  (let* ((tmu-path "TeXmacs/tests/tmu/202_53.tmu")
         (test-doc (tmu->texmacs (path-read-text tmu-path)))
         (non-chinese (count-words test-doc))
         (chars-no-space (count-chars-no-space test-doc))
         (chinese (count-chinese-and-words test-doc))
         (line-count (count-lines test-doc))
         (char-count (count-characters test-doc))
         ;; 测试 get-statistics-data-from-doc 数据组装
         (data (get-statistics-data-from-doc test-doc))
         (data-labels (map car data))
         (data-values (map cadr data))
        ) ;

    ;; --- 计数函数 ---
    ;; 测试字数 - 预期: 16 (10个中文字符 + 6个非中文单词)
    (check (+ (car chinese) non-chinese) => 16)

    ;; 测试字符数(计空格) - 预期: 28
    (check char-count => 28)

    ;; 测试字符数(不计空格) - 预期: 23
    (check chars-no-space => 23)

    ;; 测试段落数 - 预期: 3
    (check line-count => 3)

    ;; 测试非中文单词数 - 预期: 6
    (check non-chinese => 6)

    ;; 测试中文字符数 - 预期: 10
    (check (car chinese) => 10)

    ;; --- get-statistics-data-from-doc 数据格式 ---
    ;; 验证标签顺序与内容（需与 get-statistics-data-from-doc 使用相同的 translate）
    (check data-labels
      =>
      (list (translate "Page count")
        (translate "Word count")
        (translate "Character count (with spaces)")
        (translate "Character count (without spaces)")
        (translate "Paragraph count")
        (translate "Non-Chinese word")
        (translate "Chinese character")
      ) ;list
    ) ;check

    ;; 验证 7 个统计项
    (check (length data) => 7)

    ;; 验证各项值
    (check data-values => '("0" "16" "28" "23" "3" "6" "10"))
  ) ;let*
) ;tm-define
