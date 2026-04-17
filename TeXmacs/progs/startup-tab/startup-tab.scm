
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : startup-tab.scm
;; DESCRIPTION : startup tab helpers
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (startup-tab startup-tab)
  (:use (texmacs texmacs tm-server))
  (:use (texmacs texmacs tm-files))
  (:use (startup-tab startup-tab-file)))

(define startup-tab-preload-done? #f)
(define startup-tab-preload-running? #f)

;; Debug mode predicate
(define (in-debug?) (with-debugging-tool?))

(tm-define (startup-tab-enabled?)
  (startup-tab-preload-modules)
  #t)

(tm-define (startup-tab-default-entry)
  "file")

(tm-define (startup-tab-preload-modules)
  ;; Preload modules that will be needed when opening files.
  ;; Keep this orchestration in startup-tab.scm and run it only once.
  (cond
    (startup-tab-preload-done? #t)
    (startup-tab-preload-running? #f)
    (else
      (define (safe-preload name thunk)
        (catch #t
          thunk
          (lambda (err)
            (debug-message "startup-tab" (string-append "Preload warning ["
                                         name "]: " (object->string err) "\n"))
            #f)))

      (define (preload-step step-name thunk)
        (safe-preload step-name thunk))

      (set! startup-tab-preload-running? #t)
      (catch #t
        (lambda ()
          ;; 1. Force all lazy plugin initializations (critical)
          (preload-step "plugins" lazy-plugin-force)

          ;; 2. Preload format converters (critical)
          (preload-step "formats" lazy-format-force)

          ;; 3. Preload language support for common document languages
          (preload-step "lang-minimal" (lambda () (lazy-language-force "minimal")))
          (preload-step "lang-std-math" (lambda () (lazy-language-force "std-math")))

          ;; 4. Preload keyboard handlers (nice-to-have)
          (preload-step "keyboard" (lambda () (lazy-keyboard-force #f)))

          ;; 5. Preload font database to reduce first-paint/font fallback latency
          (preload-step "font-db" (lambda () (font-database-load)))

          (set! startup-tab-preload-done? #t)
          (when (in-debug?)
            (debug-message "startup-tab" "Startup tab: modules preloaded\n"))
          #t)
        (lambda (_)
          (set! startup-tab-preload-done? #f)
          #f))
      (set! startup-tab-preload-running? #f))))
