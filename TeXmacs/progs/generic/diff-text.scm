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
  (:use (kernel texmacs tm-define) (utils library cursor) (version version-compare))
) ;texmacs-module

;; =============================================================================
;; Helper functions
;; =============================================================================

(define (string-reverse s)
  (list->string (reverse (string->list s)))
) ;define

(define (diff-check-popup)
  (if (tree-innermost 'version-both)
      (begin
        (set! diff-active? #t)
        (show-diff-popup)
      ) ;begin
      (begin
        (set! diff-active? #f)
        (hide-diff-popup)
      ) ;begin
  ) ;if
) ;define

(define (diff-scan-next)
  (if (not (tree-innermost 'version-both))
      (if (tree-is? (cursor-tree) 'version-both)
          (tree-go-to (tree-ref (cursor-tree) 0) :start) ; 单差异特判：直接送入第一个子段落内部
          (go-to-next-tag 'version-both) ; 多差异常规流程：向后搜寻
      ) ;if
  ) ;if
  (delayed (:idle 1) (diff-check-popup))
) ;define

;; =============================================================================
;; State variables for Diff Text
;; =============================================================================

(define diff-active? #f)

(tm-define (is-diff-active?)
  diff-active?
) ;tm-define

(tm-define (diff-enable?)
  (not (community-stem?))
) ;tm-define

;; =============================================================================
;; Model evaluation & Feedback functions
;; =============================================================================

(tm-define (diff-feedback action)
  (noop)
) ;tm-define

;; =============================================================================
;; Diff Text core control flow
;; =============================================================================

(tm-define (trigger-diff-text)
  (let* ((sel (selection-tree))
         (origin_stree (tree->stree sel))
         (origin_str (tm->string sel))
         (suggested_str (string-reverse origin_str))
         ;; 精度选项："detailed" (字符级)、"block" (块级)、"rough" (粗粒度)
         (diff-stree (compare-versions origin_stree suggested_str))
         (diff-tree (stree->tree diff-stree))
         (p (cursor-path)))
    (clipboard-cut "primary")
    (insert diff-tree)
    (insert-return)
    (go-to p)
    (diff-scan-next)
  ) ;let*
) ;tm-define

(tm-define (accept-diff)
  (let ((t (tree-innermost 'version-both)))
    (when t
      (let* ((new-val (tree-ref t 1))
             (p (tree-up t))
             (i (tree-index t)))
        (tree-remove! p i 1)
        (insert new-val)
      ) ;let*
    ) ;when
  ) ;let
  (diff-feedback 'accept)
  (diff-scan-next)
) ;tm-define

(tm-define (reject-diff)
  (let ((t (tree-innermost 'version-both)))
    (when t
      (let* ((old-val (tree-ref t 0))
             (p (tree-up t))
             (i (tree-index t)))
        (tree-remove! p i 1)
        (insert old-val)
      ) ;let*
    ) ;when
  ) ;let
  (diff-feedback 'reject)
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
  (delayed (:idle 1) (diff-check-popup))
) ;tm-define

(tm-define (mouse-event key x y mods time data)
  (:require (diff-enable?))
  (former key x y mods time data)
  (delayed (:idle 1) (diff-check-popup))
) ;tm-define
