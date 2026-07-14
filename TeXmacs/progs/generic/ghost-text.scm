
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ghost-text.scm
;; DESCRIPTION : Ghost Text feature for MoganSTEM
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic ghost-text)
  (:use (kernel texmacs tm-define)
        (utils library cursor)))

;; =============================================================================
;; State variables for Ghost Text
;; =============================================================================
(define ghost-serial 0)
(define ghost-active? #f)
(define ghost-mark 0)
(define ghost-content "")

;; Local acceptance threshold (91%)
(define ghost-acceptance-threshold 0.91)

(tm-define (is-ghost-active?)
  ghost-active?)

;; =============================================================================
;; Model evaluation & Feedback functions
;; =============================================================================
(tm-define (ghost-feedback action)
  ;; Action can be 'accept, 'reject, or 'ignore
  (noop))

(define (mock-cloud-api)
  ;; Simulate cloud API response with confidence level (100%)
  (let* ((candidates '(" because of this" " to implement a demo" " as specified in the task" " with elegant Scheme code"))
         (text (list-ref candidates (random (length candidates))))
         (confidence 1.0))
    (cons text confidence)))

;; =============================================================================
;; Ghost Text core control flow
;; =============================================================================
(tm-define (trigger-ghost-text)
  (when ghost-active?
    (ignore-ghost))
  (set! ghost-serial (+ ghost-serial 1))
  (let ((current ghost-serial))
    (delayed (:idle 500)
      (when (== ghost-serial current)
        (generate-ghost-text)))))

(tm-define (generate-ghost-text)
  (let* ((api-res (mock-cloud-api))
         (text (car api-res))
         (confidence (cdr api-res)))
    ;; Check if model confidence meets local acceptance threshold (91%)
    (when (>= confidence ghost-acceptance-threshold)
      (set! ghost-content text)
      (set! ghost-mark (mark-new))
      (mark-start ghost-mark)
      (archive-state)
      ;; cursor-after ensures the cursor position remains before the ghost text
      (cursor-after (insert `(ghost ,text)))
      (set! ghost-active? #t)
      (show-ghost-popup))))

(tm-define (accept-ghost)
  (let ((text ghost-content))
    (set! ghost-active? #f)
    (hide-ghost-popup)
    (mark-cancel ghost-mark)
    (insert text)
    (ghost-feedback 'accept)))

(tm-define (reject-ghost)
  (set! ghost-active? #f)
  (hide-ghost-popup)
  (mark-cancel ghost-mark)
  (ghost-feedback 'reject))

(tm-define (ignore-ghost)
  (set! ghost-active? #f)
  (hide-ghost-popup)
  (mark-cancel ghost-mark)
  (ghost-feedback 'ignore))

;; =============================================================================
;; Intercept handlers (keyboard and mouse)
;; =============================================================================
(tm-define (kbd-insert s)
  (former s)
  (trigger-ghost-text))

(tm-define (keyboard-press key time)
  (:require (is-ghost-active?))
  (cond ((== key "right") (accept-ghost))
        ((== key "escape") (reject-ghost))
        (else
          (ignore-ghost)
          (delayed (:idle 0) (keyboard-press key time)))))

(tm-define (mouse-event key x y mods time data)
  (if (and (is-ghost-active?) (not (== key "move")))
      (begin
        (ignore-ghost)
        (delayed (:idle 0) (mouse-event key x y mods time data)))
      (former key x y mods time data)))
