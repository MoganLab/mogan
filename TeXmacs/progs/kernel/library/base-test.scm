;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : base-test.scm
;; DESCRIPTION : 纯逻辑单元测试：kernel 字符串工具函数
;;               （string-join、string-starts?、string-ends?）。
;;               无 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r base-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/kernel/library/base.scm")

;; string-join：缺省分隔符是 " "（kernel 契约，区别于 g_string-join 的 ""）。

(define (test-string-join-default-delimiter)
  (check (string-join '("a" "b" "c")) => "a b c")
  (check (string-join '("solo")) => "solo")
  (check (string-join '()) => "")
) ;define

(define (test-string-join-explicit-delimiter)
  (check (string-join '("a" "b" "c") ":") => "a:b:c")
  (check (string-join '("a") ":") => "a")
  (check (string-join '() ":") => "")
  (check (string-join '("a" "b") "") => "ab")
  (check (string-join '("" "") "-") => "-")
  (check (string-join '("a" "b") "--") => "a--b")
) ;define

;; 分隔符只插入元素之间（infix），首尾不添加。

(define (test-string-join-infix)
  (check (string-join '("a" "b" "c") "x") => "axbxc")
) ;define

;; 错误契约：元素非 string、分隔符非 string、首参非 proper list 均抛 type-error。

(define (test-string-join-errors)
  (check-catch 'type-error (string-join '("a" 1) ":"))
  (check-catch 'type-error (string-join '("a" "b") 1))
  (check-catch 'type-error (string-join '("a" . "b") ":"))
) ;define

;; string-starts?/string-ends?：转调 g_string-starts?/g_string-ends? 的 C 实现。

(define (test-string-starts-basic)
  (check (string-starts? "hello" "he") => #t)
  (check (string-starts? "hello" "hello") => #t)
  (check (string-starts? "hello" "") => #t)
  (check (string-starts? "hello" "lo") => #f)
  (check (string-starts? "he" "hello") => #f)
) ;define

(define (test-string-ends-basic)
  (check (string-ends? "hello" "lo") => #t)
  (check (string-ends? "hello" "hello") => #t)
  (check (string-ends? "hello" "") => #t)
  (check (string-ends? "hello" "he") => #f)
  (check (string-ends? "lo" "hello") => #f)
) ;define

(tm-define (regtest-base)
  (test-string-join-default-delimiter)
  (test-string-join-explicit-delimiter)
  (test-string-join-infix)
  (test-string-join-errors)
  (test-string-starts-basic)
  (test-string-ends-basic)
  (check-report)
) ;tm-define
