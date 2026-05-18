;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0134.scm
;; DESCRIPTION : Unit tests for chat-tab-session unfolded-io-text migration
;; COPYRIGHT   : (C) 2026 Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (dynamic chat-tab-session))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-chat-tab-model-prompt)
  (check (chat-tab-model-prompt "DeepSeek-V4-Pro") => "V4> ")
  (check (chat-tab-model-prompt "gpt-4o") => "4o> ")
  (check (chat-tab-model-prompt "Kimi-K2.5") => "K2.5> ")
  (check (chat-tab-model-prompt "GPT-4") => "4> ")
  (check (chat-tab-model-prompt "Claude-3.7-Sonnet") => "3.7> ")
  (check (chat-tab-model-prompt "default") => "default> "))

(define (test-chat-tab-append-round-structure)
  (let* ((test-msg-url (string->url "tmfs://test-0134-msg"))
         (test-input-url (string->url "tmfs://test-0134-input"))
         (saved-model chat-tab-current-model))
    (buffer-set test-msg-url '(session "llm" "test-session" (document)))
    (buffer-pretend-saved test-msg-url)
    (chat-tab-set-state! test-msg-url (chat-tab-state test-input-url "DeepSeek-V4-Pro" "test-session"))
    (set! chat-tab-current-model "DeepSeek-V4-Pro")
    ;; Append first round
    (let ((out1 (chat-tab-append-round! test-msg-url (stree->tree '(document "hello")))))
      (check (tree->stree (buffer-get-body test-msg-url))
             => '(session "llm" "test-session"
                  (document
                    (unfolded-io-text
                      (document "V4> ") (document "hello")
                      (document "")))))
      ;; Check return value is the output document
      (check (tree->stree out1) => '(document ""))
      ;; Test output can be written to
      (chat-tab-output out1 (stree->tree '(document "response1")))
      (check (tree->stree (buffer-get-body test-msg-url))
             => '(session "llm" "test-session"
                  (document
                    (unfolded-io-text
                      (document "V4> ") (document "hello")
                      (document "" "response1")))))
      )
    ;; Append second round
    (let ((out2 (chat-tab-append-round! test-msg-url (stree->tree '(document "world")))))
      (check (tree->stree (buffer-get-body test-msg-url))
             => '(session "llm" "test-session"
                  (document
                    (unfolded-io-text
                      (document "V4> ") (document "hello")
                      (document "" "response1"))
                    (unfolded-io-text
                      (document "V4> ") (document "world")
                      (document ""))))))
    (set! chat-tab-current-model saved-model)
    ))

(define (test-chat-tab-message-document)
  (let* ((test-url (string->url "tmfs://test-0134-doc")))
    (buffer-set test-url '(session "llm" "test-session" (document "test")))
    (buffer-pretend-saved test-url)
    (let ((doc (chat-tab-message-document test-url)))
      (check (tree->stree doc) => '(document "test")))
    ))

(tm-define (test_0134)
  (test-chat-tab-model-prompt)
  (test-chat-tab-append-round-structure)
  (test-chat-tab-message-document)
  (check-report))
