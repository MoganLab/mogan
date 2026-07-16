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
  (:use (kernel texmacs tm-define)
    (utils library cursor)
    (version version-compare)
  ) ;:use
) ;texmacs-module

;; =============================================================================
;; Helper functions
;; =============================================================================

(define (remove-random-thes words count)
  (cond ((null? words) '())
        ((<= count 0) words)
        ((== (car words) "the")
         (cond ((<= (random 3) 1) (remove-random-thes (cdr words) (- count 1)))
               (else (cons (car words) (remove-random-thes (cdr words) count)))
         ) ;cond
        ) ;
        (else (cons (car words) (remove-random-thes (cdr words) count)))
  ) ;cond
) ;define

(define (insert-random-as words count)
  (cond ((null? words)
         (if (> count 0) (cons "a" (insert-random-as '() (- count 1))) '())
        ) ;
        ((<= count 0) words)
        (else (if (== (random 5) 0)
                (cons "a" (insert-random-as words (- count 1)))
                (cons (car words) (insert-random-as (cdr words) count))
              ) ;if
        ) ;else
  ) ;cond
) ;define

(define (upcase-random-words words count)
  (cond ((or (null? words) (<= count 0)) words)
        ((> (string-length (car words)) 1)
         (if (== (random 4) 0)
           (cons (upcase-all (car words)) (upcase-random-words (cdr words) (- count 1)))
           (cons (car words) (upcase-random-words (cdr words) count))
         ) ;if
        ) ;
        (else (cons (car words) (upcase-random-words (cdr words) count)))
  ) ;cond
) ;define

(define (demo-suggest t)
  (cond ((not t) #f)
        ((string? t)
         (let* ((words (string-split t #\space))
                (words-no-the (remove-random-thes words 2))
                (words-with-a (insert-random-as words-no-the 2))
                (final-words (upcase-random-words words-with-a 3))
               ) ;
           (string-recompose final-words " ")
         ) ;let*
        ) ;
        ((pair? t) (cons (car t) (map demo-suggest (cdr t))))
        (else t)
  ) ;cond
) ;define

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

(tm-define (diff-enable?) (not (community-stem?)))

;; =============================================================================
;; Model evaluation & Feedback functions
;; =============================================================================

(tm-define (diff-feedback action) (noop))

;; =============================================================================
;; Diff Text core control flow
;; =============================================================================

(tm-define (trigger-diff-text)
  (let* ((sel (selection-tree))
         (origin_stree (tree->stree sel))
         (suggested_stree (demo-suggest origin_stree))
         (pre-cur (cursor-path))
         (pre-grain (get-preference "versioning grain"))
        ) ;
    ;; 设为字符级精度 "detailed"；其余选项："block" (块级)、"rough" (粗粒度)
    (set-preference "versioning grain" "detailed")
    (let* ((diff-stree (compare-versions origin_stree suggested_stree))
           (diff-tree (stree->tree diff-stree))
          ) ;
      (clipboard-cut "primary")
      (insert diff-tree)
      (go-to pre-cur)
      (diff-scan-next)
    ) ;let*
    ;; 还原精度
    (set-preference "versioning grain" pre-grain)
  ) ;let*
) ;tm-define

(tm-define (accept-diff)
  (let ((t (tree-innermost 'version-both)))
    (when t
      (let* ((new-val (tree-ref t 1)) (p (tree-up t)) (i (tree-index t)))
        (tree-remove! p i 1)
        (when (not (tree-is? new-val 'version-suppressed))
          (insert new-val)
        ) ;when
      ) ;let*
    ) ;when
  ) ;let
  (diff-feedback 'accept)
  (diff-scan-next)
) ;tm-define

(tm-define (reject-diff)
  (let ((t (tree-innermost 'version-both)))
    (when t
      (let* ((old-val (tree-ref t 0)) (p (tree-up t)) (i (tree-index t)))
        (tree-remove! p i 1)
        (when (not (tree-is? old-val 'version-suppressed))
          (insert old-val)
        ) ;when
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
  (delayed (:idle 0) (diff-check-popup))
) ;tm-define

(tm-define (mouse-event key x y mods time data)
  (:require (diff-enable?))
  (former key x y mods time data)
  (when (not (== key "move"))
    (delayed (:idle 0) (diff-check-popup))
  ) ;when
) ;tm-define
