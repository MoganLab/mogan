;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-sidebar-ui-test.scm
;; DESCRIPTION : Test suite for chat sidebar UI (pure UI layer)
;; COPYRIGHT   : (C) 2026  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic chat-sidebar-ui-test)
  (:use (generic chat-sidebar-ui)))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-chat-sidebar-buffer-management)
  (chat-sidebar-reset!)
  (check (buffer-exists? (chat-sidebar-message-buffer-name)) => #t)
  (check (buffer-exists? (chat-sidebar-input-buffer-name)) => #t)
  (check (tree->stree (buffer-get-body (chat-sidebar-message-buffer-name)))
         => '(document ""))
  (check (tree->stree (buffer-get-body (chat-sidebar-input-buffer-name)))
         => '(document "")))

(define (test-chat-sidebar-ensure-buffer)
  (chat-sidebar-ensure-buffer! (chat-sidebar-message-buffer-name))
  (check (buffer-exists? (chat-sidebar-message-buffer-name)) => #t)
  (chat-sidebar-ensure-buffer! (chat-sidebar-input-buffer-name))
  (check (buffer-exists? (chat-sidebar-input-buffer-name)) => #t))

(define (test-chat-sidebar-clear)
  (chat-sidebar-clear-input!)
  (check (tree? (buffer-get-body (chat-sidebar-input-buffer-name))) => #t)
  (chat-sidebar-clear-message!)
  (check (tree? (buffer-get-body (chat-sidebar-message-buffer-name))) => #t))

(tm-define (regtest-chat-sidebar-ui)
  (test-chat-sidebar-buffer-management)
  (test-chat-sidebar-ensure-buffer)
  (test-chat-sidebar-clear)
  (check-report))

(tm-define (test_chat-sidebar-ui-test)
  (regtest-chat-sidebar-ui))
