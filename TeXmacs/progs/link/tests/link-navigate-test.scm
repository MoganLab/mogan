;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : link-navigate-test.scm
;; DESCRIPTION : 回归测试：(link link-navigate) 中被 C++ 直接 call 的符号必须
;;               全部在 init-research.scm 里 lazy-define（#4552）。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   src/Edit/Interface/edit_mouse.cpp 在鼠标移动 / 点击 locus 时直接 call
;;   link-navigate 模块里的若干符号。(link link-navigate) 是懒加载模块：启动后
;;   只有 lazy-define 登记过的符号有桩，其它符号在模块首次被加载前处于未绑定
;;   状态。漏登记的符号一旦先于其它符号被 C++ 调到，就报
;;   ";unbound variable link-mouse-ids"（#4552）。
;;
;;   本测试钉死「C++ 调用点 <-> lazy-define 登记」这份契约：
;;     1. 启动后、模块加载前，每个 C++ 直调符号都已绑定（即有 lazy 桩）；
;;     2. 首次调用能经 lazy 桩加载模块并正确执行；
;;     3. 模块加载后其余符号可正常调用。
;;
;; USAGE
;;   xmake b stem
;;   xmake r link-navigate-test
;;
;; 注意：本测试故意 **不** load / (:use (link link-navigate))。一旦模块被加载，
;; 所有符号都会被定义，(defined? ...) 断言就失去区分度。步骤 1 必须先于任何
;; 真实调用执行。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;; edit_mouse.cpp 直接 call 的 (link link-navigate) 符号。
;; 新增 C++ 调用点时同步维护此表，并在 init-research.scm 的 lazy-define 中登记。

(define cpp-called-link-navigate-symbols
  '(link-active-upwards
    link-active-ids
    link-follow-ids
    link-mouse-ids
    link-contains-inner-link?
    show-hlink-tooltip
  ) ;quote
) ;define

;; 1. 模块加载前每个符号都应已绑定。把符号名带进断言，失败时能直接看出漏了谁。

(define (test-lazy-stubs-defined-before-load)
  (for-each
    (lambda (sym) (check (list sym (defined? sym)) => (list sym #t)))
    cpp-called-link-navigate-symbols
  ) ;for-each
) ;define

;; 2. 首次调用经 lazy 桩加载模块并执行。选 link-contains-inner-link?：
;;    mouse_select 里它在 link-follow-ids 之前被 call，是真实的首个触发点。
;;    空 id 列表没有 url 顶点，按定义返回 #t。

(define (test-first-call-resolves-through-lazy-stub)
  (check (link-contains-inner-link? '()) => #t)
) ;define

;; 3. 模块已加载，其余符号正常可调。show-hlink-tooltip 对空列表应 no-op 不抛错。

(define (test-remaining-symbols-callable)
  (check (link-mouse-ids '()) => '())
  (check (link-active-ids '()) => '())
  (check (begin (show-hlink-tooltip '()) 'ok) => 'ok)
) ;define

(tm-define (regtest-link-navigate)
  (test-lazy-stubs-defined-before-load)
  (test-first-call-resolves-through-lazy-stub)
  (test-remaining-symbols-callable)
  (check-report)
) ;tm-define
