
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : polish-kbd.scm
;; DESCRIPTION : keystrokes for the Polish language
;; COPYRIGHT   : (C) 2026  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (lang polish-kbd) (:use (text text-kbd)))

;; 迁移自 text-kbd.scm 的 in-polish? kbd-map（原 cork 字节 RHS 已用
;; <#XXXX> unicode 转义重建，cork↔utf8 字节契约由 tests/1159.scm 钉死）。
;; text:symbol 前缀对应 Shift+F5 符号输入模式。
(kbd-map (:mode in-polish?)
 ("text:symbol a" "<#105>")
 ("text:symbol A" "<#104>")
 ("text:symbol c" "<#107>")
 ("text:symbol C" "<#106>")
 ("text:symbol e" "<#119>")
 ("text:symbol E" "<#118>")
 ("text:symbol l" "<#142>")
 ("text:symbol L" "<#141>")
 ("text:symbol n" "<#144>")
 ("text:symbol N" "<#143>")
 ("text:symbol o" "<#F3>")
 ("text:symbol O" "<#D3>")
 ("text:symbol s" "<#15B>")
 ("text:symbol S" "<#15A>")
 ("text:symbol x" "<#17A>")
 ("text:symbol X" "<#179>")
 ("text:symbol z" "<#17C>")
 ("text:symbol Z" "<#17B>")
 ("text:symbol a var" "<#E6>")
 ("text:symbol A var" "<#C6>")
 ("text:symbol o var" "<#F8>")
 ("text:symbol O var" "<#D8>")
 ("text:symbol s var" "<#DF>")
 ("text:symbol S var" "<#1E9E>")
 ("text:symbol z var" "<#17A>")
 ("text:symbol Z var" "<#179>")
) ;kbd-map
