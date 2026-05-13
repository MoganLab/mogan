;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-sidebar-ui.scm
;; DESCRIPTION : pure UI layer for chat sidebar (buffer management, visibility)
;; COPYRIGHT   : (C) 2025--2026  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic chat-sidebar-ui)
  (:use (utils library tree)
        (utils library cursor)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Debug logging
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define sidebar-debug-log-file "/tmp/sidebar-session-debug.log")
(define sidebar-debug-counter 0)

(define (sidebar-debug msg . args)
  (set! sidebar-debug-counter (+ sidebar-debug-counter 1))
  (let* ((log-msg (if (null? args)
                      (string-append "[UI-" (number->string sidebar-debug-counter) "] " msg "\n")
                      (string-append "[UI-" (number->string sidebar-debug-counter) "] " msg
                                     " => " (object->string args) "\n"))))
    (display log-msg)
    (catch #t
      (lambda ()
        (with-output-to-file sidebar-debug-log-file
          (lambda () (display log-msg))
          'append))
      (lambda e (display "[WARN] Failed to write UI log file\n")))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Buffer management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define chat-sidebar-message-buffer "tmfs://aux/chat-sidebar-body")
(define chat-sidebar-input-buffer   "tmfs://aux/chat-sidebar-input")

(tm-define (chat-sidebar-message-buffer-name)
  (:synopsis "Get message buffer name")
  chat-sidebar-message-buffer)

(tm-define (chat-sidebar-input-buffer-name)
  (:synopsis "Get input buffer name")
  chat-sidebar-input-buffer)

(tm-define (chat-sidebar-ensure-buffer! name)
  (:synopsis "Ensure buffer exists, create if not")
  (sidebar-debug "ensure-buffer" name)
  (if (buffer-exists? name)
      (begin
        (sidebar-debug "buffer exists" name)
        name)
      (begin
        (sidebar-debug "creating buffer" name)
        (buffer-set-body name '(document ""))
        (buffer-pretend-saved name)
        (sidebar-debug "buffer created" name)
        name)))

(tm-define (chat-sidebar-clear-input!)
  (:synopsis "Clear input buffer")
  (sidebar-debug "clear-input!")
  (let ((name (chat-sidebar-ensure-buffer! chat-sidebar-input-buffer)))
    (buffer-set-body name '(document ""))
    (buffer-pretend-saved name)
    (sidebar-debug "clear-input! done")))

(tm-define (chat-sidebar-clear-message!)
  (:synopsis "Clear message buffer")
  (sidebar-debug "clear-message!")
  (let ((name (chat-sidebar-ensure-buffer! chat-sidebar-message-buffer)))
    (buffer-set-body name '(document ""))
    (buffer-pretend-saved name)
    (sidebar-debug "clear-message! done")))

(tm-define (chat-sidebar-reset!)
  (:synopsis "Reset both buffers")
  (sidebar-debug "reset!")
  (chat-sidebar-ensure-buffer! chat-sidebar-message-buffer)
  (chat-sidebar-ensure-buffer! chat-sidebar-input-buffer)
  (buffer-set-body chat-sidebar-message-buffer '(document ""))
  (buffer-set-body chat-sidebar-input-buffer '(document ""))
  (buffer-pretend-saved chat-sidebar-message-buffer)
  (buffer-pretend-saved chat-sidebar-input-buffer)
  (sidebar-debug "reset! done"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Refresh (called by C++ after creating buffer widgets)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (chat-sidebar-refresh!)
  (:synopsis "Refresh chat sidebar (called by C++ after widget creation)")
  (sidebar-debug "refresh!")
  ;; No-op in session-based architecture: buffer content is managed by
  ;; sidebar-session.scm via make-sidebar-session. This function exists
  ;; because C++ qt_tm_widget.cpp calls it after creating buffer widgets.
  (noop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Visibility control
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-chat-sidebar)
  (:synopsis "Open chat sidebar")
  (:interactive #t)
  (sidebar-debug "open-chat-sidebar")
  (show-chat-sidebar #t)
  (sidebar-debug "open-chat-sidebar done"))

(tm-define (close-chat-sidebar)
  (:synopsis "Close chat sidebar")
  (:interactive #t)
  (sidebar-debug "close-chat-sidebar")
  (show-chat-sidebar #f)
  (sidebar-debug "close-chat-sidebar done"))

(tm-define (toggle-chat-sidebar)
  (:synopsis "Toggle chat sidebar visibility")
  (:interactive #t)
  (sidebar-debug "toggle-chat-sidebar" (chat-sidebar-visible?))
  (if (chat-sidebar-visible?)
      (close-chat-sidebar)
      (open-chat-sidebar)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; UI state
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (chat-sidebar-set-send-state enabled?)
  (:synopsis "Set send button state")
  (sidebar-debug "set-send-state" enabled?)
  ;; TODO: wire to C++ widget if needed
  (noop))
