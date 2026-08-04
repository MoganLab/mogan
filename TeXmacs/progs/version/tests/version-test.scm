
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : version-test.scm
;; DESCRIPTION : Tests for version-compare (denormalize-string, compare-versions)
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(check-set-mode! 'report-failed)
(load "./TeXmacs/progs/version/version-compare.scm")

;; denormalize-string
;; 把字符串切成 diff 对齐用的 token 列表
;;
;; 切词规则
;; ----
;; - 空格：独立分隔 token
;; - ASCII 连续段：一个词 token
;; - herk 转义 <#XXXX>（中文等非 ASCII 字符的编码形态）：每个转义独立成 token
;;
;; 说明
;; ----
;; 单元测试本身就是最佳示例

(define (test-denormalize-words)
  ;; 空串与空格边界（原有行为）
  (check (denormalize-string "") => '())
  (check (denormalize-string " ") => '(" "))
  (check (denormalize-string " a  b ") => '(" " "a" " " " " "b" " "))
  ;; 纯英文按空格切词（原有行为）
  (check (denormalize-string "hello brave world")
    =>
    '("hello" " " "brave" " " "world")
  ) ;check
  ;; 单个词不拆分
  (check (denormalize-string "abcd") => '("abcd"))
) ;define

(define (test-denormalize-herk)
  ;; 单个 herk 转义
  (check (denormalize-string "<#4F60>") => '("<#4F60>"))
  ;; 连续 herk 转义（中文句子），每字一个 token
  (check (denormalize-string "<#4F60><#597D><#554A><#FF0C>")
    =>
    '("<#4F60>" "<#597D>" "<#554A>" "<#FF0C>")
  ) ;check
  ;; herk 与英文混排
  (check (denormalize-string "a<#4F60>b") => '("a" "<#4F60>" "b"))
  (check (denormalize-string "<#4F60> <#597D>") => '("<#4F60>" " " "<#597D>"))
  ;; 中文标点同样是独立 token
  (check (denormalize-string "<#FF0C><#FF1F>") => '("<#FF0C>" "<#FF1F>"))
) ;define

(define (test-denormalize-herk-edge)
  ;; 无闭合的 <# 按普通文本处理
  (check (denormalize-string "a<#4F6") => '("a<#4F6"))
  ;; < 后非 # 按普通字符处理
  (check (denormalize-string "a<b") => '("a<b"))
  ;; 结尾裸 <
  (check (denormalize-string "a<") => '("a<"))
) ;define

;; compare-versions（CJK 场景）
;; herk 编码的中文对做 diff，应对齐到单个转义字符
;;
;; 说明
;; ----
;; version-get d 0 可取回旧版本、version-get d 1 取回新版本，
;; diff 结果中 version-both 的数量即变更块数

(define (count-tag t tag)
  (if (pair? t)
    (+ (if (eq? (car t) tag) 1 0)
      (apply + (map (lambda (x) (count-tag x tag)) (cdr t)))
    ) ;+
    0
  ) ;if
) ;define

(define cjk-old
  "<#4F60><#597D><#554A><#FF0C><#8BF7><#95EE><#4F60><#5728><#54EA><#91CC><#5BC6><#8DEF><#7684><#FF1F>"
) ;define

(define cjk-new
  "<#4F60><#597D><#554A><#FF0C><#8BF7><#95EE><#4F60><#5728><#54EA><#91CC><#8FF7><#8DEF><#7684><#FF1F>"
) ;define

(define (test-compare-cjk)
  ;; 只改一个字符（密->谜）：恰好一个变更块，且新旧版本可完整取回
  (let ((d (compare-versions cjk-old cjk-new)))
    (check (count-tag d 'version-both) => 1)
    (check (version-get d 0) => cjk-old)
    (check (version-get d 1) => cjk-new)
  ) ;let
  ;; 完全相同：无变更
  (check (compare-versions cjk-old cjk-old) => cjk-old)
) ;define

(define (test-compare-english)
  ;; 英文插入一个单词：恰好一个变更块，新旧版本可完整取回
  (let ((d (compare-versions "hello world" "hello brave world")))
    (check (count-tag d 'version-both) => 1)
    (check (version-get d 0) => "hello world")
    (check (version-get d 1) => "hello brave world")
  ) ;let
) ;define

(tm-define (regtest-version)
  (test-denormalize-words)
  (test-denormalize-herk)
  (test-denormalize-herk-edge)
  (test-compare-cjk)
  (test-compare-english)
  (check-report)
) ;tm-define
