;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-widgets-test.scm
;; DESCRIPTION : Test suite for chat sidebar echo controller
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

(define (test-chat-sidebar-send-echo)
  (chat-sidebar-reset!)
  (buffer-set-body "tmfs://aux/chat-sidebar-input" '(document "hello"))
  (chat-sidebar-send)
  (check (tree->stree (buffer-get-body "tmfs://aux/chat-sidebar-body"))
         => '(document
               (concat (with "font-series" "bold" "User"))
               "hello"
               ""
               (concat (with "font-series" "bold" "Agent"))
               (concat (with "font-shape" "italic" "Thinking: ")
                       "preparing demo response")
               "echo: hello"
               (concat (with "font-series" "bold" "Explored"))
               (concat "  " "Search demo request")
               (concat "    " "Input: hello")
               (concat (with "font-series" "bold" "Permission"))
               "Allow demo tool call?"
               (concat "[" "yes" "] [" "no" "]")
               (concat (with "font-series" "bold" "File diff"))
               (concat "demo.txt" ": " "show all chat item types")
               (with "color" "red" (concat "- " "old demo payload"))
               (with "color" "green" (concat "+ " "echo demo payload"))))
  (check (tree->stree (buffer-get-body "tmfs://aux/chat-sidebar-input"))
         => '(document "")))

(tm-define (regtest-chat-widgets)
  (test-chat-sidebar-send-echo)
  (check-report))

(tm-define (test_chat-widgets-test)
  (regtest-chat-widgets))
