
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-convert-test.scm
;; DESCRIPTION : Test suite for tm-convert
;; COPYRIGHT   : (C) 2026  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel texmacs tm-convert-test)
  (:use (kernel texmacs tm-convert))
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for format-test?
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-format-test-basic)
  ;; 大小写不敏感的 ASCII 关键字匹配
  (check (format-test? "<HTMLxxx" 0 "<html") => #t)
  (check (format-test? "<Htmlxxx" 0 "<html") => #t)
  (check (format-test? "<htmlxxx" 0 "<html") => #t)
  ;; 不匹配
  (check (format-test? "<bodyxxx" 0 "<html") => #f)
  ;; 子串长度不足时不越界（应返回 #f）
  (check (format-test? "<htm" 0 "<html") => #f)
  (check (format-test? "" 0 "<html") => #f)
  ;; pos 越界（输入比 pos 短）
  (check (format-test? "ab" 5 "<html") => #f)
) ;define

(define (test-format-test-pos)
  ;; pos 不为零：从中间位置匹配
  (check (format-test? "xxx<HTML" 3 "<html") => #t)
  (check (format-test? "xxx<htm" 3 "<html") => #f)
  ;; LaTeX 反斜杠关键字
  (check (format-test? "xxx\\DocumentClass" 3 "\\documentclass") => #t)
) ;define

(define (test-format-test-non-ascii)
  ;; 非 ASCII 字符原样保留：子串不等于 ASCII 关键字
  (check (format-test? "<hätml" 0 "<html") => #f)
  ;; 非 ASCII 出现在 what 之前的位置不影响匹配
  (check (format-test? "中文<HTML" 6 "<html") => #t)
  ;; 合法 UTF-8 多字节字符（中文）作为整体不匹配 ASCII 关键字
  (check (format-test? "中文" 0 "<html") => #f)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for safe-ascii-string-downcase
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-safe-ascii-string-downcase)
  ;; 纯 ASCII 大写折叠
  (check (safe-ascii-string-downcase "HELLO") => "hello")
  (check (safe-ascii-string-downcase "Hello World") => "hello world")
  (check (safe-ascii-string-downcase "<HTML>") => "<html>")
  ;; 无可折叠字符时原样返回
  (check (safe-ascii-string-downcase "abc123") => "abc123")
  (check (safe-ascii-string-downcase "") => "")
  ;; 非 ASCII 字符原样保留（中文 UTF-8 字节序列不参与折叠，也不触发崩溃）
  (check (safe-ascii-string-downcase "好A好B") => "好a好b")
  ;; 混合：ASCII 折叠 + 非 ASCII 保留
  (check (safe-ascii-string-downcase "测试<P>") => "测试<p>")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-tm-convert)
  (test-format-test-basic)
  (test-format-test-pos)
  (test-format-test-non-ascii)
  (test-safe-ascii-string-downcase)
  (check-report)
) ;tm-define
