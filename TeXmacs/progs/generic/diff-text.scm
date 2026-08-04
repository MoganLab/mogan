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

(define diff-serial 0)

(define diff-active? #f)

(tm-define (is-diff-active?) diff-active?)

(tm-define (diff-enable?) (defined? 'diff-cloud-polish))

;; 树锚点（tree pin）：触发时把选区两端点所在的树节点钉住，
;; 树引用随编辑自动追踪，选区取消/光标移动后仍能找回原区域

(define (diff-pin->path pin offset)
  ;; 锚点节点存活则重建「当前路径 + 原偏移」，节点被删（区域已移除）返回 #f
  (let ((p (tree->path pin)))
    (if (pair? p) (append p (list offset)) #f)
  ) ;let
) ;define

;; =============================================================================
;; Diff Text core control flow
;; =============================================================================

(tm-define (trigger-diff-text)
  (when (diff-enable?)
    (set! diff-serial (+ diff-serial 1))
    (let* ((current diff-serial)
           (sel (selection-tree))
           (origin-stree (tree->stree sel))
           (p1 (selection-get-start))
           (p2 (selection-get-end))
           (pin1 (path->tree (cDr p1)))
           (pin2 (path->tree (cDr p2)))
           (off1 (cAr p1))
           (off2 (cAr p2))
           (stree-str (object->string origin-stree))
          ) ;
      (debug-message "debug-io"
        (string-append "diff-request: stree=[" (herk->utf8 stree-str) "]\n")
      ) ;debug-message
      (diff-cloud-polish stree-str
        (lambda (suggested-str)
          (diff-apply-suggestion current origin-stree suggested-str pin1 off1 pin2 off2)
        ) ;lambda
      ) ;diff-cloud-polish
    ) ;let*
  ) ;when
) ;tm-define

(define (diff-apply-suggestion current origin-stree suggested-str pin1 off1 pin2 off2)
  (let ((new-p1 (diff-pin->path pin1 off1)) (new-p2 (diff-pin->path pin2 off2)))
    (when (and (== current diff-serial)
            (string? suggested-str)
            (not (string=? suggested-str ""))
          ) ;and
      (if (and new-p1 new-p2)
        (begin
          (debug-message "debug-io"
            (string-append "diff-predict: suggested=[" (herk->utf8 suggested-str) "]\n")
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
                  ;; 经树锚点重建选区，与当前活跃选区无关
                  (selection-set new-p1 new-p2)
                  (clipboard-cut "primary")
                  (insert diff-tree)
                  (go-to new-p1)
                  (diff-scan-next)
                ) ;let*
                ;; 还原精度
                (set-preference "versioning grain" pre-grain)
              ) ;let
            ) ;when
          ) ;let
        ) ;begin
        (debug-message "debug-io" "diff-predict: dropped (region deleted)\n")
      ) ;if
    ) ;when
  ) ;let
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
