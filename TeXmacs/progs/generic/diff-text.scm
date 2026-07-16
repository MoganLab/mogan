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

(define (string-reverse s)
  (list->string (reverse (string->list s)))
) ;define

(define (find-version-both-in-tree t)
  (cond ((not t) #f)
        ((tree-is? t 'version-both) t)
        ((tree-atomic? t) #f)
        ((== (tree-arity t) 0) #f)
        (else
          (let loop ((i 0))
            (if (>= i (tree-arity t))
                #f
                (or (find-version-both-in-tree (tree-ref t i))
                    (loop (+ i 1)))
            ) ;if
          ) ;let
        ) ;else
  ) ;cond
) ;define

(define (get-diff-tree)
  (let ((t (tree-innermost 'version-both)))
    (if t
        t
        (find-version-both-in-tree (root-tree))
    ) ;if
  ) ;let
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
  ;; Action can be 'accept, 'reject, 'ignore
  (noop)
) ;tm-define

;; =============================================================================
;; Diff Text core control flow
;; =============================================================================

(tm-define (trigger-diff-text)
  (let* ((sel (selection-tree))
         (origin_str (tm->string sel))
         (suggested_str (string-reverse origin_str))
         (origin_text `(framed ,sel))
         (suggested_text `(framed ,suggested_str)))
    (clipboard-cut "primary")
    (insert `(version-both ,origin_text ,suggested_text))
    (insert-return)
    (set! diff-active? #t)
    (show-diff-popup)
  ) ;let*
) ;tm-define

(tm-define (accept-diff)
  (let ((t (get-diff-tree)))
    (when t
      (let* ((framed-node (tree-ref t 1))
             (new-val (tree-ref framed-node 0)))
        (tree-assign! t new-val)
      ) ;let*
    ) ;when
  ) ;let
  (set! diff-active? #f)
  (hide-diff-popup)
  (diff-feedback 'accept)
) ;tm-define

(tm-define (reject-diff)
  (let ((t (get-diff-tree)))
    (when t
      (let* ((framed-node (tree-ref t 0))
             (old-val (tree-ref framed-node 0)))
        (tree-assign! t old-val)
      ) ;let*
    ) ;when
  ) ;let
  (set! diff-active? #f)
  (hide-diff-popup)
  (diff-feedback 'reject)
) ;tm-define

(tm-define (ignore-diff)
  (reject-diff)
) ;tm-define

;; =============================================================================
;; Keyboard Hook
;; =============================================================================

(tm-define (kbd-tab)
  (:require (and (diff-enable?) (selection-active?)))
  (trigger-diff-text)
) ;tm-define

(tm-define (keyboard-press key time)
  (:require (is-diff-active?))
  (cond ((== key "return") (accept-diff))
        ((== key "backspace") (reject-diff))
        (else (ignore-diff) (delayed (:idle 0) (keyboard-press key time)))
  ) ;cond
) ;tm-define

(tm-define (mouse-event key x y mods time data)
  (:require (is-diff-active?))
  (former key x y mods time data)
  (when (not (== key "move"))
    (ignore-diff)
  ) ;when
) ;tm-define
