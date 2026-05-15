;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : double-buffer-session.scm
;; DESCRIPTION : double buffer session engine (adapted from session-edit)
;; COPYRIGHT   : (C) 2025--2026  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (dynamic double-buffer-session)
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

(define double-buffer-session-debug-log-file "/tmp/double-buffer-session-debug.log")
(define double-buffer-session-debug-counter 0)

(define (double-buffer-session-debug msg . args)
  (set! double-buffer-session-debug-counter (+ double-buffer-session-debug-counter 1))
  (let* ((prefix (string-append "[SESSION-" (number->string double-buffer-session-debug-counter) "] "))
         (log-msg (if (null? args)
                      (string-append prefix msg "\n")
                      (string-append prefix msg " => " (object->string args) "\n"))))
    (display log-msg)
    (catch #t
      (lambda ()
        (with-output-to-file double-buffer-session-debug-log-file
          (lambda () (display log-msg))
          'append))
      (lambda e (display "[WARN] Failed to write session log file\n")))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; State
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define double-buffer-active-session #f)
(define double-buffer-session-serial 0)

(tm-define (double-buffer-session-active?)
  (:synopsis "Check if a double buffer session is active")
  ;; nnull? returns #t for #f (because #f is not an empty list),
  ;; so we must use an explicit truthiness check
  (if double-buffer-active-session #t #f))

(tm-define (double-buffer-session-id)
  (:synopsis "Get current double buffer session id")
  (if double-buffer-active-session
      (cdr double-buffer-active-session)
      #f))

(tm-define (double-buffer-session-busy?)
  (:synopsis "Check if the double buffer session is busy processing")
  (and double-buffer-active-session
       (with (lan ses) double-buffer-active-session
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

(define (double-buffer-session-encode in out next opts)
  (list (list double-buffer-session-do
              double-buffer-session-notify
              double-buffer-session-next
              double-buffer-session-cancel)
        in
        (tree->tree-pointer out)
        (if next (tree->tree-pointer next) #f)
        opts))

(define (double-buffer-session-decode l)
  (list (second l)
        (tree-pointer->tree (third l))
        (if (fourth l) (tree-pointer->tree (fourth l)) #f)
        (fifth l)))

(define (double-buffer-session-detach l)
  (tree-pointer-detach (third l))
  (when (fourth l) (tree-pointer-detach (fourth l))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Helpers (adapted from session-edit.scm)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (var-tree-children t)
  (with r (tree-children t)
    (if (and (nnull? r) (tree-empty? (cAr r))) (cDr r) r)))

(define (double-buffer-session-flatten-stree x)
  (cond ((string? x) x)
        ((pair? x)
         (apply string-append
                (map double-buffer-session-flatten-stree (cdr x))))
        (else "")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Input helpers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Read text from the input buffer's plain document
(tm-define (double-buffer-session-read-input)
  (:synopsis "Read trimmed text from the input buffer's plain document")
  (with-buffer (in-buf)
    (let ((body (buffer-get-body (in-buf))))
      (double-buffer-session-debug "read-input: body type" (if (tree? body) 'tree #f))
      (if (tree? body)
          (string-trim-spaces
            (double-buffer-session-flatten-stree (tree->stree body)))
          ""))))

;; Clear the input buffer content
(define (double-buffer-session-clear-input!)
  (with-buffer (in-buf)
    (buffer-set-body (in-buf) '(document "")))
  (buffer-pretend-saved (in-buf))
  (double-buffer-session-debug "input cleared"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Output handlers (adapted from session-edit.scm)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (double-buffer-session-output t u)
  (when (tm-func? t 'document)
    (with i (tree-arity t)
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'script-busy))
          (set! i (- i 1)))
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'errput))
          (set! i (- i 1)))
      (when (tm-func? u 'document)
        (tree-insert! t i (var-tree-children u))
        (set-user-active #f)))))

(define (double-buffer-session-errput t u)
  (when (tm-func? t 'document)
    (with i (tree-arity t)
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'script-busy))
          (set! i (- i 1)))
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'errput))
          (set! i (- i 1)))
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'errput))
          (set! i (- i 1))
          (tree-insert! t i '((errput (document)))))
      (double-buffer-session-output (tree-ref t i 0) u))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Core callbacks (adapted from session-edit.scm, with with-buffer switching)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (double-buffer-session-do lan ses)
  (double-buffer-session-debug "do" (list lan ses))
  (with l (pending-ref lan ses)
    (double-buffer-session-debug "do: pending length" (length l))
    (when (nnull? l)
      (with (in out next opts) (double-buffer-session-decode (car l))
        (double-buffer-session-debug "do: in type" (if (tree? in) 'tree (if (symbol? in) in 'string)))
        (double-buffer-session-debug "do: in content" (if (string? in) in (if (symbol? in) in "<tree>")))
        (double-buffer-session-debug "do: connection-status" (connection-status lan ses))
        (if (or (tree-empty? in) (== in :start))
            (begin
              (double-buffer-session-debug "do: empty/start input, plugin-next")
              (plugin-next lan ses))
            (begin
              (double-buffer-session-debug "do: calling plugin-write" lan ses)
              (plugin-write lan ses in :session)
              (double-buffer-session-debug "do: plugin-write done")))))))

(define (double-buffer-session-next lan ses)
  (double-buffer-session-debug "next" (list lan ses))
  (with l (pending-ref lan ses)
    (when (nnull? l)
      (with (in out next opts) (double-buffer-session-decode (car l))
        (double-buffer-session-debug "next: in type" (if (symbol? in) in (if (string? in) in 'tree)))
        (with-buffer (msg-buf)
          (when (and (tm-func? out 'document)
                     (tm-func? (tree-ref out :last) 'script-busy))
            (let* ((dt (plugin-timing lan ses))
                   (ts (if (< dt 1000)
                           (string-append (number->string dt) " msec")
                           (string-append (number->string (/ dt 1000.0)) " sec"))))
              (double-buffer-session-debug "next: removing script-busy, timing" ts)
              (if (and (in? :timings opts) (>= dt 1))
                  (tree-set (tree-ref out :last) `(timing ,ts))
                  (tree-remove! out (- (tree-arity out) 1) 1)))))
        (double-buffer-session-detach (car l))
        (double-buffer-session-debug "next: done")))))

(define (double-buffer-session-notify lan ses ch t)
  (double-buffer-session-debug "notify" (list lan ses ch))
  (double-buffer-session-debug "notify: data" (tree->stree t))
  (with l (pending-ref lan ses)
    (if (null? l)
        (double-buffer-session-debug "notify: WARNING - pending queue is empty, dropping notification")
        (with (in out next opts) (double-buffer-session-decode (car l))
          (cond ((== ch "output")
                 (double-buffer-session-debug "notify: handling output")
                 (with-buffer (msg-buf)
                   (double-buffer-session-output out t)))
                ((== ch "error")
                 (double-buffer-session-debug "notify: handling error")
                 (with-buffer (msg-buf)
                   (double-buffer-session-errput out t)))
                ((== ch "prompt")
                 ;; Prompt updates not needed for sidebar (no input node)
                 (double-buffer-session-debug "notify: received prompt (ignored in sidebar mode)")))))))

(define (double-buffer-session-cancel lan ses dead?)
  (double-buffer-session-debug "cancel" (list lan ses dead?))
  (with l (pending-ref lan ses)
    (when (nnull? l)
      (with (in out next opts) (double-buffer-session-decode (car l))
        (with-buffer (msg-buf)
          (when (and (tm-func? out 'document)
                     (tm-func? (tree-ref out :last) 'script-busy))
            (tree-assign (tree-ref out :last)
                         (if dead? '(script-dead) '(script-interrupted)))))
        (double-buffer-session-detach (car l))
        (double-buffer-session-debug "cancel: done")))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Feed (adapted from session-feed, with with-buffer)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (double-buffer-session-feed lan ses in out next opts)
  (double-buffer-session-debug "feed" (list lan ses (if (symbol? in) in (if (string? in) in 'tree)) opts))
  (double-buffer-session-debug "feed: out tree valid?" (if (tree? out) #t #f))
  (double-buffer-session-debug "feed: next tree" (if next "present" "not used"))
  (double-buffer-session-debug "feed: connection-status before" (connection-status lan ses))
  (set! in (plugin-preprocess lan ses in opts))
  (with-buffer (msg-buf)
    (tree-assign! out '(document (script-busy))))
  (double-buffer-session-debug "feed: assigned script-busy to out")
  (with x (double-buffer-session-encode in out next opts)
    (double-buffer-session-debug "feed: calling plugin-feed")
    (apply plugin-feed `(,lan ,ses ,@(car x) ,(cdr x))))
  (double-buffer-session-debug "feed: plugin-feed returned")
  (double-buffer-session-debug "feed: connection-status after" (connection-status lan ses)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Session creation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (double-buffer-session-gen-id model)
  (set! double-buffer-session-serial (+ double-buffer-session-serial 1))
  ;; Format: "Model:sidebar:timestamp-serial"
  ;; connection-info extracts text before first colon as variant name,
  ;; so "Model:sidebar:..." -> variant = "Model" -> correct launcher
  (string-append model ":sidebar:"
                 (number->string (texmacs-time)) "-"
                 (number->string double-buffer-session-serial)))

(tm-define (make-double-buffer-session model)
  (:synopsis "Create a new double buffer session and open sidebar")
  (:argument model "Model name")

  (double-buffer-session-debug "=== make-double-buffer-session START ===" model)

  ;; 1. Stop any existing session
  (when double-buffer-active-session
    (double-buffer-session-debug "stopping existing session")
    (double-buffer-session-stop))

  ;; 2. Open sidebar first — this triggers C++ to create buffer widgets
  ;;    and bind them to the message/input buffer names
  (open-chat-sidebar)
  (double-buffer-session-debug "sidebar opened")

  ;; 3. Ensure buffers exist (created by C++ widget or manually)
  (chat-sidebar-ensure-buffer! (msg-buf))
  (chat-sidebar-ensure-buffer! (in-buf))
  (double-buffer-session-debug "buffers ensured"
    (list (buffer-exists? (msg-buf)) (buffer-exists? (in-buf))))

  ;; 4. Set up message buffer structure:
  ;;    (document (session "llm" session-id (document (output (document "")))))
  ;;    The session tag wraps output/input to set prog-language/prog-session
  ;;    for correct macro dispatch (llm-output, llm-input, etc.)
  (define session-id (double-buffer-session-gen-id model))
  (double-buffer-session-debug "session-id" session-id)
  (buffer-set-body (msg-buf)
                   `(document (session "llm" ,session-id
                                   (document (output (document ""))))))
  (buffer-pretend-saved (msg-buf))
  (double-buffer-session-debug "message buffer set up")

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
  (double-buffer-session-debug "input buffer set up (plain document)")

  ;; 6. Load style packages for proper macro rendering
  ;;    Set base style to "generic" (like make-session-in-sidebar does),
  ;;    then add main document packages and session-specific packages
  (let ((main-packs (with-buffer (current-buffer) (get-style-list))))
    (double-buffer-session-debug "main document style packages" main-packs)
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
        (double-buffer-session-debug "combined style packages" unique-packs)
        (with-buffer (msg-buf) (set-style-list unique-packs))
        (with-buffer (in-buf) (set-style-list unique-packs)))))
  ;; Add llm package for LLM-specific overrides
  (if (url-exists? (url-unix "$TEXMACS_STYLE_PATH" "llm.ts"))
      (begin
        (with-buffer (msg-buf) (add-style-package "llm"))
        (with-buffer (in-buf) (add-style-package "llm"))
        (double-buffer-session-debug "loaded llm package"))
      (double-buffer-session-debug "llm.ts not found, skipping"))

  ;; 6b. Match font size from main document for consistent appearance
  ;;     The "generic" base style may use a different default font size
  ;;     than the main document's style (e.g., "article")
  (let ((main-font-size (with-buffer (current-buffer) (get-env "font-base-size"))))
    (double-buffer-session-debug "main document font-base-size" main-font-size)
    (when (and main-font-size (string? main-font-size) (!= main-font-size ""))
      (with-buffer (msg-buf) (init-env "font-base-size" main-font-size))
      (with-buffer (in-buf) (init-env "font-base-size" main-font-size))
      (double-buffer-session-debug "font-base-size applied" main-font-size)))

  ;; 7. Set environment variables for correct macro dispatch and key handling
  ;;    Message buffer: prog-language/prog-session for output macro dispatch
  ;;    Input buffer: sidebar-input-mode for kbd-enter override detection
  (with-buffer (msg-buf)
    (init-env "prog-language" "llm")
    (init-env "prog-session" session-id))
  (with-buffer (in-buf)
    (init-env "sidebar-input-mode" "true"))
  (double-buffer-session-debug "set env vars: msg-buf prog-language=llm, in-buf sidebar-input-mode=true")

  ;; 8. Find out/next nodes (need with-buffer for tree context)
  ;;    tree-search returns a LIST; use tree-find-first to get first match
  (define out-tree
    (with-buffer (msg-buf)
      (with t (tree-find-first (buffer-tree)
                               (lambda (n) (tree-is? n 'output)))
        (double-buffer-session-debug "out-tree found" (if t (tree->stree t) #f))
        (and t (tree-ref t 0)))))

  ;; next-tree: #f (prompt updates not needed in sidebar - no input node)
  ;;    The standard session uses next for prompt display in input nodes,
  ;;    but sidebar uses a plain document for input, so prompt is N/A
  (define next-tree #f)
  (double-buffer-session-debug "next-tree: #f (not used in sidebar)")

  (double-buffer-session-debug "out-tree valid?" (if (tree? out-tree) #t #f))

  ;; 9. Enable text input for this session
  (session-enable-text-input "llm" session-id)
  (double-buffer-session-debug "text input enabled")

  ;; 10. Start session feed with :start
  ;;     plugin-feed with :start triggers plugin-start if not running,
  ;;     or plugin-next if already running. No need for separate plugin-write.
  (when out-tree
    (double-buffer-session-feed "llm" session-id :start out-tree next-tree '())
    (double-buffer-session-debug "feed started"))
  (when (not out-tree)
    (double-buffer-session-debug "ERROR: out-tree is #f, cannot start feed"))

  ;; 11. Register active session
  (set! double-buffer-active-session (list "llm" session-id))
  (double-buffer-session-debug "active session registered" double-buffer-active-session)

  (double-buffer-session-debug "=== make-double-buffer-session END ==="))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Stop / Cancel
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (double-buffer-session-stop)
  (:synopsis "Stop the active double buffer session and clean up")
  (when double-buffer-active-session
    (with (lan ses) double-buffer-active-session
      (double-buffer-session-debug "stop" (list lan ses))
      (double-buffer-session-debug "stop: connection-status" (connection-status lan ses))
      ;; Interrupt current evaluation if busy
      (if (== (connection-status lan ses) 3)
          (begin
            (double-buffer-session-debug "stop: interrupting busy connection")
            (connection-interrupt lan ses)))
      ;; Stop the plugin connection
      (if (!= (connection-status lan ses) 0)
          (begin
            (double-buffer-session-debug "stop: stopping connection")
            (connection-stop lan ses))
          (double-buffer-session-debug "stop: connection already dead"))
      ;; Cancel any remaining pending entries
      (with l (pending-ref lan ses)
        (when (nnull? l)
          (double-buffer-session-debug "stop: canceling pending entries" (length l))
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
      (set! double-buffer-active-session #f)
      (double-buffer-session-debug "stop: done"))))

(tm-define (double-buffer-session-cancel)
  (:synopsis "Cancel current evaluation, keep session alive")
  (when double-buffer-active-session
    (with (lan ses) double-buffer-active-session
      (double-buffer-session-debug "cancel-current" (list lan ses))
      (double-buffer-session-debug "cancel-current: connection-status" (connection-status lan ses))
      (if (== (connection-status lan ses) 3)
          (begin
            (double-buffer-session-debug "cancel-current: interrupting")
            (connection-interrupt lan ses))
          (double-buffer-session-debug "cancel-current: not busy, nothing to cancel")))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Send
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Multi-turn send: each round creates a new output node and feeds through
;; session mechanism, so the pending queue and callbacks work correctly.
(tm-define (double-buffer-session-send text)
  (:synopsis "Send user text to double buffer session via session-feed")
  (:argument text "User input text")

  (double-buffer-session-debug "send" text)
  (when (and double-buffer-active-session (!= text ""))
    (with (lan ses) double-buffer-active-session
      (double-buffer-session-debug "send: lan/ses" (list lan ses))
      (double-buffer-session-debug "send: connection-status" (connection-status lan ses))

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
      (double-buffer-session-debug "send: appended user message and output node")

      ;; 2. Get out reference (document inside the new output node)
      ;;    Find the last output node in the session's inner document
      (define out-tree
        (with-buffer (msg-buf)
          (let* ((sess (tree-find-first (buffer-tree)
                                        (lambda (n) (tree-is? n 'session))))
                 (doc (and sess (>= (tree-arity sess) 2)
                           (tree-ref sess 2)))
                 (out-node (and doc (tree-ref doc :last))))
            (double-buffer-session-debug "send: last node" (if out-node (tree->stree out-node) #f))
            (and out-node (tree-is? out-node 'output)
                 (tree-ref out-node 0)))))
      (double-buffer-session-debug "send: out-tree valid?" (if (tree? out-tree) #t #f))

      ;; 3. Clear input content
      (double-buffer-session-clear-input!)

      ;; 4. next-tree is #f (prompt updates not needed in sidebar)
      (define next-tree #f)
      (double-buffer-session-debug "send: next-tree is #f (not used in sidebar)")

      ;; 5. Feed through session mechanism
      ;;    This creates a pending entry, sets script-busy on out,
      ;;    and triggers plugin-do -> double-buffer-session-do -> plugin-write
      (when out-tree
        (double-buffer-session-feed lan ses `(document ,text) out-tree next-tree '()))
      (when (not out-tree)
        (double-buffer-session-debug "send: ERROR - out-tree is #f")))))

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
  (double-buffer-session-debug "kbd-enter: intercepted in sidebar input buffer")
  (if shift?
      (insert-return)
      (if (double-buffer-session-active?)
          (chat-sidebar-send)
          (insert-return))))

;; Called by C++ Send button: (chat-sidebar-send)
(tm-define (chat-sidebar-send)
  (:synopsis "Send input text (called by C++ Send button)")
  (double-buffer-session-debug "chat-sidebar-send called")
  (double-buffer-session-debug "chat-sidebar-send: active?" (double-buffer-session-active?))
  (when (double-buffer-session-active?)
    (let ((text (double-buffer-session-read-input)))
      (double-buffer-session-debug "chat-sidebar-send: read input" text)
      (when (!= text "")
        (double-buffer-session-send text)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Menu integration
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-double-buffer-session model)
  (:synopsis "Open double buffer session")
  (:interactive #t)
  (:argument model "Model")
  (make-double-buffer-session model))

(tm-define (close-double-buffer-session)
  (:synopsis "Close double buffer session")
  (:interactive #t)
  (double-buffer-session-stop)
  (close-chat-sidebar))

(tm-define (toggle-double-buffer-session model)
  (:synopsis "Toggle double buffer session")
  (:interactive #t)
  (:argument model "Model")
  (if (chat-sidebar-visible?)
      (close-double-buffer-session)
      (open-double-buffer-session model)))
