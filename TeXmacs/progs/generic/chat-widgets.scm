
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-widgets.scm
;; DESCRIPTION : commands for a static chat sidebar skeleton
;; COPYRIGHT   : (C) 2025  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic chat-widgets)
  (:use (generic chat-model)
        (generic chat-render)))

(define chat-sidebar-message-buffer "tmfs://aux/chat-sidebar-body")
(define chat-sidebar-input-buffer "tmfs://aux/chat-sidebar-input")
(define chat-sidebar-session #f)
(define chat-sidebar-next-id 0)

(define (chat-sidebar-next-id-string prefix)
  (set! chat-sidebar-next-id (+ chat-sidebar-next-id 1))
  (string-append prefix "-" (number->string chat-sidebar-next-id)))

(tm-define (chat-sidebar-reset!)
  (set! chat-sidebar-session #f)
  (set! chat-sidebar-next-id 0)
  (chat-sidebar-ensure-buffer! chat-sidebar-message-buffer)
  (chat-sidebar-ensure-buffer! chat-sidebar-input-buffer)
  (buffer-set-body chat-sidebar-message-buffer '(document ""))
  (buffer-set-body chat-sidebar-input-buffer '(document ""))
  (buffer-pretend-saved chat-sidebar-message-buffer)
  (buffer-pretend-saved chat-sidebar-input-buffer))

(define (chat-sidebar-ensure-session!)
  (if chat-sidebar-session
      chat-sidebar-session
      (begin
        (set! chat-sidebar-session
              (make-chat-session
               (chat-sidebar-next-id-string "session")
               "tmfs://aux/chat-sidebar-session.json"
               0))
        chat-sidebar-session)))

(define (chat-sidebar-ensure-buffer! name)
  (if (buffer-exists? name)
      name
      (begin
        (buffer-set-body name '(document ""))
        (buffer-pretend-saved name)
        name)))

(define (chat-sidebar-flatten-stree x)
  (cond ((string? x) x)
        ((pair? x)
         (apply string-append (map chat-sidebar-flatten-stree (cdr x))))
        (else "")))

(define (chat-sidebar-read-input-text)
  (let* ((name (chat-sidebar-ensure-buffer! chat-sidebar-input-buffer))
         (body (buffer-get-body name)))
    (if body
        (string-trim-spaces (chat-sidebar-flatten-stree (tree->stree body)))
        "")))

(tm-define (chat-sidebar-refresh!)
  (let* ((session (chat-sidebar-ensure-session!))
         (name (chat-sidebar-ensure-buffer! chat-sidebar-message-buffer))
         (doc (chat-session->message-document session)))
    (buffer-set-body name doc)
    (buffer-pretend-saved name)))

(define (chat-sidebar-clear-input!)
  (let ((name (chat-sidebar-ensure-buffer! chat-sidebar-input-buffer)))
    (buffer-set-body name '(document ""))
    (buffer-pretend-saved name)))

(define (chat-sidebar-append-text-block! actor text start-at end-at close-reason)
  (let ((session (chat-sidebar-ensure-session!))
        (block-id (chat-sidebar-next-id-string "block"))
        (item-id (chat-sidebar-next-id-string "item")))
    (chat-session-open-block! session block-id actor start-at)
    (chat-session-append-item! session item-id 'text text #f end-at 'done)
    (chat-session-seal-current-block! session close-reason end-at)))


(tm-define (chat-sidebar-send)
  (:interactive #t)
  (let ((text (chat-sidebar-read-input-text)))
    (when (!= text "")
      (chat-sidebar-append-text-block! 'user text 0 0 'user-submitted)
      (chat-sidebar-clear-input!)
      (chat-sidebar-refresh!))))

(tm-define (open-chat-sidebar)
  (:interactive #t)
  (chat-sidebar-refresh!)
  (show-chat-sidebar #t))

(tm-define (close-chat-sidebar)
  (:interactive #t)
  (show-chat-sidebar #f))

(tm-define (toggle-chat-sidebar)
  (:interactive #t)
  (if (chat-sidebar-visible?)
      (close-chat-sidebar)
      (open-chat-sidebar)))
