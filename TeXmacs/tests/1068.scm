;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1068.scm
;; DESCRIPTION : 集成测试：U+25B0/U+25B1（平行四边形）无物理字体覆盖。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1068] 钉死事实：U+25B0（black parallelogram）、U+25B1（white
;;   parallelogram）目前没有任何物理字体覆盖——仓库自带虚拟字体
;;   （emu-*.vfn）与常见系统字体均无对应字形。
;;
;;   断言走 physical-font-for-string（真实排版路径，移植自 da/1166/font）：
;;   把字符串放进临时文档排版，取叶子盒 smart font 再按字符下钻到实际物理
;;   子字体，返回其 res_name；无覆盖时落到 error 字体，res_name 以
;;   "error-" 开头。
;;
;;   后续为这两个字符补齐渲染方案（如虚拟字体合成）后，本测试应翻红，
;;   提示把期望从 error 字体更新为新方案的实际物理字体。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 1068                     # headless，同步断言全量执行
;;   MOGAN_TEST_GUI=1 xmake r 1068    # 真实 GUI，同样同步执行
;;
;; 注意：本测试全程同步（physical-font-for-string 是同步 bridge），
;; headless 与 GUI 模式都会真正执行断言。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(import (liii unicode))

(check-set-mode! 'report-failed)

;; 码位 -> tm 内部字符串（cork；超出 cork 的由 utf8->cork 转 <#XXXX>）。
(define (codepoint->tmstring n)
  (utf8->cork (utf8-string (integer->char n))))

;; res_name 以 "error-" 开头：无字体覆盖的占位字体。
(define (error-font? f)
  (and (> (string-length f) 6)
       (string=? (substring f 0 6) "error-")))

(tm-define (test_1068)
  ;; 前置：默认环境为文本模式（与 1166 同一排版入口）。
  (check (get-env "mode") => "text")
  (check (get-env "font") => "roman")

  ;; 对照组：ASCII 'A' 有物理字体覆盖（随仓库分发的 Cork 字体）。
  (check (physical-font-for-string "A") => "ec:ecrm10@600")

  ;; 事实：两个平行四边形码位均无物理字体覆盖，落到 error 字体。
  (check-true (error-font? (physical-font-for-string (codepoint->tmstring #x25B0))))
  (check-true (error-font? (physical-font-for-string (codepoint->tmstring #x25B1))))

  (check-report)
  (quit-TeXmacs)
) ;tm-define
