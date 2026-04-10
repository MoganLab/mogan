
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
  (:use (kernel texmacs tm-dialogue))
  (:use (utils library cursor)))

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
  ;; Get list of recent document paths from global recent-file state
  (learned-interactive 'recent-buffer))

(tm-define (startup-tab-add-recent-doc path)
  ;; Add or refresh a document in global recent-file state
  (learn-interactive 'recent-buffer (list (cons "0" path))))

(tm-define (startup-tab-clear-recent-doc path)
  ;; Remove a specific document from global recent-file state
  (recent-files-remove-by-path path))

(tm-define (startup-tab-clear-all-recent)
  ;; Clear all recent documents
  (noop))
