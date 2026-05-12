;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-controller-test.scm
;; DESCRIPTION : Test suite for chat controller
;; COPYRIGHT   : (C) 2026  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic chat-controller-test)
  (:use (generic chat-controller)
        (generic chat-model)))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-chat-controller-reset)
  (chat-controller-reset!)
  (let ((session (chat-controller-session)))
    (check (chat-session-block-count session) => 0)
    (check (chat-session-current-block-id session) => #f)))

(define (test-chat-controller-append-text-block)
  (chat-controller-reset!)
  (chat-controller-append-text-block! 'user "hello" 0 0 'user-submitted)
  (let* ((session (chat-controller-session))
         (block (chat-session-block-ref session 0))
         (item (chat-block-item-ref block 0)))
    (check (chat-session-block-count session) => 1)
    (check (chat-block-actor block) => 'user)
    (check (chat-block-sealed? block) => #t)
    (check (chat-block-close-reason block) => 'user-submitted)
    (check (chat-item-type item) => 'text)
    (check (chat-item-content item) => "hello")))

(tm-define (regtest-chat-controller)
  (test-chat-controller-reset)
  (test-chat-controller-append-text-block)
  (check-report))

(tm-define (test_chat-controller-test)
  (regtest-chat-controller))
