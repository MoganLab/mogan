;; 单测：ghost-api 的 ghost-deepseek-complete（DeepSeek FIM，返回 tokens/logprobs）
;; 跑法：cd ghost/goldfish/liii && gf ghost-api-test.scm

(set! *load-path* (cons ".." *load-path*))

(import (liii check))
(import (liii string))
(import (liii ghost-api))

(check-set-mode! 'report)

;; token 是否在向量中
(define (token-contains? toks tok)
  (let loop ((i 0))
    (cond ((>= i (vector-length toks)) #f)
          ((string-contains? (vector-ref toks i) tok) #t)
          (else (loop (+ i 1))))))

;; 主测：返回 (tokens . logprobs)，tokens 非空向量
(let ((res (ghost-deepseek-complete "三鲤网络是一家专注" "的科技公司")))
  (display "tokens=") (display (car res)) (newline)
  (display "logprobs=") (display (cdr res)) (newline)
  (check (pair? res) => #t)
  (check (vector? (car res)) => #t)
  (check (vector? (cdr res)) => #t)
  (check (> (vector-length (car res)) 0) => #t)
  (check (= (vector-length (car res)) (vector-length (cdr res))) => #t))

;; 南方科技 → 补 "大学"（纯续写，suffix 空）
(let ((res (ghost-deepseek-complete "南方科技" "")))
  (display "南方科技+空 => tokens=") (display (car res)) (newline)
  (check (token-contains? (car res) "大学") => #t))

;; 南方 + 大学 → 中间填充（模型预测有不确定性，只验证返回有效结构）
(let ((res (ghost-deepseek-complete "南方" "大学")))
  (display "南方+大学 => tokens=") (display (car res)) (newline)
  (check (vector? (car res)) => #t)
  (check (> (vector-length (car res)) 0) => #t))

(check-report)
