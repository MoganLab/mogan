
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MODULE     : startup-tab-file.scm
;; DESCRIPTION: Scheme bindings for startup tab file operations
;; COPYRIGHT  : (C) 2026 Yuki Lu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (startup-tab startup-tab-file)
  (:use (texmacs texmacs tm-server))
  (:use (texmacs texmacs tm-files))
  (:use (texmacs menus file-menu))
  (:use (kernel texmacs tm-dialogue))
  (:use (utils library cursor)))

;; Debug mode predicate
(define (in-debug?) (with-debugging-tool?))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document creation with specific style
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (new-document-with-style style-id)
  ;; Create a new document with the specified style
  ;; style-id: "generic", "beamer", "book", "exam", "letter", "article"
  ;; Use with-buffer to ensure we're working in the correct buffer context
  (with-default-view
    (let ((buf (if (window-per-buffer?) (open-window) (new-buffer))))
      ;; Schedule style initialization after buffer is fully set up
      (delayed
        (:idle 100)
        (with-buffer buf
          (init-style style-id))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; File operations wrappers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (startup-tab-file-open)
  ;; Open file dialog wrapper
  (open-document))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Recent documents management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (startup-tab-get-recent-docs)
  ;; Get recent document paths with the same filtering and ordering
  ;; as File -> Recent used
  (let* ((raw (string->number (get-preference "startup-tab:max-recent")))
         (nr (if (number? raw) raw 10))
         (nr (max 1 nr)))
    (map url->system (recent-file-list nr))))

(tm-define (startup-tab-add-recent-doc path)
  ;; Add or refresh a document in global recent-file state
  (learn-interactive 'recent-buffer (list (cons "0" path))))

(tm-define (startup-tab-clear-recent-doc path)
  ;; Remove a specific document from global recent-file state
  (recent-files-remove-by-path path))

(tm-define (startup-tab-clear-all-recent)
  ;; Clear all recent documents
  (noop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preload modules for faster file operations
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (startup-tab-preload-modules)
  ;; Preload modules that will be needed when opening files
  ;; This is called in the background when startup tab is shown
  ;; to reduce latency when user clicks to open a file
  ;;
  ;; Preload order: critical modules first, then nice-to-have
  ;; Each step is wrapped with error handling to ensure one failure
  ;; doesn't prevent subsequent modules from loading

  (define (safe-preload name thunk)
    (catch #t
      thunk
      (lambda (err)
        (debug-message "startup-tab" (string-append "Preload warning [" name "]: "
                                     (object->string err) "\n"))
        #f)))

  (define (preload-step step-name thunk)
    (safe-preload step-name thunk))

  ;; 1. Force all lazy plugin initializations (critical)
  ;; This loads all plugins that are normally loaded on-demand
  (preload-step "plugins" lazy-plugin-force)

  ;; 2. Preload format converters (critical)
  ;; These are needed when loading different document formats
  ;; lazy-format-force loads all pending format modules
  (preload-step "formats" lazy-format-force)

  ;; 3. Preload language support for common document languages
  ;; These are needed for syntax highlighting and language-specific features
  (preload-step "lang-minimal" (lambda () (lazy-language-force "minimal")))
  (preload-step "lang-std-math" (lambda () (lazy-language-force "std-math")))

  ;; 4. Preload keyboard handlers (nice-to-have, runs in background)
  ;; This ensures keyboard shortcuts work immediately after file open
  (preload-step "keyboard" (lambda () (lazy-keyboard-force #f)))

  ;; 5. Preload font database (reduces UI rendering latency)
  ;; This loads cached font information to avoid delays during first paint
  (preload-step "font-db" (lambda () (font-database-load)))

  ;; Debug: log completion (only in debug mode)
  (when (in-debug?)
    (debug-message "startup-tab" "Startup tab: modules preloaded\n")))
