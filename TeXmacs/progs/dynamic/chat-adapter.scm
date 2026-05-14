;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-adapter.scm
;; DESCRIPTION : Adapter layer between chat tab container and session engine
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (dynamic chat-adapter)
  (:use (dynamic chat-tab-session)
    (texmacs texmacs tm-files)
    (texmacs texmacs tm-server)
  ) ;:use
) ;texmacs-module

(define chat-tab-url (string->url "tmfs://chat-tab"))

(tm-define (open-llm-chat-tab . model-opt)
  (:synopsis "Open or switch to the LLM Chat tab")
  (when (nnull? model-opt)
    (chat-tab-session-select-model (car model-opt))
  ) ;when
  (if (buffer-exists? chat-tab-url)
    (switch-to-buffer chat-tab-url)
    (begin
      (buffer-set chat-tab-url '(document ""))
      (buffer-set-title chat-tab-url "Chat")
      (switch-to-buffer chat-tab-url)
      (buffer-pretend-saved chat-tab-url)
    ) ;begin
  ) ;if
) ;tm-define

(tm-define (chat-tab-send message-buffer input-buffer body)
  (:synopsis "Adapter send entry for a chat tab")
  (:argument message-buffer "Message buffer name")
  (:argument input-buffer "Input buffer name")
  (:argument body "Input message tree")
  (chat-tab-session-send message-buffer input-buffer body)
) ;tm-define
