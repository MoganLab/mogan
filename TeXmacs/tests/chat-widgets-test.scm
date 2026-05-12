;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-widgets-test.scm
;; DESCRIPTION : Test suite for chat sidebar message controller
;; COPYRIGHT   : (C) 2026  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic chat-widgets-test)
  (:use (generic chat-widgets)))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-chat-sidebar-send-text)
  (chat-sidebar-reset!)
  (buffer-set-body "tmfs://aux/chat-sidebar-input" '(document "hello"))
  (chat-sidebar-send)
  (check (tree->stree (buffer-get-body "tmfs://aux/chat-sidebar-body"))
         => '(document
                (concat (with "font-series" "bold" "User"))
                "hello"))
  (check (tree->stree (buffer-get-body "tmfs://aux/chat-sidebar-input"))
         => '(document "")))

(tm-define (regtest-chat-widgets)
  (test-chat-sidebar-send-text)
  (check-report))

(tm-define (test_chat-widgets-test)
  (regtest-chat-widgets))
