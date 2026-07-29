;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1159.scm
;; DESCRIPTION : 纯逻辑回归测试：polish kbd-map 迁移到 lang 插件后，相关
;;               cork 编码契约不变。也是理解 text-kbd.scm cork 编码的活文档。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1159] 把 text-kbd.scm 中 in-polish? 的 kbd-map（26 条 text:symbol 组合键）
;;   迁移到 lang/polish-kbd.scm。迁移前 RHS 在 cork 编码的 text-kbd.scm 里
;;   以单字节 cork 字面量直写；迁移到 UTF-8 新文件后用 <#XXXX> unicode 转义
;;   重建。本测试钉死迁移前后必须成立的 cork↔utf8 转换契约——任一字节映射
;;   回退或码点抄错都会红。
;;
;;   想理解 cork 编码（为什么 text-kbd.scm 是 cork、<#XXXX> 如何对应单字节、
;;   cork↔utf8 转换怎么工作），直接读本文件 + 跑 xmake r 1159：每个 polish
;;   相关字符的 cork 字节、unicode 码点、UTF-8 字面量三者并排，是编码的
;;   活参考。
;;
;;   覆盖 polish kbd-map 涉及的全部 24 个不重复 cork 字节（小写 9 + 大写 9
;;   + var 组合的 6 个特殊字符 æ/Æ/ø/Ø/ß/ß）。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 1159
;;
;;   纯逻辑（headless 即跑断言），无需 MOGAN_TEST_GUI。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;; cork 字节 -> UTF-8 字符（kbd-map RHS 在文档里最终渲染的字符）。
;; 迁移前的 text-kbd.scm 里这些 RHS 是单字节 cork 字面量；cork->utf8
;; 把它们解成下面的 UTF-8 字符串。迁移后的 polish-kbd.scm 用 <#XXXX>
;; 写 RHS，TeXmacs 同样把它转成这些字符插入，故 cork->utf8 契约不变。
(define (test-polish-cork-to-utf8)
  ;; 基本字母（小写）
  (check (cork->utf8 "\xA1") => "ą")   ; text:symbol a
  (check (cork->utf8 "\xA2") => "ć")   ; text:symbol c
  (check (cork->utf8 "\xA6") => "ę")   ; text:symbol e
  (check (cork->utf8 "\xAA") => "ł")   ; text:symbol l
  (check (cork->utf8 "\xAB") => "ń")   ; text:symbol n
  (check (cork->utf8 "\xF3") => "ó")   ; text:symbol o
  (check (cork->utf8 "\xB1") => "ś")   ; text:symbol s
  (check (cork->utf8 "\xB9") => "ź")   ; text:symbol x / z var
  (check (cork->utf8 "\xBB") => "ż")   ; text:symbol z
  ;; 基本字母（大写）
  (check (cork->utf8 "\x81") => "Ą")   ; text:symbol A
  (check (cork->utf8 "\x82") => "Ć")   ; text:symbol C
  (check (cork->utf8 "\x86") => "Ę")   ; text:symbol E
  (check (cork->utf8 "\x8A") => "Ł")   ; text:symbol L
  (check (cork->utf8 "\x8B") => "Ń")   ; text:symbol N
  (check (cork->utf8 "\xD3") => "Ó")   ; text:symbol O
  (check (cork->utf8 "\x91") => "Ś")   ; text:symbol S
  (check (cork->utf8 "\x99") => "Ź")   ; text:symbol X / Z var
  (check (cork->utf8 "\x9B") => "Ż")   ; text:symbol Z
  ;; var 组合的特殊字符（语义疑为历史遗留，本次只做字节等价迁移）
  (check (cork->utf8 "\xE6") => "æ")   ; text:symbol a var
  (check (cork->utf8 "\xC6") => "Æ")   ; text:symbol A var
  (check (cork->utf8 "\xF8") => "ø")   ; text:symbol o var
  (check (cork->utf8 "\xD8") => "Ø")   ; text:symbol O var
  ;; cork 0xDF 是德语 ß 的大写连字，cork->utf8 解为 "SS"（两字符），
  ;; 无单一 unicode 码点，故 polish-kbd.scm 里 s var / S var 的 RHS
  ;; 直接写 "SS" 字面量，与原 cork 0xDF 显示等价。
  (check (cork->utf8 "\xDF") => "SS")  ; text:symbol s var / S var
  ;; 对照：ß (U+00DF) 在 cork 里是 0xFF，不是 0xDF。
  (check (cork->utf8 "\xFF") => "ß")
) ;define

;; UTF-8 字符 -> cork 字节（反向：直接输入这些字符时 Qt 交 UTF-8 给
;; handle_keypress，utf8->cork 转成内部 cork 字节再匹配 kbd-map LHS）。
(define (test-polish-utf8-to-cork)
  (check (utf8->cork "ą") => "\xA1")
  (check (utf8->cork "ć") => "\xA2")
  (check (utf8->cork "ę") => "\xA6")
  (check (utf8->cork "ł") => "\xAA")
  (check (utf8->cork "ń") => "\xAB")
  (check (utf8->cork "ó") => "\xF3")
  (check (utf8->cork "ś") => "\xB1")
  (check (utf8->cork "ź") => "\xB9")
  (check (utf8->cork "ż") => "\xBB")
  (check (utf8->cork "Ą") => "\x81")
  (check (utf8->cork "Ć") => "\x82")
  (check (utf8->cork "Ę") => "\x86")
  (check (utf8->cork "Ł") => "\x8A")
  (check (utf8->cork "Ń") => "\x8B")
  (check (utf8->cork "Ó") => "\xD3")
  (check (utf8->cork "Ś") => "\x91")
  (check (utf8->cork "Ź") => "\x99")
  (check (utf8->cork "Ż") => "\x9B")
) ;define

;; 双向往返自反：关键字符 utf8->cork->utf8 不丢信息。
(define (test-polish-roundtrip)
  (check (cork->utf8 (utf8->cork "ą")) => "ą")
  (check (cork->utf8 (utf8->cork "ł")) => "ł")
  (check (cork->utf8 (utf8->cork "ó")) => "ó")
  (check (cork->utf8 (utf8->cork "ź")) => "ź")
  (check (cork->utf8 (utf8->cork "ż")) => "ż")
  (check (cork->utf8 (utf8->cork "Ą")) => "Ą")
  (check (cork->utf8 (utf8->cork "Ż")) => "Ż")
) ;define

(tm-define (test_1159)
  (test-polish-cork-to-utf8)
  (test-polish-utf8-to-cork)
  (test-polish-roundtrip)
  (check-report)
) ;tm-define
