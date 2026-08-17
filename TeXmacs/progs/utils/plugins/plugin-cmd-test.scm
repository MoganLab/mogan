;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : plugin-cmd-test.scm
;; DESCRIPTION : Regression test for plugin-cmd serialization support
;; COPYRIGHT   : (C) 2026 Liii Network
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(use-modules (utils plugins plugin-cmd))
(load "./TeXmacs/plugins/maxima/progs/maxima/maxima-input.scm")

;; test-pre-serialize-passthrough
;; 普通字符串与多段落 document 原样透传

(define (test-pre-serialize-passthrough)
  (check (pre-serialize "python" "1+1") => "1+1")
  (check (pre-serialize "python" "print('中文')") => "print('中文')")
  (check (pre-serialize "python" '(document "a" "b")) => '(document "a" "b"))
) ;define

;; test-pre-serialize-document
;; 单层 document 剥壳，递归到内容本身

(define (test-pre-serialize-document)
  (check (pre-serialize "python" '(document "1+1")) => "1+1")
) ;define

;; test-pre-serialize-math
;; math 输入按 lan 的转换规则渲染成插件语言文本（generic 规则兜底）

(define (test-pre-serialize-math)
  (check (pre-serialize "python" '(math (frac "1" "2"))) => "(1/2)")
  (check (pre-serialize "python" '(math (concat "x" (rsup "2")))) => "x^2")
  (check (pre-serialize "python" '(math "<alpha>")) => "alpha")
  (check (pre-serialize "python" '(math "<mathpi>")) => "(4*atan(1))")
) ;define

;; test-pre-serialize-document-math
;; document 剥壳后递归进入 math 分支

(define (test-pre-serialize-document-math)
  (check (pre-serialize "python" '(document (math (frac "1" "2")))) => "(1/2)")
) ;define

;; test-pre-serialize-math-maxima
;; 插件自定义 handler（maxima-input.scm）的输出同样写入独立 port

(define (test-pre-serialize-math-maxima)
  (check (pre-serialize "maxima" '(math "<mathi>")) => "%i")
  (check (pre-serialize "maxima" '(math (binom "n" "k"))) => "binomial(n,k)")
  (check (pre-serialize "maxima" '(math (sqrt "2"))) => "sqrt(2)")
) ;define

(tm-define (regtest-plugin-cmd)
  (test-pre-serialize-passthrough)
  (test-pre-serialize-document)
  (test-pre-serialize-math)
  (test-pre-serialize-document-math)
  (test-pre-serialize-math-maxima)
  (check-report)
) ;tm-define
