;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1243.scm
;; DESCRIPTION : 打开 AI 侧边栏时自动填入文档选区内容
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;; PURPOSE
;;   chat-tab-should-prefill? / chat-tab-prefillable-source? 是纯判定，
;;   headless 下直接断言。
;;   端到端用例以间谍替换写入函数 chat-tab-set-input-body!（其内部
;;   with-buffer + buffer-focus 仅对已有嵌入式视图的生产输入 buffer
;;   有效），验证选区捕获与参数传递；真实写入路径由
;;   chat-tab-clear-input! 的生产调用（每次发送后清空输入区）覆盖。
;;
;; USAGE
;;   xmake b stem && xmake r 1243
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1243) (:use (utils library cursor)))

(import (liii check))

(check-set-mode! 'report)

;;; ---------- 辅助 ----------

(define (in-buf sid)
  (chat-tab-session->input-buffer sid)
) ;define

(define (reset-input! sid)
  (buffer-set-body (in-buf sid) '(document ""))
) ;define

;;; ---------- chat-tab-should-prefill?（纯判定，headless） ----------

;; 输入区为空 + 选区非空 => 预填

(define (test-should-prefill-empty-input)
  (let ((sid "test1243a"))
    (reset-input! sid)
    (check (chat-tab-should-prefill? (in-buf sid) (stree->tree '(document "hello")))
      =>
      #t
    ) ;check
  ) ;let
) ;define

;; 输入区已有内容 => 不覆盖

(define (test-should-prefill-nonempty-input)
  (let ((sid "test1243b"))
    (buffer-set-body (in-buf sid) '(document "typed"))
    (check (chat-tab-should-prefill? (in-buf sid) (stree->tree '(document "hello")))
      =>
      #f
    ) ;check
  ) ;let
) ;define

;; 选区为 #f（无选区）=> 不预填

(define (test-should-prefill-false-selection)
  (let ((sid "test1243c"))
    (reset-input! sid)
    (check (chat-tab-should-prefill? (in-buf sid) #f) => #f)
  ) ;let
) ;define

;; 选区为纯空白 => 不预填

(define (test-should-prefill-blank-selection)
  (let ((sid "test1243d"))
    (reset-input! sid)
    (check (chat-tab-should-prefill? (in-buf sid) (stree->tree '(document "  ")))
      =>
      #f
    ) ;check
  ) ;let
) ;define

;; 选区为原子串（selection-tree 对单行选区的实际返回形态）=> 预填

(define (test-should-prefill-atomic-selection)
  (let ((sid "test1243e"))
    (reset-input! sid)
    (check (chat-tab-should-prefill? (in-buf sid) (string->tree "plain text"))
      =>
      #t
    ) ;check
  ) ;let
) ;define

;;; ---------- chat-tab-prefillable-source?（来源 buffer 守卫） ----------

(define (test-prefillable-source)
  (check (chat-tab-prefillable-source? (string->url "tmfs://chat/abc/input"))
    =>
    #f
  ) ;check
  (check (chat-tab-prefillable-source? (string->url "tmfs://chat/abc/message"))
    =>
    #f
  ) ;check
  (check (chat-tab-prefillable-source? (string->url "tmfs://chat-tab")) => #f)
  (check (chat-tab-prefillable-source? (string->url "tmfs://startup-tab")) => #t)
  (check (chat-tab-prefillable-source? (string->url "/tmp/test1243.tm")) => #t)
) ;define

;;; ---------- 端到端（选区 → 写入调用链） ----------

;; 写入函数 chat-tab-set-input-body! 内部走 with-buffer + buffer-focus，
;; 仅对已有嵌入式视图的生产输入 buffer 有效；测试中以间谍替换，
;; 验证包装函数正确捕获选区并以正确参数触发写入

(define prefill-spy '())

(define (test-prefill-from-selection-e2e)
  (let ((sid "test1243e2e"))
    (reset-input! sid)
    (set! prefill-spy '())
    (tm-define (chat-tab-set-input-body! input-buffer body)
      (set! prefill-spy (list input-buffer body))
    ) ;tm-define
    ;; headless 验证过当前 buffer 上 insert/select-all 可用
    (insert "pick me")
    (select-all)
    (check (selection-active-any?) => #t)
    (chat-tab-prefill-from-selection! sid)
    (check (url->string (car prefill-spy)) => "tmfs://chat/test1243e2e/input")
    ;; selection-tree 对单行选区返回原子串
    (check (tree->stree (cadr prefill-spy)) => "pick me")
  ) ;let
) ;define

;; 无选区时包装函数不触发写入

(define (test-prefill-from-selection-no-selection)
  (let ((sid "test1243e2f"))
    (reset-input! sid)
    (set! prefill-spy '())
    (selection-cancel)
    (check (selection-active-any?) => #f)
    (chat-tab-prefill-from-selection! sid)
    (check prefill-spy => '())
  ) ;let
) ;define

;;; ---------- 入口 ----------

(tm-define (test_1243)
  ;; llm 插件按 idle 延迟初始化，headless 下测试先于插件加载执行
  (use-modules (llm chat-loader))
  (test-should-prefill-empty-input)
  (test-should-prefill-nonempty-input)
  (test-should-prefill-false-selection)
  (test-should-prefill-blank-selection)
  (test-should-prefill-atomic-selection)
  (test-prefillable-source)
  ;; e2e 会修改当前 buffer 内容并重定义写入函数，须最后执行
  (test-prefill-from-selection-e2e)
  (test-prefill-from-selection-no-selection)
  (check-report)
  (when (getenv "MOGAN_TEST_GUI")
    (quit-TeXmacs)
  ) ;when
) ;tm-define
