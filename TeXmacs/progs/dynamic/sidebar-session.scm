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
        (generic document-style)
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
  ;; nnull? returns #t for #f (because #f is not an empty list),
  ;; so we must use an explicit truthiness check
  (if sidebar-active-session #t #f))

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
(define (in-buf) (chat-sidebar-input-buffer-name))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tree search helper
;;
;; tree-search returns a LIST of matching trees, not a single tree.
;; This helper extracts the first match or returns #f.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (tree-find-first t pred?)
  (with l (tree-search t pred?)
    (if (nnull? l) (car l) #f)))

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
        (if next (tree->tree-pointer next) #f)
        opts))

(define (sidebar-session-decode l)
  (list (second l)
        (tree-pointer->tree (third l))
        (if (fourth l) (tree-pointer->tree (fourth l)) #f)
        (fifth l)))

(define (sidebar-session-detach l)
  (tree-pointer-detach (third l))
  (when (fourth l) (tree-pointer-detach (fourth l))))

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

;; Read text from the input buffer's plain document
(tm-define (sidebar-session-read-input)
  (:synopsis "Read trimmed text from the input buffer's plain document")
  (with-buffer (in-buf)
    (let ((body (buffer-get-body (in-buf))))
      (sidebar-session-debug "read-input: body type" (if (tree? body) 'tree #f))
      (if (tree? body)
          (string-trim-spaces
            (sidebar-session-flatten-stree (tree->stree body)))
          ""))))

;; Clear the input buffer content
(define (sidebar-session-clear-input!)
  (with-buffer (in-buf)
    (buffer-set-body (in-buf) '(document "")))
  (buffer-pretend-saved (in-buf))
  (sidebar-session-debug "input cleared"))

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
    (sidebar-session-debug "do: pending length" (length l))
    (when (nnull? l)
      (with (in out next opts) (sidebar-session-decode (car l))
        (sidebar-session-debug "do: in type" (if (tree? in) 'tree (if (symbol? in) in 'string)))
        (sidebar-session-debug "do: in content" (if (string? in) in (if (symbol? in) in "<tree>")))
        (sidebar-session-debug "do: connection-status" (connection-status lan ses))
        (if (or (tree-empty? in) (== in :start))
            (begin
              (sidebar-session-debug "do: empty/start input, plugin-next")
              (plugin-next lan ses))
            (begin
              (sidebar-session-debug "do: calling plugin-write" lan ses)
              (plugin-write lan ses in :session)
              (sidebar-session-debug "do: plugin-write done")))))))

(define (sidebar-session-next lan ses)
  (sidebar-session-debug "next" (list lan ses))
  (with l (pending-ref lan ses)
    (when (nnull? l)
      (with (in out next opts) (sidebar-session-decode (car l))
        (sidebar-session-debug "next: in type" (if (symbol? in) in (if (string? in) in 'tree)))
        (with-buffer (msg-buf)
          (when (and (tm-func? out 'document)
                     (tm-func? (tree-ref out :last) 'script-busy))
            (let* ((dt (plugin-timing lan ses))
                   (ts (if (< dt 1000)
                           (string-append (number->string dt) " msec")
                           (string-append (number->string (/ dt 1000.0)) " sec"))))
              (sidebar-session-debug "next: removing script-busy, timing" ts)
              (if (and (in? :timings opts) (>= dt 1))
                  (tree-set (tree-ref out :last) `(timing ,ts))
                  (tree-remove! out (- (tree-arity out) 1) 1)))))
        (sidebar-session-detach (car l))
        (sidebar-session-debug "next: done")))))

(define (sidebar-session-notify lan ses ch t)
  (sidebar-session-debug "notify" (list lan ses ch))
  (sidebar-session-debug "notify: data" (tree->stree t))
  (with l (pending-ref lan ses)
    (if (null? l)
        (sidebar-session-debug "notify: WARNING - pending queue is empty, dropping notification")
        (with (in out next opts) (sidebar-session-decode (car l))
          (cond ((== ch "output")
                 (sidebar-session-debug "notify: handling output")
                 (with-buffer (msg-buf)
                   (sidebar-session-output out t)))
                ((== ch "error")
                 (sidebar-session-debug "notify: handling error")
                 (with-buffer (msg-buf)
                   (sidebar-session-errput out t)))
                ((== ch "prompt")
                 ;; Prompt updates not needed for sidebar (no input node)
                 (sidebar-session-debug "notify: received prompt (ignored in sidebar mode)")))))))

(define (sidebar-session-cancel lan ses dead?)
  (sidebar-session-debug "cancel" (list lan ses dead?))
  (with l (pending-ref lan ses)
    (when (nnull? l)
      (with (in out next opts) (sidebar-session-decode (car l))
        (with-buffer (msg-buf)
          (when (and (tm-func? out 'document)
                     (tm-func? (tree-ref out :last) 'script-busy))
            (tree-assign (tree-ref out :last)
                         (if dead? '(script-dead) '(script-interrupted)))))
        (sidebar-session-detach (car l))
        (sidebar-session-debug "cancel: done")))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Feed (adapted from session-feed, with with-buffer)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (sidebar-session-feed lan ses in out next opts)
  (sidebar-session-debug "feed" (list lan ses (if (symbol? in) in (if (string? in) in 'tree)) opts))
  (sidebar-session-debug "feed: out tree valid?" (if (tree? out) #t #f))
  (sidebar-session-debug "feed: next tree" (if next "present" "not used"))
  (sidebar-session-debug "feed: connection-status before" (connection-status lan ses))
  (set! in (plugin-preprocess lan ses in opts))
  (with-buffer (msg-buf)
    (tree-assign! out '(document (script-busy))))
  (sidebar-session-debug "feed: assigned script-busy to out")
  (with x (sidebar-session-encode in out next opts)
    (sidebar-session-debug "feed: calling plugin-feed")
    (apply plugin-feed `(,lan ,ses ,@(car x) ,(cdr x))))
  (sidebar-session-debug "feed: plugin-feed returned")
  (sidebar-session-debug "feed: connection-status after" (connection-status lan ses)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Session creation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (sidebar-session-gen-id model)
  (set! sidebar-session-serial (+ sidebar-session-serial 1))
  ;; Format: "Model:sidebar:timestamp-serial"
  ;; connection-info extracts text before first colon as variant name,
  ;; so "Model:sidebar:..." -> variant = "Model" -> correct launcher
  (string-append model ":sidebar:"
                 (number->string (texmacs-time)) "-"
                 (number->string sidebar-session-serial)))

(tm-define (make-sidebar-session model)
  (:synopsis "Create a new sidebar session and open sidebar")
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
  (sidebar-session-debug "buffers ensured"
    (list (buffer-exists? (msg-buf)) (buffer-exists? (in-buf))))

  ;; 4. Set up message buffer structure:
  ;;    (document (session "llm" session-id (document (output (document "")))))
  ;;    The session tag wraps output/input to set prog-language/prog-session
  ;;    for correct macro dispatch (llm-output, llm-input, etc.)
  (define session-id (sidebar-session-gen-id model))
  (sidebar-session-debug "session-id" session-id)
  (buffer-set-body (msg-buf)
                   `(document (session "llm" ,session-id
                                   (document (output (document ""))))))
  (buffer-pretend-saved (msg-buf))
  (sidebar-session-debug "message buffer set up")

  ;; 5. Set up input buffer structure:
  ;;    (document "") - plain empty document without session/input tags.
  ;;    We intentionally avoid input/session tags because:
  ;;    - The standard kbd-enter handler checks field-input-context? which
  ;;      matches on input nodes, causing it to run session-evaluate ->
  ;;      field-process-input -> session-feed with silent-* callbacks
  ;;    - This routes output to the input buffer instead of message buffer
  ;;    - Instead, we use get-env "sidebar-input-mode" for kbd-enter detection
  (buffer-set-body (in-buf) '(document ""))
  (buffer-pretend-saved (in-buf))
  (sidebar-session-debug "input buffer set up (plain document)")

  ;; 6. Load style packages for proper macro rendering
  ;;    Set base style to "generic" (like make-session-in-sidebar does),
  ;;    then add main document packages and session-specific packages
  (let ((main-packs (with-buffer (current-buffer) (get-style-list))))
    (sidebar-session-debug "main document style packages" main-packs)
    ;; Build combined style list: "generic" base + main doc packages + session packages
    (let ((all-packs (append (list "generic")
                             main-packs
                             (list "session" "scripts" "std-utils"))))
      ;; Remove duplicates while preserving order
      (let ((seen (make-ahash-table))
            (unique-packs '()))
        (for-each
          (lambda (pack)
            (when (not (ahash-ref seen pack))
              (ahash-set! seen pack #t)
              (set! unique-packs (append unique-packs (list pack)))))
          all-packs)
        (sidebar-session-debug "combined style packages" unique-packs)
        (with-buffer (msg-buf) (set-style-list unique-packs))
        (with-buffer (in-buf) (set-style-list unique-packs)))))
  ;; Add llm package for LLM-specific overrides
  (if (url-exists? (url-unix "$TEXMACS_STYLE_PATH" "llm.ts"))
      (begin
        (with-buffer (msg-buf) (add-style-package "llm"))
        (with-buffer (in-buf) (add-style-package "llm"))
        (sidebar-session-debug "loaded llm package"))
      (sidebar-session-debug "llm.ts not found, skipping"))

  ;; 6b. Match font size from main document for consistent appearance
  ;;     The "generic" base style may use a different default font size
  ;;     than the main document's style (e.g., "article")
  (let ((main-font-size (with-buffer (current-buffer) (get-env "font-base-size"))))
    (sidebar-session-debug "main document font-base-size" main-font-size)
    (when (and main-font-size (string? main-font-size) (!= main-font-size ""))
      (with-buffer (msg-buf) (init-env "font-base-size" main-font-size))
      (with-buffer (in-buf) (init-env "font-base-size" main-font-size))
      (sidebar-session-debug "font-base-size applied" main-font-size)))

  ;; 7. Set environment variables for correct macro dispatch and key handling
  ;;    Message buffer: prog-language/prog-session for output macro dispatch
  ;;    Input buffer: sidebar-input-mode for kbd-enter override detection
  (with-buffer (msg-buf)
    (init-env "prog-language" "llm")
    (init-env "prog-session" session-id))
  (with-buffer (in-buf)
    (init-env "sidebar-input-mode" "true"))
  (sidebar-session-debug "set env vars: msg-buf prog-language=llm, in-buf sidebar-input-mode=true")

  ;; 8. Find out/next nodes (need with-buffer for tree context)
  ;;    tree-search returns a LIST; use tree-find-first to get first match
  (define out-tree
    (with-buffer (msg-buf)
      (with t (tree-find-first (buffer-tree)
                               (lambda (n) (tree-is? n 'output)))
        (sidebar-session-debug "out-tree found" (if t (tree->stree t) #f))
        (and t (tree-ref t 0)))))

  ;; next-tree: #f (prompt updates not needed in sidebar - no input node)
  ;;    The standard session uses next for prompt display in input nodes,
  ;;    but sidebar uses a plain document for input, so prompt is N/A
  (define next-tree #f)
  (sidebar-session-debug "next-tree: #f (not used in sidebar)")

  (sidebar-session-debug "out-tree valid?" (if (tree? out-tree) #t #f))

  ;; 9. Enable text input for this session
  (session-enable-text-input "llm" session-id)
  (sidebar-session-debug "text input enabled")

  ;; 10. Start session feed with :start
  ;;     plugin-feed with :start triggers plugin-start if not running,
  ;;     or plugin-next if already running. No need for separate plugin-write.
  (when out-tree
    (sidebar-session-feed "llm" session-id :start out-tree next-tree '())
    (sidebar-session-debug "feed started"))
  (when (not out-tree)
    (sidebar-session-debug "ERROR: out-tree is #f, cannot start feed"))

  ;; 11. Register active session
  (set! sidebar-active-session (list "llm" session-id))
  (sidebar-session-debug "active session registered" sidebar-active-session)

  (sidebar-session-debug "=== make-sidebar-session END ==="))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Stop / Cancel
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (sidebar-session-stop)
  (:synopsis "Stop the active sidebar session and clean up")
  (when sidebar-active-session
    (with (lan ses) sidebar-active-session
      (sidebar-session-debug "stop" (list lan ses))
      (sidebar-session-debug "stop: connection-status" (connection-status lan ses))
      ;; Interrupt current evaluation if busy
      (if (== (connection-status lan ses) 3)
          (begin
            (sidebar-session-debug "stop: interrupting busy connection")
            (connection-interrupt lan ses)))
      ;; Stop the plugin connection
      (if (!= (connection-status lan ses) 0)
          (begin
            (sidebar-session-debug "stop: stopping connection")
            (connection-stop lan ses))
          (sidebar-session-debug "stop: connection already dead"))
      ;; Cancel any remaining pending entries
      (with l (pending-ref lan ses)
        (when (nnull? l)
          (sidebar-session-debug "stop: canceling pending entries" (length l))
          (plugin-cancel lan ses #f)))
      ;; Append end marker to message buffer's session inner document
      (with-buffer (msg-buf)
        (with sess (tree-find-first (buffer-tree)
                                    (lambda (n) (tree-is? n 'session)))
          (when (and sess (>= (tree-arity sess) 2))
            (with doc (tree-ref sess 2)
              (when (and doc (tm-func? doc 'document))
                (tree-insert! doc (tree-arity doc)
                  '((script-interrupted))))))))
      ;; Clear sidebar-input-mode env var so kbd-enter override stops matching
      (with-buffer (in-buf)
        (init-env "sidebar-input-mode" ""))
      ;; Clear state
      (set! sidebar-active-session #f)
      (sidebar-session-debug "stop: done"))))

(tm-define (sidebar-session-cancel)
  (:synopsis "Cancel current evaluation, keep session alive")
  (when sidebar-active-session
    (with (lan ses) sidebar-active-session
      (sidebar-session-debug "cancel-current" (list lan ses))
      (sidebar-session-debug "cancel-current: connection-status" (connection-status lan ses))
      (if (== (connection-status lan ses) 3)
          (begin
            (sidebar-session-debug "cancel-current: interrupting")
            (connection-interrupt lan ses))
          (sidebar-session-debug "cancel-current: not busy, nothing to cancel")))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Send
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Multi-turn send: each round creates a new output node and feeds through
;; session mechanism, so the pending queue and callbacks work correctly.
(tm-define (sidebar-session-send text)
  (:synopsis "Send user text to sidebar session via session-feed")
  (:argument text "User input text")

  (sidebar-session-debug "send" text)
  (when (and sidebar-active-session (!= text ""))
    (with (lan ses) sidebar-active-session
      (sidebar-session-debug "send: lan/ses" (list lan ses))
      (sidebar-session-debug "send: connection-status" (connection-status lan ses))

      ;; 1. Append user message and new output node to message buffer
      ;;    Insert into the session's inner document, not the top-level document
      (with-buffer (msg-buf)
        (let* ((sess (tree-find-first (buffer-tree)
                                      (lambda (n) (tree-is? n 'session))))
               (doc (and sess (>= (tree-arity sess) 2)
                         (tree-ref sess 2))))
          (when (and doc (tm-func? doc 'document))
            ;; Append user message with separator
            (tree-insert! doc (tree-arity doc)
              (list `(concat (with "font-series" "bold" "User: ") ,text)
                    '(document "")))
            ;; Append new output node for agent response
            (tree-insert! doc (tree-arity doc)
              '((output (document "")))))))
      (sidebar-session-debug "send: appended user message and output node")

      ;; 2. Get out reference (document inside the new output node)
      ;;    Find the last output node in the session's inner document
      (define out-tree
        (with-buffer (msg-buf)
          (let* ((sess (tree-find-first (buffer-tree)
                                        (lambda (n) (tree-is? n 'session))))
                 (doc (and sess (>= (tree-arity sess) 2)
                           (tree-ref sess 2)))
                 (out-node (and doc (tree-ref doc :last))))
            (sidebar-session-debug "send: last node" (if out-node (tree->stree out-node) #f))
            (and out-node (tree-is? out-node 'output)
                 (tree-ref out-node 0)))))
      (sidebar-session-debug "send: out-tree valid?" (if (tree? out-tree) #t #f))

      ;; 3. Clear input content
      (sidebar-session-clear-input!)

      ;; 4. next-tree is #f (prompt updates not needed in sidebar)
      (define next-tree #f)
      (sidebar-session-debug "send: next-tree is #f (not used in sidebar)")

      ;; 5. Feed through session mechanism
      ;;    This creates a pending entry, sets script-busy on out,
      ;;    and triggers plugin-do -> sidebar-session-do -> plugin-write
      (when out-tree
        (sidebar-session-feed lan ses `(document ,text) out-tree next-tree '()))
      (when (not out-tree)
        (sidebar-session-debug "send: ERROR - out-tree is #f")))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; C++ button entry points
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Override kbd-enter in sidebar input buffer
;; Uses get-env "sidebar-input-mode" for reliable detection instead of
;; URL comparison (which can fail due to URL normalization differences)
;; or field-input-context? (which requires input tags that cause output
;; to be routed to the input buffer via standard session flow)
(tm-define (kbd-enter t shift?)
  (:require (== (get-env "sidebar-input-mode") "true"))
  (sidebar-session-debug "kbd-enter: intercepted in sidebar input buffer")
  (if shift?
      (insert-return)
      (if (sidebar-session-active?)
          (chat-sidebar-send)
          (insert-return))))

;; Called by C++ Send button: (chat-sidebar-send)
(tm-define (chat-sidebar-send)
  (:synopsis "Send input text (called by C++ Send button)")
  (sidebar-session-debug "chat-sidebar-send called")
  (sidebar-session-debug "chat-sidebar-send: active?" (sidebar-session-active?))
  (when (sidebar-session-active?)
    (let ((text (sidebar-session-read-input)))
      (sidebar-session-debug "chat-sidebar-send: read input" text)
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
