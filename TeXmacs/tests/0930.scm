;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0930.scm
;; DESCRIPTION : Tests for magic paste of ChatGPT's new-frontend HTML
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;; ChatGPT 新版前端（2026-08 起，KaTeX 客户端布局）的公式标记：
;; 不再输出 katex-mathml / MathML annotation，LaTeX 源码保存在
;; role="math" 节点的 data-math-source 属性（aria-label 兜底），
;; 行间公式带 style="display: block;" 且首个子节点为 katex-display。

(define gpt-inline
  "<p>inline <span data-start=\"119\" data-end=\"128\" role=\"math\" aria-label=\"f'(x)\" data-math-source=\"f'(x)\" data-client-katex-layout=\"\"><span class=\"katex\"><span class=\"katex-html\" aria-hidden=\"true\"><span class=\"base\">f'(x)</span></span></span></span> end</p>"
) ;define

(define gpt-display
  "<p>before</p><span data-start=\"479\" data-end=\"507\" role=\"math\" aria-label=\"\\int_0^1 x^2\\,dx\" data-math-source=\"\\int_0^1 x^2\\,dx\" data-client-katex-layout=\"\" style=\"display: block;\"><span class=\"katex-display\"><span class=\"katex\"><span class=\"katex-html\" aria-hidden=\"true\"><span class=\"base\">int01x2dx</span></span></span></span></span><p>after</p>"
) ;define

(define gpt-display-multiline
  "<span role=\"math\" data-math-source=\"a=b \\\\ c=d\" data-client-katex-layout=\"\" style=\"display: block;\"><span class=\"katex-display\"><span class=\"katex\"></span></span></span>"
) ;define

(define gpt-aria-only
  "<span role=\"math\" aria-label=\"y=x\"><span class=\"katex\"></span></span>"
) ;define

(define gpt-legacy-katex
  "<span class=\"katex\"><span class=\"katex-mathml\"><math><semantics><mrow><mi>x</mi></mrow><annotation encoding=\"application/x-tex\">x</annotation></semantics></math></span><span class=\"katex-html\" aria-hidden=\"true\"></span></span>"
) ;define

(define gpt-no-source-mathml
  "<span role=\"math\"><span class=\"katex\"><span class=\"katex-mathml\"><math><mi>z</mi></math></span></span></span>"
) ;define

(define (test-chatgpt-inline-math)
  (check (convert gpt-inline "html-snippet" "texmacs-stree")
    => '(concat "inline "
          (math (concat "f" (rprime "'") (around "(" "x" ")")))
          " end"
        ) ;concat
  ) ;check
) ;define

(define (test-chatgpt-display-math)
  (check (convert gpt-display "html-snippet" "texmacs-stree")
    => '(document "before"
          (equation*
           (concat (big "int") (rsub "0") (rsup "1") "x" (rsup "2") "*"
             (space "0.17em") "d*x"
           ) ;concat
          ) ;equation*
          "after"
        ) ;document
  ) ;check
) ;define

(define (test-chatgpt-display-multiline)
  (check (convert gpt-display-multiline "html-snippet" "texmacs-stree")
    => '(equation* (concat "a=b" (next-line) "c=d"))
  ) ;check
) ;define

(define (test-chatgpt-aria-label-fallback)
  ;; 缺 data-math-source 时用 aria-label 兜底
  (check (convert gpt-aria-only "html-snippet" "texmacs-stree")
    => '(math "y=x")
  ) ;check
) ;define

(define (test-legacy-katex-unchanged)
  ;; 旧版 katex-mathml（MathML annotation）路径不受影响
  (check (convert gpt-legacy-katex "html-snippet" "texmacs-stree") => '(math "x"))
) ;define

(define (test-role-math-without-source-unchanged)
  ;; role="math" 但无源码属性时仍走通用 MathML 路径
  (check (convert gpt-no-source-mathml "html-snippet" "texmacs-stree") => '(math "z"))
) ;define

(tm-define (test_0930)
  (test-chatgpt-inline-math)
  (test-chatgpt-display-math)
  (test-chatgpt-display-multiline)
  (test-chatgpt-aria-label-fallback)
  (test-legacy-katex-unchanged)
  (test-role-math-without-source-unchanged)
  (check-report)
) ;tm-define
