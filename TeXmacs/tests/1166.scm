;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1166.scm
;; DESCRIPTION : 集成测试：文本模式字符的逻辑字体与物理字体解析。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1166] 钉死文本模式下 Unicode 码位 0–255 的字体解析契约（后续逐步扩大
;;   码位范围）：
;;     1. 逻辑字体：当前环境 font/font-family/font-series/font-shape 为
;;        roman rm medium right（默认文档、文本模式）。
;;     2. 物理字体：每个码位经 smart font 按覆盖度解析出的实际叶子字体
;;        （physical-font-for-string，真实排版路径量得）：
;;        - 大多数码位 -> ec:ecrm10@600（随仓库分发的 Cork 字体）
;;        - 128–160 及部分 Latin-1 符号 -> unicode:cmunrm10@600
;;          （TeXmacs/fonts/opentype/cm-unicode 随仓库分发，跨机器稳定）
;;        - 181 (µ) -> gr:grmn1000@600（希腊 tfm）
;;     3. 不可渲染识别：无字体覆盖的码位（如 U+10FFFE）落到 error 字体，
;;        res_name 以 "error-" 开头。
;;
;;   码位区间以 font-test-segments 参数化；扩大范围时追加区间并扩展
;;   expected-physical-font 的例外表即可。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 1166                     # headless，同步断言全量执行
;;   MOGAN_TEST_GUI=1 xmake r 1166    # 真实 GUI，同样同步执行
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

;; 待覆盖的码位区间（闭区间）。后续扩大范围时在此追加。
(define font-test-segments '((0 . 255)))

;; 解析到 unicode:cmunrm10@600 的码位（0–255 内）。
(define cmunrm-codepoints
  '(128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146
    147 148 149 150 151 152 153 154 155 156 157 158 159 160 162 164 165 166 169
    170 172 173 174 176 177 178 179 182 183 185 186 188 189 190 215 247))

;; 0–255 的物理字体期望：默认 Cork 字体，例外见上。
(define (expected-physical-font n)
  (cond ((== n 181) "gr:grmn1000@600")
        ((member n cmunrm-codepoints) "unicode:cmunrm10@600")
        (else "ec:ecrm10@600")))

;; 码位 -> tm 内部字符串（cork；超出 cork 的由 utf8->cork 转 <#XXXX>）。
(define (codepoint->tmstring n)
  (utf8->cork (utf8-string (integer->char n))))

(define (test-logical-font)
  ;; 前置：默认环境为文本模式。
  (check (get-env "mode") => "text")
  (check (get-env "font") => "roman")
  (check (get-env "font-family") => "rm")
  (check (get-env "font-series") => "medium")
  (check (get-env "font-shape") => "right"))

(define (test-physical-fonts lo hi)
  (do ((n lo (+ n 1)))
      ((> n hi))
    (check (physical-font-for-string (codepoint->tmstring n))
      => (expected-physical-font n))))

(define (test-unrenderable-detection)
  ;; U+10FFFE 无字体覆盖，须落到 error 字体（res_name 前缀 error-）。
  (let ((f (physical-font-for-string (codepoint->tmstring #x10FFFE))))
    (check-true (and (> (string-length f) 6)
                     (string=? (substring f 0 6) "error-")))))

(tm-define (test_1166)
  (test-logical-font)
  (for (seg font-test-segments)
    (test-physical-fonts (car seg) (cdr seg)))
  (test-unrenderable-detection)
  (check-report)
  (quit-TeXmacs)
) ;tm-define
