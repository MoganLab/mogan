;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-render-test.scm
;; DESCRIPTION : Test suite for chat message rendering
;; COPYRIGHT   : (C) 2026  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic chat-render-test)
  (:use (generic chat-model)
        (generic chat-render)))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-chat-render-empty-session)
  (let ((session (make-chat-session "session-1" "/tmp/chat.json" 100)))
    (check (chat-session->message-document session)
           => '(document ""))))

(define (test-chat-render-user-text-block)
  (let ((session (make-chat-session "session-1" "/tmp/chat.json" 100)))
    (chat-session-open-block! session "block-1" 'user 101)
    (chat-session-append-item! session "item-1" 'text "Hello" #f 102 'done)
    (chat-session-seal-current-block! session 'user-submitted 103)
    (check (chat-session->message-document session)
           => '(document
                 (concat (with "font-series" "bold" "User"))
                 "Hello"))))

(define (test-chat-render-thinking-and-text)
  (let ((session (make-chat-session "session-1" "/tmp/chat.json" 100)))
    (chat-session-open-block! session "block-1" 'agent 101)
    (chat-session-append-item! session "item-1" 'thinking "searching" #f 102 'done)
    (chat-session-append-item! session "item-2" 'text "done" #f 103 'done)
    (chat-session-seal-current-block! session 'agent-finished 104)
    (check (chat-session->message-document session)
           => '(document
                 (concat (with "font-series" "bold" "Agent"))
                 (concat (with "font-shape" "italic" "Thinking: ") "searching")
                 "done"))))

(define (test-chat-render-tool-call)
  (let* ((session (make-chat-session "session-1" "/tmp/chat.json" 100))
         (payload `(("title" . "Explored")
                    ("entries" .
                     #((("label" . "Search goldfish")
                        ("children" .
                         #((("label" . "Search goldfish in .")
                             ("children" . #()))))))))))
    (chat-session-open-block! session "block-1" 'agent 101)
    (chat-session-append-item! session "item-1" 'tool-call #f payload 102 'done)
    (chat-session-seal-current-block! session 'agent-finished 103)
    (check (chat-session->message-document session)
           => '(document
                 (concat (with "font-series" "bold" "Agent"))
                 (concat (with "font-series" "bold" "Explored"))
                 (concat "  " "Search goldfish")
                 (concat "    " "Search goldfish in .")))))

(define (test-chat-render-tool-permission)
  (let* ((session (make-chat-session "session-1" "/tmp/chat.json" 100))
         (payload `(("question" . "xxx accept?")
                     ("approve-label" . "yes")
                     ("reject-label" . "no and tell agent what to do differently"))))
    (chat-session-open-block! session "block-1" 'agent 101)
    (chat-session-append-item! session "item-1" 'tool-permission #f payload 102 'pending)
    (chat-session-seal-current-block! session 'agent-stopped 103)
    (check (chat-session->message-document session)
           => '(document
                  (concat (with "font-series" "bold" "Agent"))
                  (concat (with "font-series" "bold" "Permission"))
                  (concat "Permission request: " "xxx accept?")))))

(define (test-chat-render-tool-permission-defaults)
  (let ((session (make-chat-session "session-1" "/tmp/chat.json" 100)))
    (chat-session-open-block! session "block-1" 'agent 101)
    (chat-session-append-item! session "item-1" 'tool-permission #f '() 102 'pending)
    (chat-session-seal-current-block! session 'agent-stopped 103)
    (check (chat-session->message-document session)
           => '(document
                  (concat (with "font-series" "bold" "Agent"))
                  (concat (with "font-series" "bold" "Permission"))
                  (concat "Permission request: " "Permission required")))))

(define (test-chat-render-file-diff)
  (let* ((session (make-chat-session "session-1" "/tmp/chat.json" 100))
         (payload `(("title" . "File diff")
                    ("files" .
                     #((("path" . "demo.txt")
                        ("summary" . "updated"))))
                    ("lines" .
                     #((("kind" . delete)
                        ("text" . "old line"))
                       (("kind" . add)
                        ("text" . "new line")))))))
    (chat-session-open-block! session "block-1" 'agent 101)
    (chat-session-append-item! session "item-1" 'file-diff #f payload 102 'done)
    (chat-session-seal-current-block! session 'agent-finished 103)
    (check (chat-session->message-document session)
           => '(document
                 (concat (with "font-series" "bold" "Agent"))
                 (concat (with "font-series" "bold" "File diff"))
                 (concat "demo.txt" ": " "updated")
                 (with "color" "red" (concat "- " "old line"))
                 (with "color" "green" (concat "+ " "new line"))))))

(define (test-chat-render-multiple-blocks)
  (let ((session (make-chat-session "session-1" "/tmp/chat.json" 100)))
    (chat-session-open-block! session "block-1" 'user 101)
    (chat-session-append-item! session "item-1" 'text "Hello" #f 102 'done)
    (chat-session-seal-current-block! session 'user-submitted 103)
    (chat-session-open-block! session "block-2" 'agent 104)
    (chat-session-append-item! session "item-2" 'text "echo: Hello" #f 105 'done)
    (chat-session-seal-current-block! session 'agent-finished 106)
    (check (chat-session->message-document session)
           => '(document
                 (concat (with "font-series" "bold" "User"))
                 "Hello"
                 ""
                 (concat (with "font-series" "bold" "Agent"))
                 "echo: Hello"))))

(define (test-chat-render-three-blocks-spacing)
  (let ((session (make-chat-session "session-1" "/tmp/chat.json" 100)))
    (chat-session-open-block! session "block-1" 'user 101)
    (chat-session-append-item! session "item-1" 'text "Hello" #f 102 'done)
    (chat-session-seal-current-block! session 'user-submitted 103)
    (chat-session-open-block! session "block-2" 'agent 104)
    (chat-session-append-item! session "item-2" 'text "echo: Hello" #f 105 'done)
    (chat-session-seal-current-block! session 'agent-finished 106)
    (chat-session-open-block! session "block-3" 'system 107)
    (chat-session-append-item! session "item-3" 'text "saved" #f 108 'done)
    (chat-session-seal-current-block! session 'agent-finished 109)
    (check (chat-session->message-document session)
           => '(document
                 (concat (with "font-series" "bold" "User"))
                 "Hello"
                 ""
                 (concat (with "font-series" "bold" "Agent"))
                 "echo: Hello"
                 ""
                 (concat (with "font-series" "bold" "System"))
                 "saved"))))

(define (test-chat-render-unknown-item-type)
  (let ((session (make-chat-session "session-1" "/tmp/chat.json" 100)))
    (chat-session-open-block! session "block-1" 'agent 101)
    (chat-session-append-item! session "item-1" 'custom-event #f #f 102 'done)
    (chat-session-seal-current-block! session 'agent-finished 103)
    (check (chat-session->message-document session)
           => '(document
                 (concat (with "font-series" "bold" "Agent"))
                 (concat (with "font-series" "bold" "Unknown item: ")
                         "custom-event")))))

(tm-define (regtest-chat-render)
  (test-chat-render-empty-session)
  (test-chat-render-user-text-block)
  (test-chat-render-thinking-and-text)
  (test-chat-render-tool-call)
  (test-chat-render-tool-permission)
  (test-chat-render-tool-permission-defaults)
  (test-chat-render-file-diff)
  (test-chat-render-multiple-blocks)
  (test-chat-render-three-blocks-spacing)
  (test-chat-render-unknown-item-type)
  (check-report))

(tm-define (test_chat-render-test)
  (regtest-chat-render))
