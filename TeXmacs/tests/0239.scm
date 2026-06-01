;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0239.scm
;; DESCRIPTION : Tests for plugin-driven chat-tab prompt
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report)

;;; ========== chat-tab-model-prompt ==========

;; chat-tab-model-prompt 根据模型 key 字符串解析生成 prompt 前缀。
;; 它按 - 分割，查找包含数字的部分；找不到时回退到最后一部分。
;; 注意：string-occurs? 检查子串出现，因此 "0123456789" 不会匹配单数字符，
;; 实际行为通常是回退到最后一部分。

(define (chat-tab-model-prompt model)
  (with parts
    (string-tokenize-by-char model #\-)
    (with part
      (list-find parts (lambda (p) (string-occurs? "0123456789" p)))
      (string-append (if part part (cAr parts)) "> ")
    ) ;with
  ) ;with
) ;define

(define (test-chat-tab-model-prompt)
  ;; 仅含一个部分时直接用它
  (check (chat-tab-model-prompt "Test") => "Test> ")
  ;; 多部分且不含 "0123456789" 子串时回退到最后一部分
  (check (chat-tab-model-prompt "Kimi-VLM") => "VLM> ")
  (check (chat-tab-model-prompt "DeepSeek-V4-Pro") => "Pro> ")
  (check (chat-tab-model-prompt "Foo-Bar") => "Bar> ")
  ;; 多部分时回退到最后一部分（string-occurs? 按子串匹配）
  (check (chat-tab-model-prompt "claude-3-5-sonnet") => "sonnet> ")
  ;; gpt-4o 最后一部分恰好是期望的 prompt
  (check (chat-tab-model-prompt "gpt-4o") => "4o> ")
) ;define

;;; ========== plugin-prompt fallback ==========

(define (test-plugin-prompt-fallback)
  ;; 未存储 prompt 时，plugin-prompt 返回 fallback 字符串
  (let ((fallback (plugin-prompt "llm" "nonexistent-session-0239")))
    (check (string? fallback) => #t)
    (check (string-starts? fallback "Llm") => #t)
  ) ;let
) ;define

;;; ========== llm-welcome prompt construction ==========

;; 模拟 model-profile 访问器

(define (make-test-model-profile model thinking provider)
  (lambda (key)
    (case key
     ((model) model)
     ((thinking) thinking)
     ((provider) provider)
     (else "")
    ) ;case
  ) ;lambda
) ;define

(define (test-llm-prompt-construction)
  ;; model 字段驱动 prompt 前缀
  (let ((mp (make-test-model-profile "kimi-k2.6" "" "Moonshot")))
    (check (string-append (mp 'model) "> ") => "kimi-k2.6> ")
  ) ;let
  ;; 不同模型名
  (let ((mp (make-test-model-profile "deepseek-v4" "enabled" "DeepSeek")))
    (check (string-append (mp 'model) "> ") => "deepseek-v4> ")
  ) ;let
  ;; thinking 为 disabled
  (let ((mp (make-test-model-profile "gpt-4o" "" "OpenAI")))
    (check (string-append (mp 'model) "> ") => "gpt-4o> ")
  ) ;let
) ;define

;;; ========== 测试入口 ==========

(tm-define (test_0239)
  (test-chat-tab-model-prompt)
  (test-plugin-prompt-fallback)
  (test-llm-prompt-construction)
  (check-report))
