
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tmuniversaltounicode-test.scm
;; DESCRIPTION : TeXmacs universal 符号 <-> Unicode 双向转换测试
;;               （数据表：TeXmacs/langs/encoding/tmuniversaltounicode.scm）。
;;               锁定 bowtie 一族的命名契约：
;;               <bowtie> = U+22C8（对应 LaTeX \bowtie，小蝴蝶结）
;;               <Join>   = U+2A1D（对应 LaTeX \Join，大蝴蝶结）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory of <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;; USAGE
;;   xmake b stem && xmake r tmuniversaltounicode-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;; 符号 -> Unicode

(define (test-symbol-to-unicode)
  (check (string-convert "<bowtie>" "Cork" "UTF-8") => "⋈")
  (check (string-convert "<Join>" "Cork" "UTF-8") => "⨝")
) ;define

;; Unicode -> 符号（反向映射用于 LaTeX/UTF-8 导入时还原符号名）

(define (test-unicode-to-symbol)
  (check (string-convert "⋈" "UTF-8" "Cork") => "<bowtie>")
  (check (string-convert "⨝" "UTF-8" "Cork") => "<Join>")
) ;define

;; 旧（错误）命名不再占用这两个码位（未注册符号原样透传）

(define (test-old-names-removed)
  (check (string-convert "<join>" "Cork" "UTF-8") => "<join>")
  (check (string-convert "<Bowtie>" "Cork" "UTF-8") => "<Bowtie>")
) ;define

(tm-define (regtest-tmuniversaltounicode)
  (test-symbol-to-unicode)
  (test-unicode-to-symbol)
  (test-old-names-removed)
  (check-report)
) ;tm-define
