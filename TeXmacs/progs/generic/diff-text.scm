;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : diff-text.scm
;; DESCRIPTION : Diff Text feature for MoganSTEM
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic diff-text)
  (:use (kernel texmacs tm-define) (utils library cursor))
) ;texmacs-module

;; =============================================================================
;; Helper functions
;; =============================================================================

(define (diff-check-popup)
  ;; 先隐藏弹窗，避免位置错乱
  (set! diff-active? #f)
  (hide-diff-popup)
  (when (tree-innermost 'version-both)
    (set! diff-active? #t)
    (show-diff-popup)
  ) ;when
) ;define

(define (diff-scan-next)
  (if (not (tree-innermost 'version-both))
    (if (tree-is? (cursor-tree) 'version-both)
      ;; 单差异特判：直接送入第一个子段落内部
      (tree-go-to (tree-ref (cursor-tree) 0) :start)
      ;; 多差异常规流程：向后搜寻
      (go-to-next-tag 'version-both)
    ) ;if
  ) ;if
  (delayed (:idle 0) (diff-check-popup))
) ;define

;; =============================================================================
;; State variables for Diff Text
;; =============================================================================

(define diff-active? #f)

(tm-define (is-diff-active?) diff-active?)

(tm-define (diff-enable?) (defined? 'diff-cloud-polish))

;; =============================================================================
;; Diff Text core control flow
;; =============================================================================

(tm-define (trigger-diff-text)
  (when (diff-enable?)
    (let* ((sel (selection-tree))
           (origin-stree (tree->stree sel))
           (pre-cur (cursor-path))
           (stree-str (object->string origin-stree))
          ) ;
      (debug-message "debug-io"
        (string-append "diff-request: stree=["
          (substring stree-str 0 (min 500 (string-length stree-str)))
          "]\n"
        ) ;string-append
      ) ;debug-message
      (diff-cloud-polish stree-str
        (lambda (suggested-str)
          (diff-apply-suggestion origin-stree suggested-str pre-cur)
        ) ;lambda
      ) ;diff-cloud-polish
    ) ;let*
  ) ;when
) ;tm-define

(define (diff-apply-suggestion origin-stree suggested-str pre-cur)
  (when (and (string? suggested-str)
          (not (string=? suggested-str ""))
          (selection-active?)
        ) ;and
    (debug-message "debug-io"
      (string-append "diff-predict: suggested=["
        (substring suggested-str 0 (min 500 (string-length suggested-str)))
        "]\n"
      ) ;string-append
    ) ;debug-message
    (let ((suggested-stree (catch #t (lambda () (string->object suggested-str)) (lambda args #f))
          ) ;suggested-stree
         ) ;
      (when suggested-stree
        (let ((pre-grain (get-preference "versioning grain")))
          ;; 设为字符级精度 "detailed"；其余选项："block" (块级)、"rough" (粗粒度)
          (set-preference "versioning grain" "detailed")
          (let* ((diff-stree (compare-versions origin-stree suggested-stree))
                 (diff-tree (stree->tree diff-stree))
                ) ;
            (clipboard-cut "primary")
            (insert diff-tree)
            (go-to pre-cur)
            (diff-scan-next)
          ) ;let*
          ;; 还原精度
          (set-preference "versioning grain" pre-grain)
        ) ;let
      ) ;when
    ) ;let
  ) ;when
) ;define

(tm-define (accept-diff)
  (let ((t (tree-innermost 'version-both)))
    (when t
      (let ((new-val (tree-ref t 1)))
        ;; 用 tree-set! 原位替换，不走「删除 + 光标处 insert」：
        ;; 父节点是 with 时，光标式 insert 会把文本粘进 with 的参数位
        (if (tree-is? new-val 'version-suppressed)
          (let ((p (tree-up t)) (i (tree-index t)))
            (tree-remove! p i 1)
          ) ;let
          (tree-set! t new-val)
        ) ;if
      ) ;let
    ) ;when
  ) ;let
  (diff-feedback 'accept)
  (refresh-window)
  (diff-scan-next)
) ;tm-define

(tm-define (reject-diff)
  (let ((t (tree-innermost 'version-both)))
    (when t
      (let ((old-val (tree-ref t 0)))
        (if (tree-is? old-val 'version-suppressed)
          (let ((p (tree-up t)) (i (tree-index t)))
            (tree-remove! p i 1)
          ) ;let
          (tree-set! t old-val)
        ) ;if
      ) ;let
    ) ;when
  ) ;let
  (diff-feedback 'reject)
  (refresh-window)
  (diff-scan-next)
) ;tm-define

;; =============================================================================
;; Keyboard and Mouse Hooks
;; =============================================================================

(tm-define (kbd-tab)
  (:require (and (diff-enable?) (selection-active?)))
  (trigger-diff-text)
) ;tm-define

(tm-define (keyboard-press key time)
  (:require (is-diff-active?))
  (cond ((== key "return") (accept-diff))
        ((== key "backspace") (reject-diff))
        (else (former key time))
  ) ;cond
) ;tm-define

(tm-define (keyboard-press key time)
  (:require (diff-enable?))
  (former key time)
  (delayed (:idle 0) (diff-check-popup))
) ;tm-define

(tm-define (mouse-event key x y mods time data)
  (:require (diff-enable?))
  (former key x y mods time data)
  (when (not (== key "move"))
    (delayed (:idle 0) (diff-check-popup))
  ) ;when
) ;tm-define

;; =============================================================================
;; Feedback functions
;; =============================================================================

(tm-define (diff-feedback action)
  ;; Action can be 'accept, 'reject, or 'ignore
  (noop)
) ;tm-define
