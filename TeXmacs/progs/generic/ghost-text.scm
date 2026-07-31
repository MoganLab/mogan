
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
;; 用户停输入 500ms → 收集上下文 → ghost-cloud-predict 经常驻 goldfish 子进程
;; 异步请求 DeepSeek → 回调做 serial + 光标 + pre-editing 校验后插入 ghost。
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
;; State variables
;; =============================================================================

(define ghost-serial 0)

(define ghost-active? #f)

(define ghost-content "")

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

;; 在 ghost-formula-labels 上递归查找光标所在公式（R7RS base 无 find）

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

(define (ghost-stree->text stree)
  (if (string? stree) stree (object->string stree))
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
            ) ;
        (cons (ghost-stree->text (car pair)) (ghost-stree->text (cdr pair)))
      ) ;let*
    ) ;if
  ) ;let*
) ;define

;; inline 数学：光标在 with "mode" "math" 内但非 displayed 公式环境

(define (ghost-innermost-inline-math)
  (and (in-math?) (not (ghost-find-formula)) (cursor-tree))
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
    ;; 校验通过后才插入 ghost
    (ghost-cloud-predict prefix
      suffix
      (if (in-math?) "math" "text")
      (lambda (res)
        (when (and res
                (== ghost-serial current)
                (not-in-tab-cycling?)
                (not-in-math-subnode?)
                (not (is-pre-editing))
                (equal? (cursor-path) ghost-pending-cursor)
                (not (string=? (car res) ""))
              ) ;and
          (ghost-on-predict (car res))
        ) ;when
      ) ;lambda
    ) ;ghost-cloud-predict
  ) ;let*
) ;tm-define

;; 数学模式：文本转 stree 再转 tree；非法则置空
;; 文本模式：直接返回文本

(define (ghost-predict-content text)
  (if (in-math?)
    (catch #t
      (lambda ()
        (let ((t (tm->tree (string->object text))))
          (if (tm-func? t 'uninit) #f t)
        ) ;let
      ) ;lambda
      (lambda args #f)
    ) ;catch
    text
  ) ;if
) ;define

(define (ghost-on-predict text)
  (let ((content (ghost-predict-content text)))
    (when content
      (debug-message "debug-io"
        (string-append "ghost-predict: text=[" (herk->utf8 text) "]\n")
      ) ;debug-message
      (set! ghost-content content)
      (cursor-after (insert `(ghost ,content)))
      (set! ghost-active? #t)
      (show-ghost-popup)
    ) ;when
  ) ;let
) ;define

;; =============================================================================
;; Ghost node management
;; =============================================================================

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
  (interrupt-shortcut)
  (ghost-feedback 'accept)
  (trigger-ghost-text 0)
) ;tm-define

(tm-define (reject-ghost)
  (set! ghost-active? #f)
  (hide-ghost-popup)
  (ghost-remove-node! #f)
  (interrupt-shortcut)
  (ghost-feedback 'reject)
) ;tm-define

(tm-define (ignore-ghost)
  (set! ghost-active? #f)
  (hide-ghost-popup)
  (ghost-remove-node! #f)
  (interrupt-shortcut)
  (ghost-feedback 'ignore)
) ;tm-define

;; =============================================================================
;; Intercept handlers
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

;; 在 frac/rsub/rsup/rprime 等数学子节点内不触发补全
;; 用 tree-innermost 而非 inside?（后者对这些 label 可能不生效）

(define (not-in-math-subnode?)
  (not (and (in-math?)
         (let loop
           ((labels '(frac rsub rsup rprime lsup lsub lprime)))
           (if (null? labels) #f (or (tree-innermost (car labels)) (loop (cdr labels))))
         ) ;let
       ) ;and
  ) ;not
) ;define

(tm-define (kbd-insert s)
  (former s)
  (when (and (not-in-tab-cycling?) (not-in-math-subnode?))
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
        (else (ignore-ghost) (former key time))
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
;; Feedback
;; =============================================================================

(tm-define (ghost-feedback action) (noop))
