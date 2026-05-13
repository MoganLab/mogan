;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : sidebar-session.scm
;; DESCRIPTION : sidebar session engine (adapted from session-edit)
;; COPYRIGHT   : (C) 2025--2026  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (dynamic sidebar-session)
  (:use (utils library tree)
        (utils library cursor)
        (utils plugins plugin-cmd)
        (utils plugins plugin-eval)
        (dynamic session-drd)
        (dynamic fold-edit)
        (kernel gui menu-widget)
        (texmacs texmacs tm-files)
        (generic chat-sidebar-ui)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Debug logging
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define sidebar-session-debug-log-file "/tmp/sidebar-session-debug.log")
(define sidebar-session-debug-counter 0)

(define (sidebar-session-debug msg . args)
  (set! sidebar-session-debug-counter (+ sidebar-session-debug-counter 1))
  (let* ((prefix (string-append "[SESSION-" (number->string sidebar-session-debug-counter) "] "))
         (log-msg (if (null? args)
                      (string-append prefix msg "\n")
                      (string-append prefix msg " => " (object->string args) "\n"))))
    (display log-msg)
    (catch #t
      (lambda ()
        (with-output-to-file sidebar-session-debug-log-file
          (lambda () (display log-msg))
          'append))
      (lambda e (display "[WARN] Failed to write session log file\n")))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; State
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define sidebar-active-session #f)
(define sidebar-session-serial 0)

(tm-define (sidebar-session-active?)
  (:synopsis "Check if a sidebar session is active")
  (nnull? sidebar-active-session))

(tm-define (sidebar-session-id)
  (:synopsis "Get current sidebar session id")
  (if sidebar-active-session
      (cdr sidebar-active-session)
      #f))

(tm-define (sidebar-session-busy?)
  (:synopsis "Check if the sidebar session is busy processing")
  (and sidebar-active-session
       (with (lan ses) sidebar-active-session
         (== (connection-status lan ses) 3))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Buffer names (re-export from chat-sidebar-ui for local use)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (msg-buf) (chat-sidebar-message-buffer-name))
(define (in-buf)  (chat-sidebar-input-buffer-name))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Encode / Decode / Detach (adapted from session-edit.scm)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (sidebar-session-encode in out next opts)
  (list (list sidebar-session-do
              sidebar-session-notify
              sidebar-session-next
              sidebar-session-cancel)
        in
        (tree->tree-pointer out)
        (tree->tree-pointer next)
        opts))

(define (sidebar-session-decode l)
  (list (second l)
        (tree-pointer->tree (third l))
        (tree-pointer->tree (fourth l))
        (fifth l)))

(define (sidebar-session-detach l)
  (tree-pointer-detach (third l))
  (tree-pointer-detach (fourth l)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Helpers (adapted from session-edit.scm)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (var-tree-children t)
  (with r (tree-children t)
    (if (and (nnull? r) (tree-empty? (cAr r))) (cDr r) r)))

(define (sidebar-session-flatten-stree x)
  (cond ((string? x) x)
        ((pair? x)
         (apply string-append
                (map sidebar-session-flatten-stree (cdr x))))
        (else "")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Input helpers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Read only the editable area (child 1 of input node) from input buffer
(tm-define (sidebar-session-read-input)
  (:synopsis "Read trimmed text from the input node's editable area")
  (with-buffer (in-buf)
    (with t (tree-search (buffer-tree)
                         (lambda (n) (tree-is? n 'input)))
      (if t
          (string-trim-spaces
            (sidebar-session-flatten-stree (tree->stree (tree-ref t 1))))
          ""))))

;; Clear only the editable area of the input node, preserving structure
(define (sidebar-session-clear-input!)
  (with-buffer (in-buf)
    (with t (tree-search (buffer-tree)
                         (lambda (n) (tree-is? n 'input)))
      (when t
        (tree-assign! (tree-ref t 1) '(document ""))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Output handlers (adapted from session-edit.scm)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (sidebar-session-output t u)
  (when (tm-func? t 'document)
    (with i (tree-arity t)
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'script-busy))
          (set! i (- i 1)))
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'errput))
          (set! i (- i 1)))
      (when (tm-func? u 'document)
        (tree-insert! t i (var-tree-children u))
        (set-user-active #f)))))

(define (sidebar-session-errput t u)
  (when (tm-func? t 'document)
    (with i (tree-arity t)
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'script-busy))
          (set! i (- i 1)))
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'errput))
          (set! i (- i 1))
          (tree-insert! t i '((errput (document)))))
      (sidebar-session-output (tree-ref t i 0) u))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Core callbacks (adapted from session-edit.scm, with with-buffer switching)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (sidebar-session-do lan ses)
  (sidebar-session-debug "do" (list lan ses))
  (with l (pending-ref lan ses)
    (with (in out next opts) (sidebar-session-decode (car l))
      (if (tree-empty? in)
          (begin
            (sidebar-session-debug "do: empty input, plugin-next")
            (plugin-next lan ses))
          (begin
            (sidebar-session-debug "do: plugin-write")
            (plugin-write lan ses in :session)
            (sidebar-session-debug "do: done"))))))

(define (sidebar-session-next lan ses)
  (sidebar-session-debug "next" (list lan ses))
  (with l (pending-ref lan ses)
    (with (in out next opts) (sidebar-session-decode (car l))
      ;; Remove script-busy if present
      (with-buffer (msg-buf)
        (when (and (tm-func? out 'document)
                   (tm-func? (tree-ref out :last) 'script-busy))
          (let* ((dt (plugin-timing lan ses))
                 (ts (if (< dt 1000)
                         (string-append (number->string dt) " msec")
                         (string-append (number->string (/ dt 1000.0)) " sec"))))
            (if (and (in? :timings opts) (>= dt 1))
                (tree-set (tree-ref out :last) `(timing ,ts))
                (tree-remove! out (- (tree-arity out) 1) 1)))))
      (sidebar-session-detach (car l))
      (sidebar-session-debug "next: done"))))

(define (sidebar-session-notify lan ses ch t)
  (with l (pending-ref lan ses)
    (with (in out next opts) (sidebar-session-decode (car l))
      (cond ((== ch "output")
             (with-buffer (msg-buf)
               (sidebar-session-output out t)))
            ((== ch "error")
             (with-buffer (msg-buf)
               (sidebar-session-errput out t)))
            ((== ch "prompt")
             (with-buffer (in-buf)
               (if (and (== (length l) 1) (tree-empty? (tree-ref next 1)))
                   (tree-set! next 0 (tree-copy t)))))
            ((and (== ch "input") (null? (cdr l)))
             (with-buffer (in-buf)
               (tree-set! next 1 t)))))))

(define (sidebar-session-cancel lan ses dead?)
  (sidebar-session-debug "cancel" (list lan ses dead?))
  (with l (pending-ref lan ses)
    (with (in out next opts) (sidebar-session-decode (car l))
      (with-buffer (msg-buf)
        (when (and (tm-func? out 'document)
                   (tm-func? (tree-ref out :last) 'script-busy))
          (tree-assign (tree-ref out :last)
                       (if dead? '(script-dead) '(script-interrupted)))))
      (sidebar-session-detach (car l))
      (sidebar-session-debug "cancel: done"))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Feed (adapted from session-feed, with with-buffer)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (sidebar-session-feed lan ses in out next opts)
  (sidebar-session-debug "feed" (list lan ses in opts))
  (set! in (plugin-preprocess lan ses in opts))
  (with-buffer (msg-buf)
    (tree-assign! out '(document (script-busy))))
  (with x (sidebar-session-encode in out next opts)
    (apply plugin-feed `(,lan ,ses ,@(car x) ,(cdr x))))
  (sidebar-session-debug "feed: done"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Session creation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (sidebar-session-gen-id model)
  (set! sidebar-session-serial (+ sidebar-session-serial 1))
  ;; Format: "Model:sidebar:timestamp-serial"
  ;; connection-info extracts text before first colon as variant name,
  ;; so "Model:sidebar:..." → variant = "Model" → correct launcher
  (string-append model ":sidebar:"
                 (number->string (texmacs-time)) "-"
                 (number->string sidebar-session-serial)))

(tm-define (make-sidebar-session model)
  (:synopsis "Create a new sidebar session and open the sidebar")
  (:argument model "Model name")

  (sidebar-session-debug "=== make-sidebar-session START ===" model)

  ;; 1. Stop any existing session
  (when sidebar-active-session
    (sidebar-session-debug "stopping existing session")
    (sidebar-session-stop))

  ;; 2. Open sidebar first — this triggers C++ to create buffer widgets
  ;;    and bind them to the message/input buffer names
  (open-chat-sidebar)
  (sidebar-session-debug "sidebar opened")

  ;; 3. Ensure buffers exist (created by C++ widget or manually)
  (chat-sidebar-ensure-buffer! (msg-buf))
  (chat-sidebar-ensure-buffer! (in-buf))

  ;; 4. Set up message buffer structure: (document (output (document "")))
  ;;    The initial output node is for the :start response (welcome message)
  (buffer-set-body (msg-buf) '(document (output (document ""))))
  (buffer-pretend-saved (msg-buf))
  (sidebar-session-debug "message buffer set up")

  ;; 5. Set up input buffer structure: (document (input (document "prompt") (document "")))
  ;;    The input node is reused across rounds; child 0 = prompt, child 1 = editable area
  (define prompt (plugin-prompt "llm" model))
  (buffer-set-body (in-buf)
                   `(document (input (document ,prompt) (document ""))))
  (buffer-pretend-saved (in-buf))
  (sidebar-session-debug "input buffer set up" prompt)

  ;; 6. Generate session id
  (define session-id (sidebar-session-gen-id model))
  (sidebar-session-debug "session-id" session-id)

  ;; 7. Find out/next nodes (need with-buffer for tree context)
  (define out-tree
    (with-buffer (msg-buf)
      (with t (tree-search (buffer-tree)
                           (lambda (n) (tree-is? n 'output)))
        (and t (tree-ref t 0)))))

  (define next-tree
    (with-buffer (in-buf)
      (tree-search (buffer-tree)
                   (lambda (n) (tree-is? n 'input)))))

  (sidebar-session-debug "nodes" (list out-tree next-tree))

  ;; 8. Enable text input for this session
  (session-enable-text-input "llm" session-id)

  ;; 9. Start session feed with :start
  ;;    plugin-feed with :start triggers plugin-start if not running,
  ;;    or plugin-next if already running. No need for separate plugin-write.
  (sidebar-session-feed "llm" session-id :start out-tree next-tree '())
  (sidebar-session-debug "feed started")

  ;; 10. Register active session
  (set! sidebar-active-session (cons "llm" session-id))
  (sidebar-session-debug "active session registered" sidebar-active-session)

  (sidebar-session-debug "=== make-sidebar-session END ===")
  session-id)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Send / Stop / Cancel
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Multi-turn send: each round creates a new output node and feeds through
;; the session mechanism, so the pending queue and callbacks work correctly.
(tm-define (sidebar-session-send text)
  (:synopsis "Send user text to sidebar session via session-feed")
  (:argument text "User input text")

  (sidebar-session-debug "send" text)
  (when (and sidebar-active-session (!= text ""))
    (with (lan ses) sidebar-active-session

      ;; 1. Append user message and new output node to message buffer
      (with-buffer (msg-buf)
        (let* ((doc (buffer-get-body (msg-buf))))
          (when doc
            ;; Append user message with separator
            (tree-insert! doc (tree-arity doc)
              (list `(concat (with "font-series" "bold" "User: ") ,text)
                    '(document "")))
            ;; Append new output node for agent response
            (tree-insert! doc (tree-arity doc)
              '((output (document "")))))))

      ;; 2. Get out reference (document inside the new output node)
      (define out-tree
        (with-buffer (msg-buf)
          (let* ((doc (buffer-get-body (msg-buf)))
                 (out-node (tree-ref doc :last)))
            (and (tree-is? out-node 'output)
                 (tree-ref out-node 0)))))

      ;; 3. Clear input content (preserve input node structure)
      (sidebar-session-clear-input!)

      ;; 4. Get next reference (input node in input buffer)
      (define next-tree
        (with-buffer (in-buf)
          (tree-search (buffer-tree)
                       (lambda (n) (tree-is? n 'input)))))

      ;; 5. Feed through session mechanism
      ;;    This creates a pending entry, sets script-busy on out,
      ;;    and triggers plugin-do → sidebar-session-do → plugin-write
      (when (and out-tree next-tree)
        (sidebar-session-feed lan ses text out-tree next-tree '()))
      (when (not out-tree)
        (sidebar-session-debug "send: failed to get out-tree"))
      (when (not next-tree)
        (sidebar-session-debug "send: failed to get next-tree")))))

(tm-define (sidebar-session-stop)
  (:synopsis "Stop the active sidebar session")

  (sidebar-session-debug "stop")
  (when sidebar-active-session
    (with (lan ses) sidebar-active-session
      (sidebar-session-debug "stopping" (list lan ses))
      ;; Interrupt any running evaluation
      (connection-interrupt lan ses)
      ;; Stop the plugin connection (use explicit lan/ses, not env vars)
      (if (!= (connection-status lan ses) 0)
          (connection-stop lan ses))
      ;; Append end marker to message buffer
      (with-buffer (msg-buf)
        (let* ((doc (buffer-get-body (msg-buf)))
               (arity (if doc (tree-arity doc) 0)))
          (when doc
            (tree-insert! doc arity
                          (list '(with "color" "dark grey"
                                        (document "[Session ended]"))))))))

    ;; Clear state
    (set! sidebar-active-session #f)
    (sidebar-session-debug "stop: done")))

(tm-define (sidebar-session-cancel)
  (:synopsis "Cancel current request in sidebar session")

  (sidebar-session-debug "cancel")
  (when sidebar-active-session
    (with (lan ses) sidebar-active-session
      (sidebar-session-debug "cancelling" (list lan ses))
      ;; Send interrupt signal; the plugin mechanism handles the rest:
      ;; - If plugin catches interrupt: status 2 → plugin-next → normal cleanup
      ;; - If plugin dies: status 0 → plugin-cancel with dead?=#t → cancel callback
      (if (== (connection-status lan ses) 3)
          (connection-interrupt lan ses))))
  (sidebar-session-debug "cancel: done"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; C++ button entry points
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Called by C++ Send button: (chat-sidebar-send)
(tm-define (chat-sidebar-send)
  (:synopsis "Send input text (called by C++ Send button)")
  (when (sidebar-session-active?)
    (let ((text (sidebar-session-read-input)))
      (when (!= text "")
        (sidebar-session-send text)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Menu integration
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-sidebar-session model)
  (:synopsis "Open sidebar with a new session")
  (:interactive #t)
  (:argument model "Model")
  (make-sidebar-session model))

(tm-define (close-sidebar-session)
  (:synopsis "Close sidebar and stop session")
  (:interactive #t)
  (sidebar-session-stop)
  (close-chat-sidebar))

(tm-define (toggle-sidebar-session model)
  (:synopsis "Toggle sidebar session")
  (:interactive #t)
  (:argument model "Model")
  (if (chat-sidebar-visible?)
      (close-sidebar-session)
      (open-sidebar-session model)))
