;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1157.scm
;; DESCRIPTION : 纯逻辑回归测试：spanish kbd-map 迁移到 lang 插件后，相关
;;               cork 编码契约不变。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1157] 把 text-kbd.scm 中 in-spanish? 的 kbd-map 迁移到
;;   lang/spanish-kbd.scm。迁移前该 kbd-map 的 LHS/RHS 用 cork 编码字节写
;;   死在 text-kbd.scm（cork 文件，不可 gf fmt）；迁移到 UTF-8 新文件后，
;;   必须用 <#XXXX> 转义重建完全等价的字节序列。本测试钉死迁移前后必须
;;   成立的 cork↔utf8 转换契约，任一字节映射回退都会红。
;;
;;   覆盖 spanish kbd-map 的 8 条映射所涉及的全部 cork 字节：
;;     * cork 0xBD = U+00A1 ¡ (exclamdown, 倒感叹号)
;;     * cork 0xBE = U+00BF ¿ (questiondown, 倒问号)
;;     * 顺带验证 cork 0xFF = U+00DF ß、cork 0xA1 = U+0105 ą（同为 text-kbd.scm
;;       里出现的字节，确保我们没把 LHS 0xA1 错当 ¡）。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 1157
;;
;;   纯逻辑（headless 即跑断言），无需 MOGAN_TEST_GUI。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;; 直接键入 spanish 倒感叹号 / 倒问号时，Qt 层把 UTF-8 交上来，handle_keypress
;; 用 utf8->cork 转成内部 cork 字节再去匹配 kbd-map。两条直接按键映射
;; （"¡"→"¡"、"¿"→"¿"）的 LHS 由此路径产生。
(define (test-spanish-direct-keys-utf8-to-cork)
  ;; "¡" U+00A1 → cork 0xBD（单字节）。
  (check (string-length (utf8->cork "¡")) => 1)
  (check (char->integer (string-ref (utf8->cork "¡") 0)) => #xBD)
  ;; "¿" U+00BF → cork 0xBE（单字节）。
  (check (string-length (utf8->cork "¿")) => 1)
  (check (char->integer (string-ref (utf8->cork "¿") 0)) => #xBE)
) ;define

;; kbd-map 的 RHS（插入串）在 .scm 里直接写 cork 字节，最终走 cork->utf8
;; 渲染。倒感叹号 / 倒问号的插入字符必须分别解出 U+00A1 / U+00BF。
(define (test-spanish-insertion-cork-to-utf8)
  (check (cork->utf8 "\xBD") => "¡")
  (check (cork->utf8 "\xBE") => "¿")
) ;define

;; 双向往返自反：spanish kbd-map 涉及的两个字符 utf8->cork->utf8 不丢信息。
(define (test-spanish-roundtrip)
  (check (cork->utf8 (utf8->cork "¡")) => "¡")
  (check (cork->utf8 (utf8->cork "¿")) => "¿")
) ;define

;; text-kbd.scm 里其它 cork 字节也用同样的字节直写——这里顺带钉死几个
;; 高频点，确保我们的编码模型一致（不把 LHS 0xA1 误读成 ¡）。
(define (test-other-cork-bytes-in-text-kbd)
  ;; cork 0xFF = ß（germandbls，"sz" 与 "text:symbol s" 的 RHS）。
  (check (cork->utf8 "\xFF") => "ß")
  ;; cork 0xA1 = ą（a with ogonek，U+0105），不是 ¡。
  (check (cork->utf8 "\xA1") => "ą")
) ;define

(tm-define (test_1157)
  (test-spanish-direct-keys-utf8-to-cork)
  (test-spanish-insertion-cork-to-utf8)
  (test-spanish-roundtrip)
  (test-other-cork-bytes-in-text-kbd)
  (check-report)
) ;tm-define
