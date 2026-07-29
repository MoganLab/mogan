
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ghost-text.scm
;; DESCRIPTION : Ghost Text feature for MoganSTEM
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; 用户停输入 500ms → 收集 FIM 上下文 → ghost-cloud-predict 经常驻 goldfish 子进程
;; 异步请求 DeepSeek FIM → 回调做 serial + 光标校验后插入 ghost。不阻塞 GUI。
;; 文本模式过滤表格/图片/公式为 [TABLE][IMAGE][FORMULA]；数学模式取当前公式转 LaTeX。

(texmacs-module (generic ghost-text)
  (:use (kernel texmacs tm-define)
    (kernel texmacs tm-modes)
    (kernel library content)
    (kernel library tree)
    (utils library cursor)
  ) ;:use
) ;texmacs-module

;; =============================================================================
;; State variables for Ghost Text
;; =============================================================================

(define ghost-serial 0)

(define ghost-active? #f)

(define ghost-content "")

;; 发起请求时的光标位置，回调里校验是否移动过（移动则丢弃，避免插入位置错误）

(define ghost-pending-cursor '())

(define last-key-press "")

(tm-define (is-ghost-active?) ghost-active?)

(tm-define (ghost-enable?) (defined? 'ghost-cloud-predict))

;; =============================================================================
;; Context collection
;; =============================================================================

(define (ghost-body-stree)
  (tm->stree (buffer-get-body (current-buffer)))
) ;define

;; cursor-path 形如 (<buffer-path...> <doc-idx> ...)，去掉 buffer-path 前缀

(define (ghost-relative-path)
  (let* ((cp (cursor-path)) (bp (buffer-path)) (blen (length bp)))
    (if (>= (length cp) blen) (list-tail cp blen) cp)
  ) ;let*
) ;define

(define (ghost-collect-text-context)
  (let* ((body (ghost-body-stree)) (path (ghost-relative-path)))
    (ghost-split-text-context body path)
  ) ;let*
) ;define

;; R7RS base 无 find，用命名 let 在 ghost-formula-labels 上递归

(define (ghost-find-formula)
  (let loop
    ((labels ghost-formula-labels))
    (if (null? labels)
      #f
      (let ((t (tree-innermost (car labels))))
        (or t (loop (cdr labels)))
      ) ;let
    ) ;if
  ) ;let
) ;define

(define (ghost-collect-math-context)
  (let* ((formula-t (or (ghost-find-formula) (ghost-innermost-inline-math)))
         (formula-stree (and formula-t (tm->stree formula-t)))
        ) ;
    (if (not formula-stree)
      (cons "" "")
      (let* ((fp (and formula-t (tree->path formula-t)))
             (cp (cursor-path))
             (inner (and fp (>= (length cp) (length fp)) (list-tail cp (length fp))))
             (pair (ghost-split-math-context formula-stree (or inner '())))
             (before-latex (ghost-stree->latex (car pair)))
             (after-latex (ghost-stree->latex (cdr pair)))
            ) ;
        (cons before-latex after-latex)
      ) ;let*
    ) ;if
  ) ;let*
) ;define

;; inline 数学：光标在 with "mode" "math" 内但非 displayed 公式环境

(define (ghost-innermost-inline-math)
  (and (in-math?) (not (ghost-find-formula)) (cursor-tree))
) ;define

(define (ghost-stree->latex stree)
  (if (string? stree)
    stree
    (let ((s (convert (tm->tree stree) "texmacs-tree" "latex-snippet")))
      (if (string? s) s "")
    ) ;let
  ) ;if
) ;define

(define (ghost-collect-context)
  (if (in-math?) (ghost-collect-math-context) (ghost-collect-text-context))
) ;define

;; =============================================================================
;; Model evaluation
;; =============================================================================

(tm-define (trigger-ghost-text dl-time)
  (when (ghost-enable?)
    (when ghost-active?
      (ignore-ghost)
    ) ;when
    (set! ghost-serial (+ ghost-serial 1))
    (let ((current ghost-serial))
      (delayed (:idle dl-time)
        (when (and (== ghost-serial current)
                (not-in-tab-cycling?)
                (or (in-text?) (in-math?))
                (in-editor-buffer?)
                (not-at-line-start?)
              ) ;and
          (generate-ghost-text current)
        ) ;when
      ) ;delayed
    ) ;let
  ) ;when
) ;tm-define

(tm-define (generate-ghost-text current)
  (let* ((ctx (ghost-collect-context)) (prefix (car ctx)) (suffix (cdr ctx)))
    (set! ghost-pending-cursor (cursor-path))
    (debug-message "debug-io"
      (string-append "ghost-context: mode="
        (if (in-math?) "math" "text")
        " prefix=["
        (herk->utf8 prefix)
        "] suffix=["
        (herk->utf8 suffix)
        "]\n"
      ) ;string-append
    ) ;debug-message
    ;; serial + 光标校验通过后才插入 ghost
    (ghost-cloud-predict prefix
      suffix
      (lambda (res)
        (when (and res
                (== ghost-serial current)
                (equal? (cursor-path) ghost-pending-cursor)
                (not (string=? (car res) ""))
              ) ;and
          (ghost-on-predict (car res))
        ) ;when
      ) ;lambda
    ) ;ghost-cloud-predict
  ) ;let*
) ;tm-define

(define (ghost-on-predict text)
  (debug-message "debug-io"
    (string-append "ghost-predict: text=[" (herk->utf8 text) "]\n")
  ) ;debug-message
  (set! ghost-content text)
  ;; cursor-after 保证光标停在 ghost 之前
  (cursor-after (insert `(ghost ,text)))
  (set! ghost-active? #t)
  (show-ghost-popup)
) ;define

;; ghost body 是 accessible none，光标无法进入，故用 tree-search 定位

(define (ghost-node)
  (let ((found (tree-search (buffer-get-body (current-buffer))
                 (lambda (t) (tree-is? t 'ghost))
               ) ;tree-search
        ) ;found
       ) ;
    (and (pair? found) (car found))
  ) ;let
) ;define

;; 移除 ghost 节点；keep-content? 为真则保留补全文本

(define (ghost-remove-node! keep-content?)
  (let ((t (ghost-node)))
    (when t
      (let ((p (tree-up t)) (i (tree-index t)))
        (tree-remove! p i 1)
        (when keep-content?
          (insert (tree-ref t 0))
        ) ;when
      ) ;let
    ) ;when
  ) ;let
) ;define

(tm-define (accept-ghost)
  (set! ghost-active? #f)
  (hide-ghost-popup)
  (ghost-remove-node! #t)
  (ghost-feedback 'accept)
) ;tm-define

(tm-define (reject-ghost)
  (set! ghost-active? #f)
  (hide-ghost-popup)
  (ghost-remove-node! #f)
  (ghost-feedback 'reject)
) ;tm-define

(tm-define (ignore-ghost)
  (set! ghost-active? #f)
  (hide-ghost-popup)
  (ghost-remove-node! #f)
  (ghost-feedback 'ignore)
) ;tm-define

;; =============================================================================
;; Intercept handlers (keyboard and mouse)
;; =============================================================================

(define (not-in-tab-cycling?)
  (not (== last-key-press "tab"))
) ;define

(define (not-at-line-start?)
  (let* ((ctx (ghost-collect-context)) (prefix (car ctx)))
    (and (not (string=? prefix ""))
      (not (char=? (string-ref prefix (- (string-length prefix) 1)) #\newline))
    ) ;and
  ) ;let*
) ;define

(define (in-editor-buffer?)
  (let* ((u (current-buffer-url)) (s (url->unix u)))
    (or (not (url-rooted-tmfs? u)) (string-starts? s "tmfs://part/"))
  ) ;let*
) ;define

(tm-define (kbd-insert s)
  (former s)
  (when (not-in-tab-cycling?)
    (trigger-ghost-text 500)
  ) ;when
) ;tm-define

(tm-define (keyboard-press key time)
  (set! last-key-press key)
  (when (== key "right")
    (let ((before (cursor-path)))
      (former key time)
      (when (equal? (cursor-path) before)
        (trigger-ghost-text 0)
      ) ;when
    ) ;let
  ) ;when
  (when (!= key "right")
    (former key time)
  ) ;when
) ;tm-define

(tm-define (keyboard-press key time)
  (:require (is-ghost-active?))
  (cond ((== key "right") (accept-ghost))
        ((== key "escape") (reject-ghost))
        (else (ignore-ghost) (delayed (:idle 0) (keyboard-press key time)))
  ) ;cond
) ;tm-define

(tm-define (mouse-event key x y mods time data)
  (:require (is-ghost-active?))
  (former key x y mods time data)
  (when (not (== key "move"))
    (ignore-ghost)
  ) ;when
) ;tm-define

;; =============================================================================
;; Feedback functions
;; =============================================================================

(tm-define (ghost-feedback action) (noop))
