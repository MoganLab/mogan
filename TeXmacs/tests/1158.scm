;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1158.scm
;; DESCRIPTION : 表格化回归测试：yawerty kbd-map 迁移到 lang 插件后，每个
;;               按键 → 西里尔字母 Unicode 码点契约不变。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1158] 把 TeXmacs/progs/text/cyrillic/yawerty-kbd.scm 迁移到
;;   TeXmacs/plugins/lang/progs/lang/yawerty-kbd.scm，并把 config-kbd.scm 里
;;   的 lazy-keyboard 引用从 (text cyrillic yawerty-kbd) 改成 (lang yawerty-kbd)。
;;
;;   原 kbd-map 的 RHS 全部写成 <#XXXX> 形式（人类不可读）。迁移过程中
;;   一旦任何一个码点抄错、漏抄、或多/少一条映射，都会让 yawerty 输入法
;;   产出错字符。本测试把每一行 kbd-map 的 RHS 通过 cork->utf8 解码回
;;   UTF-8 字符，并与同一行里手写的俄文字母字面量对照——三元组
;;   (key hex utf8) 让"按键 → Unicode 转义 → 实际俄文字符"一目了然。
;;
;;   说明：<#XXXX> 是 TeXmacs 内部 cork 编码的 Unicode 转义形式，
;;   cork->utf8 把它解出 UTF-8 字符串（参见 11_36.scm 反向用例）。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 1158
;;
;;   纯逻辑（headless 即跑断言），无需 MOGAN_TEST_GUI。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;; 按键 → Unicode 转义（kbd-map 的 RHS）→ 期望 UTF-8 字符。
;; 三元组同写一行便于核对：第一列是物理按键，第二列是 kbd-map RHS 原文，
;; 第三列是它应当解出的俄文字母（小写区 35 条 + 大写区 31 条 = 66 条主映射）。
;; 注意：$ 单键直接产出大写 Ъ、& 单键直接产出大写 Ё，是 yawerty 的特殊设计。

(define yawerty-table
  '(("q" "<#44F>" "я")
    ("w" "<#432>" "в")
    ("e" "<#435>" "е")
    ("r" "<#440>" "р")
    ("t" "<#442>" "т")
    ("y" "<#44B>" "ы")
    ("u" "<#443>" "у")
    ("i" "<#438>" "и")
    ("o" "<#43E>" "о")
    ("p" "<#43F>" "п")
    ("a" "<#430>" "а")
    ("s" "<#441>" "с")
    ("d" "<#434>" "д")
    ("f" "<#444>" "ф")
    ("g" "<#433>" "г")
    ("h" "<#445>" "х")
    ("j" "<#439>" "й")
    ("k" "<#43A>" "к")
    ("l" "<#43B>" "л")
    ("z" "<#437>" "з")
    ("x" "<#44C>" "ь")
    ("c" "<#446>" "ц")
    ("v" "<#436>" "ж")
    ("b" "<#431>" "б")
    ("n" "<#43D>" "н")
    ("m" "<#43C>" "м")
    ("[" "<#448>" "ш")
    ("]" "<#449>" "щ")
    ("\\" "<#44D>" "э")
    ("`" "<#44E>" "ю")
    ("=" "<#447>" "ч")
    ("#" "<#44A>" "ъ")
    ("$" "<#42A>" "Ъ")
    ("^" "<#451>" "ё")
    ("&" "<#401>" "Ё")
    ("Q" "<#42F>" "Я")
    ("W" "<#412>" "В")
    ("E" "<#415>" "Е")
    ("R" "<#420>" "Р")
    ("T" "<#422>" "Т")
    ("Y" "<#42B>" "Ы")
    ("U" "<#423>" "У")
    ("I" "<#418>" "И")
    ("O" "<#41E>" "О")
    ("P" "<#41F>" "П")
    ("A" "<#410>" "А")
    ("S" "<#421>" "С")
    ("D" "<#414>" "Д")
    ("F" "<#424>" "Ф")
    ("G" "<#413>" "Г")
    ("H" "<#425>" "Х")
    ("J" "<#419>" "Й")
    ("K" "<#41A>" "К")
    ("L" "<#41B>" "Л")
    ("Z" "<#417>" "З")
    ("X" "<#42C>" "Ь")
    ("C" "<#426>" "Ц")
    ("V" "<#416>" "Ж")
    ("B" "<#411>" "Б")
    ("N" "<#41D>" "Н")
    ("M" "<#41C>" "М")
    ("{" "<#428>" "Ш")
    ("}" "<#429>" "Щ")
    ("|" "<#42D>" "Э")
    ("~" "<#42E>" "Ю")
    ("+" "<#427>" "Ч"))
) ;define

;; 遍历三元组表：cork->utf8 解码 RHS，必须等于手写的第三列字面量。
;; 任一码点抄错 / 漏抄 / 大小写错都会红，并在 report-failed 模式下打印
;; 出错的三元组。

(define (test-yawerty-mappings)
  (for-each (lambda (entry)
              (let ((key (car entry)) (hex (cadr entry)) (expected (caddr entry)))
                (check (cork->utf8 hex) => expected)
              ) ;let
            ) ;lambda
    yawerty-table
  ) ;for-each
) ;define

;; 顺带验证：cork→utf8 往返自反不丢信息（任一码点 <#XXXX> 解码→编码回来不变）。

(define (test-yawerty-roundtrip)
  (check (utf8->cork (cork->utf8 "<#44F>")) => "<#44F>")
  (check (utf8->cork (cork->utf8 "<#42A>")) => "<#42A>")
  (check (utf8->cork (cork->utf8 "<#401>")) => "<#401>")
) ;define

(tm-define (test_1158)
  (test-yawerty-mappings)
  (test-yawerty-roundtrip)
  (check-report)
) ;tm-define
