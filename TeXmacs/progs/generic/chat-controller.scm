;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-controller.scm
;; DESCRIPTION : chat business controller
;; COPYRIGHT   : (C) 2026  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic chat-controller)
  (:use (generic chat-model)))

(define chat-controller-session* #f)
(define chat-controller-next-id 0)

(define (chat-controller-next-id-string prefix)
  (set! chat-controller-next-id (+ chat-controller-next-id 1))
  (string-append prefix "-" (number->string chat-controller-next-id)))

(define (chat-controller-ensure-session!)
  (if chat-controller-session*
      chat-controller-session*
      (begin
        (set! chat-controller-session*
              (make-chat-session
               (chat-controller-next-id-string "session")
               "tmfs://aux/chat-sidebar-session.json"
               0))
        chat-controller-session*)))

(tm-define (chat-controller-session)
  (chat-controller-ensure-session!))

(tm-define (chat-controller-reset!)
  (set! chat-controller-session* #f)
  (set! chat-controller-next-id 0))

(tm-define (chat-controller-append-text-block! actor text start-at end-at close-reason)
  (let ((session (chat-controller-ensure-session!))
        (block-id (chat-controller-next-id-string "block"))
        (item-id (chat-controller-next-id-string "item")))
    (chat-session-open-block! session block-id actor start-at)
    (chat-session-append-item! session item-id 'text text #f end-at 'done)
    (chat-session-seal-current-block! session close-reason end-at)))
