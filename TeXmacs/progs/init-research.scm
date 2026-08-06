
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-research.scm
;; DESCRIPTION : This is the standard TeXmacs initialization file (S7)
;; COPYRIGHT   : (C) 1999-2020  Joris van der Hoeven & Massimiliano Gubinelli
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;; S7 macros are not usual macros...

(define define-macro define-expansion)

(define primitive-symbol? symbol?)
(set! symbol? (lambda (s) (and (not (keyword? s)) (primitive-symbol? s))))

;; S7 loads by default in rootlet and eval in curlet
;; but we prefer to load and eval into *texmacs-user-module*
;; (the current toplevel)
;; FIXME: we have to clarify the situation with *current-module* when evaluating
;; in a different environment. In Guile *current-module* is set/reset.

(varlet (rootlet) '*current-module* (curlet))

;; 启动期 telemetry pending 队列：telemetry 实现已迁至 (plugin telemetry-*)
;; 插件，由事件循环 ~3s 后懒加载。插件加载前的 C++ 上报（OPEN/LOGIN 等）
;; 由 telemetry-track-or-enqueue 入队，(plugin init-telemetry) 加载时
;; 通过 telemetry-drain-pending! 一次性补 track。这组符号注入 rootlet，
;; 不依赖任何模块，C++ 可直接 call。
(let ((rl (rootlet)))
  (varlet rl 'telemetry-pending-events '())
  (varlet rl
    'telemetry-track-or-enqueue
    (lambda (event-type properties)
      (if (defined? 'track-event)
        (track-event event-type properties)
        (varlet rl
          'telemetry-pending-events
          (cons (cons event-type properties) telemetry-pending-events)
        ) ;varlet
      ) ;if
    ) ;lambda
  ) ;varlet
  (varlet rl
    'telemetry-drain-pending!
    (lambda ()
      (let ((pending telemetry-pending-events))
        (varlet rl 'telemetry-pending-events '())
        (for-each (lambda (e) (track-event (car e) (cdr e))) (reverse pending))
      ) ;let
    ) ;lambda
  ) ;varlet
) ;let
(let ()
  (define primitive-load load)
  (define primitive-eval eval)
  (define primitive-catch catch)

  (varlet (rootlet) 'tm-eval (lambda (obj) (eval obj *texmacs-user-module*)))
  (set! load
    (lambda (file . env)
      (primitive-load file (if (null? env) *current-module* (car env)))
    ) ;lambda
  ) ;set!
  (set! eval
    (lambda (obj . env)
      (let ((res (primitive-eval obj (if (null? env) *current-module* (car env)))))
        ;; (format #t "Eval: ~A -> ~A\n" obj res)
        res
      ) ;let
    ) ;lambda
  ) ;set!

  (set! catch
    (lambda (key cl hdl)
      (primitive-catch key
        cl
        (lambda args
          (apply hdl (car args) "[not-implemented]" (caadr args) (list (cdadr args)))
        ) ;lambda
      ) ;primitive-catch
    ) ;lambda
  ) ;set!
) ;let


(define developer-mode? #f)

(define boot-start (texmacs-time))

(define remote-client-list (list))

(debug-message "debug-std" "Booting TeXmacs kernel functionality\n")
(load (url-concretize "$TEXMACS_PATH/progs/kernel/boot/boot-s7.scm"))

(inherit-modules (kernel boot compat-s7)
  (kernel boot abbrevs)
  (kernel boot debug)
  (kernel boot srfi)
  (kernel boot ahash-table)
  (kernel boot prologue)
) ;inherit-modules
(inherit-modules (kernel library base)
  (kernel library list)
  (kernel library tree)
  (kernel library content)
  (kernel library patch)
) ;inherit-modules
(inherit-modules (kernel regexp regexp-match) (kernel regexp regexp-select))
(inherit-modules (kernel logic logic-rules)
  (kernel logic logic-query)
  (kernel logic logic-data)
) ;inherit-modules
(inherit-modules (kernel texmacs tm-define)
  (kernel texmacs tm-preferences)
  (kernel texmacs tm-modes)
  (kernel texmacs tm-plugins)
  (kernel texmacs tm-secure)
  (kernel texmacs tm-convert)
  (kernel texmacs tm-dialogue)
  (kernel texmacs tm-language)
  (kernel texmacs tm-file-system)
  (kernel texmacs tm-states)
) ;inherit-modules
(inherit-modules (kernel gui gui-markup)
  (kernel gui menu-define)
  (kernel gui menu-widget)
  (kernel gui kbd-define)
  (kernel gui kbd-handlers)
  (kernel gui menu-test)
  (kernel old-gui old-gui-widget)
  (kernel old-gui old-gui-factory)
  (kernel old-gui old-gui-form)
  (kernel old-gui old-gui-test)
) ;inherit-modules

;; (display "Booting utilities\n")
(use-modules (utils library cpp-wrap))
(lazy-define (utils library cursor) notify-cursor-moved)
(lazy-define (utils edit variants) make-inline-tag-list make-wrapped-tag-list)
(lazy-define (utils cas cas-out) cas->stree)
(lazy-define (utils plugins plugin-cmd) pre-serialize utf8raw-serialize)
(lazy-define (utils test test-convert)
  delayed-quit
  build-manual
  build-ref-suite
  run-test-suite
) ;lazy-define
(use-modules (utils library smart-table))
(use-modules (utils plugins plugin-convert))
(use-modules (utils misc markup-funcs))
(use-modules (utils handwriting handwriting))
(lazy-tmfs-handler (utils automate auto-tmfs) automate)
(lazy-define (utils automate auto-tmfs) auto-load-help)
(lazy-define (utils misc gui-keyboard) get-keyboard)

;; (display "Booting BibTeX style modules\n")
(use-modules (bibtex bib-utils))
(lazy-define (bibtex bib-complete) current-bib-file citekey-completions)
(lazy-menu (bibtex bib-widgets) open-bibliography-inserter)

;; (display "Booting main TeXmacs functionality\n")
(use-modules (texmacs texmacs tm-server) (texmacs texmacs tm-view))

(define tm-files-boot-start (texmacs-time))
(use-modules (texmacs texmacs tm-files))
(debug-message "debug-std"
  (string-append "bench tm-files: "
    (number->string (- (texmacs-time) tm-files-boot-start))
    " ms\n"
  ) ;string-append
) ;debug-message
(use-modules (texmacs texmacs tm-print))
(use-modules (texmacs keyboard config-kbd))
(lazy-menu (texmacs menus file-menu)
  file-menu
  go-menu
  new-file-menu
  load-menu
  save-menu
  print-menu
  print-menu-inline
  close-menu
) ;lazy-menu
(lazy-menu (texmacs menus edit-menu) edit-menu)
(lazy-menu (texmacs menus view-menu) view-menu texmacs-bottom-toolbars)
(lazy-menu (texmacs menus tools-menu) tools-menu)
(lazy-menu (texmacs menus preferences-menu) preferences-menu page-setup-menu)
(lazy-menu (texmacs menus preferences-widgets) open-preferences)
(use-modules (texmacs menus main-menu))
(use-modules (texmacs menus notificationbar))
(use-modules (texmacs menus tabpage-menu))
(use-modules (startup-tab startup-tab))
(use-modules (text text-outline))
(lazy-define (texmacs menus file-menu) recent-file-list recent-directory-list)
(lazy-define (texmacs menus view-menu) set-bottom-bar test-bottom-bar?)
(tm-define (notify-set-attachment name key val) (noop))

;; (display "Booting generic mode\n")
(lazy-menu (generic generic-menu) focus-menu texmacs-focus-icons)
(lazy-menu (generic format-menu)
  format-menu
  font-size-menu
  color-menu
  horizontal-space-menu
  transform-menu
  specific-menu
  text-font-effects-menu
  text-effects-menu
  vertical-space-menu
  indentation-menu
  line-break-menu
  page-header-menu
  page-footer-menu
  page-numbering-menu
  page-break-menu
) ;lazy-menu
(lazy-menu (generic document-menu)
  document-menu
  project-menu
  document-style-menu
  global-language-menu
) ;lazy-menu
(lazy-menu (generic document-part)
  preamble-menu
  document-part-menu
  project-manage-menu
) ;lazy-menu
(lazy-menu (generic insert-menu)
  insert-menu
  texmacs-insert-menu
  texmacs-insert-icons
  insert-link-menu
  insert-image-menu
) ;lazy-menu
(lazy-define (generic document-edit)
  update-document
  get-init-page-rendering
  init-page-rendering
) ;lazy-define
(lazy-define (generic generic-edit) notify-activated notify-disactivated)
(lazy-define (generic generic-doc) focus-help)
(lazy-define (generic search-widgets)
  search-toolbar
  replace-toolbar
  open-search
  toolbar-search-start
  interactive-search
  open-replace
  toolbar-replace-start
  interactive-replace
  search-next-match
) ;lazy-define
(lazy-define (generic spell-widgets)
  spell-toolbar
  open-spell
  toolbar-spell-start
  interactive-spell
) ;lazy-define
(lazy-define (generic format-widgets) open-paragraph-format open-page-format)
(lazy-define (generic pattern-selector)
  open-pattern-selector
  open-gradient-selector
  open-background-picture-selector
) ;lazy-define
(lazy-define (generic document-widgets)
  open-source-tree-preferences
  open-document-paragraph-format
  open-document-page-format
  open-document-metadata
  open-document-colors
  open-page-headers-footers
  open-document-page-number
) ;lazy-define
(tm-property (open-search) (:interactive #t))
(tm-property (open-replace) (:interactive #t))
(tm-property (open-paragraph-format) (:interactive #t))
(tm-property (open-page-format)
  (:interactive #t)
  (:applicable (not (selection-active?)))
) ;tm-property
(tm-property (open-source-tree-preferences) (:interactive #t))
(tm-property (open-document-paragraph-format) (:interactive #t))
(tm-property (open-document-page-format) (:interactive #t))
(tm-property (open-document-metadata) (:interactive #t))
(tm-property (open-document-colors) (:interactive #t))
(tm-property (open-page-headers-footers) (:interactive #t))
(tm-property (open-document-page-number) (:interactive #t))
(tm-property (open-pattern-selector cmd w) (:interactive #t))
(tm-property (open-gradient-selector cmd) (:interactive #t))
(tm-property (open-background-picture-selector cmd) (:interactive #t))

;; (display "Booting text mode\n")
(lazy-menu (text text-menu)
  text-format-menu
  text-format-icons
  text-menu
  text-block-menu
  text-inline-menu
  text-icons
  text-block-icons
  text-inline-icons
) ;lazy-menu

;; (display "Booting math mode\n")
(lazy-menu (math math-menu)
  math-format-menu
  math-format-icons
  math-menu
  math-insert-menu
  math-icons
  math-insert-icons
  math-correct-menu
  semantic-math-preferences-menu
  context-preferences-menu
  insert-math-menu
) ;lazy-menu
(lazy-initialize (math math-menu) (in-math?))
(lazy-define (math math-edit) brackets-refresh)

;; (display "Booting programming modes\n")
(lazy-menu (prog prog-menu)
  prog-format-menu
  prog-format-icons
  prog-menu
  prog-icons
) ;lazy-menu

;; (display "Booting source mode\n")
(lazy-menu (source source-menu)
  source-macros-menu
  source-menu
  source-icons
  source-transformational-menu
  source-executable-menu
) ;lazy-menu
(lazy-define (source macro-edit)
  has-macro-source?
  edit-macro-source
  edit-focus-macro-source
) ;lazy-define
(lazy-menu (source macro-menu) insert-macro-menu)
(lazy-define (source macro-widgets)
  editable-macro?
  open-macros-editor
  open-macro-editor
  create-table-macro
  edit-focus-macro
  edit-previous-macro
) ;lazy-define
(lazy-define (source shortcut-edit) init-user-shortcuts has-user-shortcut?)
(lazy-define (source shortcut-widgets) open-shortcuts-editor)
(tm-property (open-macro-editor l mode) (:interactive #t))
(tm-property (create-table-macro l mode) (:interactive #t))
(tm-property (open-macros-editor mode) (:interactive #t))
(tm-property (edit-focus-macro) (:interactive #t))
(tm-property (open-shortcuts-editor . opt) (:interactive #t))
(when (url-exists? "")
  (delayed (:idle 100) (init-user-shortcuts))
) ;when

;; (display "Booting table mode\n")
(lazy-menu (table table-menu) insert-table-menu)
(lazy-define (table table-edit) table-resize-notify)
(lazy-define (table table-widgets) open-cell-properties open-table-properties)
(tm-property (open-cell-properties) (:interactive #t))
(tm-property (open-table-properties) (:interactive #t))

;; (display "Booting graphics mode\n")
(lazy-menu (graphics graphics-menu) graphics-menu graphics-icons)
(lazy-define (graphics graphics-object)
  graphics-init-state
  graphics-decorations-update
) ;lazy-define
(lazy-define (graphics graphics-utils) make-graphics)
(lazy-define (graphics graphics-edit)
  graphics-busy?
  graphics-reset-context
  graphics-undo-enabled
  graphics-release-left
  graphics-release-middle
  graphics-release-right
  graphics-start-drag-left
  graphics-dragging-left
  graphics-end-drag-left
) ;lazy-define
(lazy-define (graphics graphics-main)
  graphics-update-proviso
  graphics-get-proviso
  graphics-set-proviso
) ;lazy-define
(lazy-define (graphics graphics-markup)
  arrow-with-text
  arrow-with-text*
  circle
  three-points-circle
  std-arc
  std-arc-counterclockwise
  three-points-arc
  sector
  sector-counterclockwise
) ;lazy-define
(define-secure-symbols arrow-with-text
  arrow-with-text*
  circle
  three-points-circle
  std-arc
  std-arc-counterclockwise
  three-points-arc
  sector
  sector-counterclockwise
) ;define-secure-symbols

;; (display "Booting formal and natural languages\n")
(lazy-language (language minimal) minimal)
(lazy-language (language std-math) std-math)
(lazy-define (language natural) replace)

;; (display "Booting educational features\n")
(lazy-menu (education edu-menu) edu-insert-menu)

;; (display "Booting dynamic features\n")
(lazy-menu (dynamic fold-menu)
  insert-fold-menu
  dynamic-menu
  dynamic-icons
  graphics-overlays-menu
  graphics-screens-menu
  graphics-focus-overlays-menu
  graphics-focus-overlays-icons
) ;lazy-menu
(lazy-menu (dynamic session-menu) insert-session-menu session-help-icons)
(lazy-menu (dynamic scripts-menu)
  scripts-eval-menu
  scripts-plot-menu
  plugin-eval-menu
  plugin-eval-toggle-menu
  plugin-plot-menu
) ;lazy-menu
(lazy-menu (dynamic calc-menu)
  calc-table-menu
  calc-insert-menu
  calc-icourse-menu
) ;lazy-menu
(lazy-menu (dynamic animate-menu) insert-animation-menu animate-toolbar)
(lazy-define (dynamic fold-edit)
  screens-switch-to
  dynamic-make-slides
  overlays-context?
) ;lazy-define
(lazy-define (dynamic session-edit) scheme-eval)
(lazy-define (dynamic calc-edit) calc-ready? calc-table-renumber)
(lazy-define (dynamic scripts-plot) open-plots-editor)
(lazy-initialize (dynamic session-menu) (in-session?))

;; (display "Booting documentation\n")
(lazy-menu (doc tmdoc-menu) tmdoc-menu tmdoc-icons)
(lazy-menu (doc help-menu) help-menu)
(lazy-define (doc tmdoc)
  tmdoc-expand-help
  tmdoc-expand-help-manual
  tmdoc-expand-this
  tmdoc-include
) ;lazy-define
(use-modules (doc docgrep))
(lazy-define (doc tmdoc-search)
  tmdoc-search-style
  tmdoc-search-tag
  tmdoc-search-parameter
  tmdoc-search-scheme
) ;lazy-define
(lazy-define (doc tmweb)
  youtube-select
  tmweb-convert-dir
  tmweb-update-dir
  tmweb-convert-dir-keep-texmacs
  tmweb-update-dir-keep-texmacs
  tmweb-interactive-build
  tmweb-interactive-update
) ;lazy-define
(lazy-define (doc apidoc) apidoc-all-modules apidoc-all-symbols)
(lazy-menu (doc apidoc-menu) apidoc-menu)
(lazy-tmfs-handler (doc docgrep) grep)
(lazy-tmfs-handler (doc tmdoc) help)
(lazy-tmfs-handler (doc apidoc) apidoc)
(define-secure-symbols tmdoc-include youtube-select)

;; (display "Booting converters\n")

(lazy-format (data code) cpp scheme)
(lazy-format (data csv) csv)

(lazy-format (data image) postscript pdf svg gif jpeg png ppm tif webp xpm)

(lazy-format (convert rewrite init-rewrite) texmacs verbatim)
(lazy-format (data stm) stm)
(lazy-format (data stem) stem)
(lazy-format (data tmu) tmu)
(lazy-format (data docx) docx)
(lazy-format (data html) html)
(lazy-define (convert images tmimage)
  export-selection-as-graphics
  clipboard-copy-image
) ;lazy-define
(lazy-define (convert rewrite init-rewrite)
  texmacs->code
  texmacs->verbatim
  texmacs->utf8raw
  utf8raw->texmacs
) ;lazy-define
(lazy-define (convert html tmhtml) ext-tmhtml-eqnarray*)
(define-secure-symbols ext-tmhtml-eqnarray*)
(lazy-define (convert html tmhtml-expand) tmhtml-env-patch)
(lazy-define (convert latex latex-drd) latex-arity latex-type)
(lazy-define (convert latex tmtex) tmtex-env-patch)
(lazy-define (convert latex latex-tools)
  latex-set-virtual-packages
  latex-has-style?
  latex-has-package?
  latex-has-texmacs-style?
  latex-has-texmacs-package?
) ;lazy-define

;; (display "Booting partial document facilities\n")
(lazy-define (part part-shared) buffer-initialize buffer-notify)
(lazy-menu (part part-menu) document-master-menu)
(lazy-tmfs-handler (part part-tmfs) part)

;; (display "Booting database facilities\n")
(lazy-define (database db-widget) open-db-chooser)
(lazy-define (database db-menu) db-show-toolbar)
(lazy-define (database db-convert) db-url?)
(lazy-define (database bib-db) zealous-bib-import zealous-bib-export)
(lazy-define (database bib-manage)
  bib-import-bibtex
  bib-compile
  bib-attach
  open-bib-chooser
) ;lazy-define
(lazy-define (database bib-local) open-biblio)
(lazy-menu (database db-menu) db-menu db-toolbar)
(lazy-tmfs-handler (database db-tmfs) db)
(tm-property (open-biblio) (:interactive #t))


;; (display "Booting linking facilities\n")
(lazy-menu (link link-menu) link-menu)
(lazy-define (link link-edit) create-unique-id)
(lazy-define (link link-navigate)
  link-active-upwards
  link-active-ids
  link-follow-ids
) ;lazy-define
(lazy-define (link link-extern)
  get-constellation
  get-link-locations
  register-link-locations
) ;lazy-define
(lazy-menu (link ref-menu) ref-menu)
(lazy-define (link ref-edit) preview-reference)
(define-secure-symbols preview-reference)

;; (display "Booting versioning facilities\n")
(lazy-menu (version version-menu) version-menu)
(lazy-define (version version-tmfs) update-buffer commit-buffer)

;; (display "Booting debugging and developer facilities\n")
(lazy-menu (debug debug-menu) debug-menu)
(lazy-menu (texmacs menus developer-menu)
  developer-menu
  custom-keyboard-toolbar
) ;lazy-menu
(lazy-define (debug debug-widgets)
  notify-debug-message
  open-debug-console
  open-error-messages
) ;lazy-define

;; (display "Booting editing modes for various special styles\n")
(lazy-menu (various poster-menu) poster-block-menu)
(lazy-menu (various theme-menu) basic-theme-menu)
(lazy-define (various theme-edit) current-basic-theme)
(lazy-define (various theme-menu) basic-theme-name)
;; (display* "time: " (- (texmacs-time) boot-start) "\n")
;; (display* "memory: " (texmacs-memory) " bytes\n")

;; (display "Booting plugins\n")
(for-each lazy-plugin-initialize (plugin-list))
;; (display* "time: " (- (texmacs-time) boot-start) "\n")
;; (display* "memory: " (texmacs-memory) " bytes\n")

;; (display "Booting fonts\n")
(use-modules (fonts fonts-ec)
  (fonts fonts-adobe)
  (fonts fonts-x)
  (fonts fonts-math)
  (fonts fonts-foreign)
  (fonts fonts-misc)
  (fonts fonts-composite)
  (fonts fonts-truetype)
) ;use-modules
(lazy-define (fonts font-old-menu) text-font-menu math-font-menu prog-font-menu)
(lazy-define (fonts font-new-widgets)
  open-font-selector
  open-document-font-selector
  open-document-other-font-selector
) ;lazy-define
(tm-property (open-font-selector) (:interactive #t))
(tm-property (open-document-font-selector) (:interactive #t))
;; (display* "time: " (- (texmacs-time) boot-start) "\n")
;; (display* "memory: " (texmacs-memory) " bytes\n")

;; (display "Booting regression testing\n")
;; (display* "time: " (- (texmacs-time) boot-start) "\n")
;; (display* "memory: " (texmacs-memory) " bytes\n")

;; (display "Booting autoupdater\n")
(when (use-plugin-updater?)
  (use-modules (utils misc updater))
  (delayed (:idle 2000) (updater-initialize))
) ;when
(debug-message "debug-std"
  (string-append "time: "
    (number->string (- (texmacs-time) boot-start))
    "\n"
    "memory: "
    (number->string (texmacs-memory))
    " bytes\n"
    "------------------------------------------------------\n"
  ) ;string-append
) ;debug-message
(texmacs-banner)
(debug-message "debug-std" "Initialization done\n")
