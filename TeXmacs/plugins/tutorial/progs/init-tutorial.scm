;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-tutorial.scm
;; DESCRIPTION : tutorial plugin entrypoint
;; COPYRIGHT   : (C) 2026
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (tutorial-magic-paste-demo-path)
  (unix->url "$TEXMACS_PATH/plugins/tutorial/data/zhihu-magic-paste-demo.html")
) ;define

(define tutorial-magic-paste-demo-opened? #f)

(define tutorial-ocr-demo-opened? #f)

(define tutorial-ocr-demo-first-result
  "\\[ \\begin{array}{cc}\n     V (\\mu, \\pi) & =\\mathbb{E}a_h \\sim \\mu (s_h, h), b_h \\sim \\pi (s_h, h)\\\\\n     & \\left[ \\sum_{h = 0}^{H - 1} \\gamma^h \\mathbb{E}(r (s_h, a_h, b_h))\n     \\right.\\\\\n     &  \\mid s_0 = s_0, s_{h + 1} \\sim \\mathbb{P}(s_h, a_h, b_h)] .\n   \\end{array} \\]\n\nWe denote the set of potential partner policies as $H^{\\ast}$, which is a\nsubset of $\\Pi$. The AI agent collaborates across K episodes with a fixed yet\nunknown policy $\\pi^{\\ast}$ from $H^{\\ast}$. However, the AI agent does not\nknow $H^{\\ast}$ exactly. Instead, an online learning algorithm that solves the\nAHT problem is provided with a prior hypothesis set H that approximates\n$H^{\\ast}$. The algorithm that takes H K as input is denoted as Alg. The\nalgorithm produces a series of AI agent policies $\\{\\mu^k \\}_{k \\in [K]}$,\ni.e., $\\{\\mu^k \\}_{k \\in [K]} = \\mathbf{Alg} (K, \\mathcal{H})$. We also use\nthe term $V^{\\ast} (\\pi)$ to represent the optimal collaboration reward if the\ntrue policy is $\\pi$, i.e., $V^{\\ast} (\\pi) \\triangleq \\max_{\\mu \\in\n\\mathcal{U}} V (\\mu, \\pi)$. The regret function for an algorithm Alg is\ndefined as"
) ;define

(define tutorial-ocr-demo-second-result
  "\\[ \\mathrm{Reg} \\mathbf{Alg} (K, \\mathcal{H}, \\pi^{}) = \\sum_{k \\in [K]} [V^{}\n   (\\pi^{}) - V (\\mu^k, \\pi^{})] . \\]"
) ;define

(define tutorial-ocr-demo-result-inserted? #f)

(define (tutorial-ocr-demo-document-path)
  (unix->url "$TEXMACS_PATH/plugins/tutorial/data/ocr-demo.tmu")
) ;define

(define (tutorial-ocr-demo-image-path)
  (unix->url "$TEXMACS_PATH/misc/images/tutorial/stem-image.png")
) ;define

(tm-define (tutorial-notify-action action)
  (cpp-set-preference "tutorial:last-action" action)
) ;tm-define

(tm-define (tutorial-prepare-magic-paste-demo)
  (let* ((html-path (tutorial-magic-paste-demo-path))
         (html (string-load html-path))
         (old-export (clipboard-get-export))
        ) ;
    (if (not tutorial-magic-paste-demo-opened?)
      (begin
        (new-document)
        (set! tutorial-magic-paste-demo-opened? #t)
      ) ;begin
    ) ;if
    (if (defined? 'qt-clipboard-set-html)
      (qt-clipboard-set-html html)
      (begin
        (clipboard-set-export "verbatim")
        (clipboard-set "primary" html)
        (clipboard-set-export old-export)
      ) ;begin
    ) ;if
  ) ;let*
) ;tm-define

(tm-define (tutorial-prepare-ocr-demo)
  (if (not tutorial-ocr-demo-opened?)
    (begin
      (load-document (tutorial-ocr-demo-document-path))
      (set! tutorial-ocr-demo-opened? #t)
    ) ;begin
  ) ;if
  (graphics-file-to-clipboard (tutorial-ocr-demo-image-path))
) ;tm-define

(define (tutorial-insert-ocr-demo-result)
  (when (not tutorial-ocr-demo-result-inserted?)
    (insert (latex->texmacs (parse-latex tutorial-ocr-demo-first-result)))
    (kbd-return)
    (insert (latex->texmacs (parse-latex tutorial-ocr-demo-second-result)))
    (set! tutorial-ocr-demo-result-inserted? #t)
  ) ;when
) ;define

;; Why: 直接调 kbd-magic-paste 会触发登录/额度校验弹窗，不适合教程演示；
;;      且其 notify-action 写死为 "ocr-paste"，无法匹配魔法粘贴步骤的 "paste"。
;;      这里直接走核心解析逻辑 smart-format-paste，再显式发 "paste" 完成信号。
(tm-define (tutorial-trigger-magic-paste)
  (smart-format-paste)
  (when (defined? 'tutorial-notify-action)
    (tutorial-notify-action "paste")
  ) ;when
) ;tm-define

(tm-define (tutorial-trigger-ocr)
  (go-end)
  (kbd-return)
  (tutorial-insert-ocr-demo-result)
  (when (defined? 'tutorial-notify-action)
    (tutorial-notify-action "ocr-paste")
  ) ;when
) ;tm-define
