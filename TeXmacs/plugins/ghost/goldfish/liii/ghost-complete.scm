;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ghost-complete.scm
;; DESCRIPTION : Ghost 补全后处理（候选调试打印 + 按置信度截断）
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-library (liii ghost-complete)
  (export ghost-build-completion)
  (import (scheme base)
          (scheme write)
          (liii string))
  (begin
    ;; token logprob 低于此阈值则截断（概率 < exp(-1.5) ≈ 0.22 视为不确定）
    (define ghost-logprob-threshold -1.5)

    ;; 打印所有候选 token 及其 logprob（调试用）
    (define (dump-candidates toks lps)
      (display "ghost candidates:")
      (let loop ((ts toks) (ls lps))
        (when (and (pair? ts) (pair? ls))
          (display " [") (display (car ts)) (display ":") (display (car ls)) (display "]")
          (loop (cdr ts) (cdr ls))))
      (newline))
     ;define

    ;; 按 logprob 截断 tokens 拼接成补全文本。
    ;; 先打印候选，再按阈值截断，返回截断后的纯文本（最高置信前缀）或 #f。
    (define (ghost-build-completion toks lps)
      (dump-candidates toks lps)
      (let loop ((ts toks) (ls lps) (acc ""))
        (cond
          ((or (null? ts) (null? ls)) acc)
          ((and (number? (car ls)) (< (car ls) ghost-logprob-threshold))
           (if (string=? acc "") #f acc))
          (else (loop (cdr ts) (cdr ls) (string-append acc (car ts)))))))
     ;define
  ) ;begin
) ;define-library
