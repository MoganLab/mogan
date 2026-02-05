
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : files.scm
;; DESCRIPTION : file handling
;; COPYRIGHT   : (C) 2001-2021  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs texmacs tm-files)
  (:use (texmacs texmacs tm-server)
        (texmacs texmacs tm-print)
        (utils library cursor)))

(import (only (liii string) string-contains))
(import (only (srfi srfi-1) find))
(import (only (srfi srfi-1) remove))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Remember last save/open directory 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define last-file-dialog-directory #f)

(tm-define (get-last-file-dialog-directory)
  "Get the last directory used in file dialog"
  (or last-file-dialog-directory
      (get-preference "last-file-dialog-directory")))

(tm-define (set-last-file-dialog-directory dir)
  "Set the last directory used in file dialog"
  (let ((u (system->url dir)))
    (when (and (string? dir) (url-exists? u) (url-directory? u)
               (not (url-descends? u (get-texmacs-path))))
      (set! last-file-dialog-directory dir)
      (set-preference "last-file-dialog-directory" dir))))

(tm-define (remember-file-dialog-directory name)
  "Remember the directory from a file operation"
  (when (url? name)
    (let ((dir (url->system (url-head name))))
      (set-last-file-dialog-directory dir))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Check whether the file name is valid (exclude *)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (url-contains-wildcard? u)
  (string-contains (url->system u) "*"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Supplementary routines on urls, taking into account the TeXmacs file system
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define cpp-url-last-modified url-last-modified)
(define cpp-url-newer? url-newer?)
(define cpp-buffer-last-save buffer-last-save)

(tm-define (url-last-modified u)
  (if (url-rooted-tmfs? u)
      (tmfs-date u)
      (cpp-url-last-modified u)))

(tm-define (url-newer? u1 u2)
  (if (or (url-rooted-tmfs? u1) (url-rooted-tmfs? u2))
      (and-let* ((d1 (url-last-modified u1))
                 (d2 (url-last-modified u2)))
        (> d1 d2))
      (cpp-url-newer? u1 u2)))

(tm-define (url-remove u)
  (if (url-rooted-tmfs? u)
      (tmfs-remove u)
      (system-remove u)))

(tm-define (url-autosave u suf)
  (if (url-rooted-tmfs? u)
      (tmfs-autosave u suf)
      (and (or (url-scratch? u)
               (url-test? u "fw")
               (not (url-exists? u))
           (url-glue u suf)))))

(tm-define (url-wrap u)
  (and (url-rooted-tmfs? u)
       (tmfs-wrap u)))

(tm-define (buffer-last-save u)
  (with base (url-wrap u)
    (cond ((not base)
           (cpp-buffer-last-save u))
          ((buffer-exists? base)
           (buffer-last-save base))
          (else (url-last-modified base)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Miscellaneous subroutines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (buffer-missing-style?)
  (with t (tree->stree (get-style-tree))
    (and (pair? t) (== (car t) 'tuple) (null? (cdr t)))))

(tm-define (buffer-set-default-style)
  (init-style "generic")
  (with lan (get-preference "language")
    (if (!= lan "english") (set-document-language lan)))
  (with psz (get-printer-paper-type)
    (if (!= psz "a4") (init-page-type psz)))
  (with type (get-preference "page medium")
    (if (!= type "papyrus") (init-env "page-medium" type)))
  (with type (get-preference "page screen margin")
    (if (!= type "true") (init-env "page-screen-margin" type)))
  (when (!= (get-preference "scripting language") "none")
    (lazy-plugin-force)
    (init-env "prog-scripts" (get-preference "scripting language")))
  (add-style-package "number-europe")
  (add-style-package "preview-ref")
  (when (== (get-preference "gui theme") "liii-night")
    (when (not (has-style-package? "dark"))
      (add-style-package "dark")))
  (buffer-pretend-saved (current-buffer)))

(tm-define (propose-name-buffer)
  (with name (url->unix (current-buffer))
    (cond ((not (url-scratch? name)) name)
          ((os-win32?) "")
          (else (string-append (var-eval-system "pwd") "/")))))

(tm-property (choose-file fun text type)
  (:interactive #t))

(tm-define (open-auxiliary aux body . opt-master)
  (let* ((name (aux-name aux))
         (master (if (null? opt-master) (buffer-master) (car opt-master))))
    (aux-set-document aux body)
    (aux-set-master aux master)
    (switch-document name)))

(define-public-macro (with-aux u . prg)
  `(let* ((u ,u)
          (t (tree-import u "texmacs"))
          (name (current-buffer))
          (aux "* Aux *"))
     (aux-set-document aux t)
     (aux-set-master aux u)
     (switch-to-buffer (aux-name aux))
     (with r (begin ,@prg)
       (switch-to-buffer name)
       r)))

(tm-define (buffer-copy buf u)
  (:synopsis "Creates a copy of @buf in @u and return @u")
  (with-buffer buf
    (let* ((styles (get-style-list))
           (init (get-all-inits))
           (refl (list-references))
           (refs (map get-reference refl))
           (body (tree-copy (buffer-get-body buf))))
      (view-new u) ; needed by buffer-focus, used in with-buffer
      (buffer-set-body u body) 
      (with-buffer u
        (set-style-list styles)
        (init-env "global-title" (buffer-get-metadata buf "title"))
        (init-env "global-author" (buffer-get-metadata buf "author"))
        (init-env "global-subject" (buffer-get-metadata buf "subject"))
        (for-each
         (lambda (t)
           (if (tree-func? t 'associate)
               (with (var val) (list (tree-ref t 0) (tree-ref t 1))
                 (init-env-tree (tree->string var) val))))
         (tree-children init))
        (for-each set-reference refl refs))
      u)))

(tm-define (buffer->windows-of-tabpage buf)
  (remove (lambda (vw) (url-none? vw)) (map view->window-of-tabpage (buffer->views buf))))

(tm-define (switch-to-buffer* buf)
  (let* ((wins (buffer->windows-of-tabpage buf))
         (win (if (member (current-window) wins)
                  (current-window)
                  (car wins)))
         (view (if (member (current-window) wins)
                   (find (lambda (vw)
                           (== (view->window-of-tabpage vw) win))
                         (buffer->views buf))
                   (car (buffer->views buf)))))
    (cond ((eq? buf (current-buffer)) (noop))
          ((nnull? (buffer->windows-of-tabpage buf))
           (switch-to-window win)
           (window-set-view win view #t))
          (else (switch-to-buffer buf)))))

(tm-define (switch-to-buffer-index index)
  (let* ((lst (buffer-menu-unsorted-list 99))
         (len (length lst)))
    (when (and (integer? index) (>= index 0) (< index len))
      (let ((buf (list-ref lst index)))
        (switch-to-buffer* buf)))))

(tm-define (switch-to-view-index index)
  (let* ((lst (tabpage-list #t)) ;; #t stands for current window
         (len (length lst)))
    (when (and (integer? index) (>= index 0) (< index len))
      (let* ((view (list-ref lst index))
             (view-win (view->window-of-tabpage view)))
        (window-set-view view-win view #t)))))

(tm-define (ensure-default-view)
  (:synopsis "Switch to parent window if not in default view")
  (if (not (is-view-type? (current-view) "default"))
    (switch-to-parent-window)))

(tm-define-macro (with-default-view . body)
  (:synopsis "Ensure we are in a default view, then execute @body")
  `(begin
     (ensure-default-view)
     (exec-delayed
       (lambda () ,@body))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Saving buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define current-save-source (url-none))
(tm-define current-save-target (url-none))

(define (buffer-notify-recent name)
  (learn-interactive 'recent-buffer (list (cons "0" (url->system name)))))

(define (has-faithful-format? name)
  (in? (url-suffix name) '("tm" "ts" "tp" "stm" "scm" "tmu")))

(define (save-buffer-post name opts)
  ;;(display* "save-buffer-post " name "\n")
  (cond ((in? :update opts)
         (update-buffer name))
        ((in? :commit opts)
         (commit-buffer name))))

(define (save-buffer-save name opts)
  ;;(display* "save-buffer-save " name "\n")
  (with vname `(verbatim ,(utf8->cork (url->system name)))
    (if (buffer-save name)
        (begin
          (buffer-pretend-modified name)
          (set-message `(concat "Could not save " ,vname) "Save file"))
        (begin
          (if (== (url-suffix name) "ts") (style-clear-cache))
          (autosave-remove name)
          (buffer-notify-recent name)
          ;; Remember directory for file dialog 
          (remember-file-dialog-directory name)
          (set-message `(concat "Saved " ,vname) "Save file")
          (save-buffer-post name opts)))))

(define (save-buffer-check-faithful name opts)
  ;;(display* "save-buffer-check-faithful " name "\n")
  (if (has-faithful-format? name)
      (save-buffer-save name opts)
      (user-confirm "Save requires data conversion. Really proceed?" #f
        (lambda (answ)
          (when answ
            (save-buffer-save name opts))))))

(tm-widget (readonly-file-dialog-widget cmd)
  (resize "500guipx" "200guipx"
    (centered
      (bold (text (translate "Read-only")))
      (glue #t #f 12 6)
      (text "The current document or its directory has read-only attributes.")
      (text "You can save the document using Save as."))
    (bottom-buttons
      >>
      ("Save as" (cmd "save_as"))
      ///
      ("Cancel" (cmd "cancel"))
      ///)))

(define (cannot-write? name action)
  (with vname `(verbatim ,(utf8->cork (url->system name)))
    (cond ((and (not (url-test? name "f")) (url-exists? name))
           (with msg "The file cannot be created:"
             (notify-now `(concat ,msg "<br>" ,vname)))
           #t)
          ((and (url-test? name "f") (not (url-test? name "w")))
           (dialogue-window
             readonly-file-dialog-widget
             (lambda (answer)
               (when (== answer "save_as")
                 (choose-file save-buffer-as "Save TeXmacs file" "action_save_as")))
             "Failed to save"))
          (else #f))))

(define (save-buffer-check-permissions name opts)
  ;;(display* "save-buffer-check-permissions " name "\n")
  (set! current-save-source name)
  (set! current-save-target name)
  (with vname `(verbatim ,(utf8->cork (url->system name)))
    (cond ((url-scratch? name)
           (choose-file
             (lambda (x) (apply save-buffer-as-main (cons x opts)))
             "Save TeXmacs file" "tmu"))
          ((not (buffer-exists? name))
           (with msg `(concat "The buffer " ,vname " does not exist")
             (set-message msg "Save file")))
          ((not (buffer-modified? name))
           (with msg "No changes need to be saved"
             (set-message msg "Save file"))
           (save-buffer-post name opts))
          ((cannot-write? name "Save file")
           (noop))
          ((and (url-test? name "fr")
                (and-with mod-t (url-last-modified name)
                  (and-with save-t (buffer-last-save name)
                    (> mod-t save-t))))
           (user-confirm "The file has changed on disk. Really save?" #f
             (lambda (answ)
               (when answ
                 (save-buffer-check-faithful name opts)))))
          (else (save-buffer-check-faithful name opts)))))

(tm-define (save-buffer-main . args)
  ;;(display* "save-buffer-main\n")
  (if (or (null? args) (not (url? (car args))))
      (save-buffer-check-permissions (current-buffer) args)
      (save-buffer-check-permissions (car args) (cdr args))))

(tm-define (save-buffer . l)
  (with-default-view (apply save-buffer-main l)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Saving buffers under a new name
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (save-buffer-as-save new-name name opts)
  ;;(display* "save-buffer-as-save " new-name ", " name "\n")
  (if (and (url-scratch? name) (url-exists? name)) (system-remove name))
  (buffer-rename name new-name)
  (buffer-pretend-modified new-name)
  (save-buffer-save new-name opts))

(define (save-buffer-as-check-faithful new-name name opts)
  ;;(display* "save-check-as-check-faithful " new-name ", " name "\n")
  (if (or (== (url-suffix new-name) (url-suffix name))
          (has-faithful-format? new-name))
      (save-buffer-as-save new-name name opts)
      (user-confirm "Save requires data conversion. Really proceed?" #f
        (lambda (answ)
          (when answ
            (save-buffer-as-save new-name name opts))))))

(define (save-buffer-as-check-other new-name name opts)
  ;;(display* "save-buffer-as-check-other " new-name ", " name "\n")
  (cond ((buffer-exists? new-name)
         (with s (string-append "The file " (url->system new-name)
                                " is being edited. Discard edits?")
           (user-confirm s #f
             (lambda (answ)
               (when answ (save-buffer-as-save new-name name opts))))))
        (else (save-buffer-as-save new-name name opts))))

(define (save-buffer-as-check-permissions new-name name opts)
  ;;(display* "save-buffer-as-check-permissions " new-name ", " name "\n")
  (cond ((cannot-write? new-name "Save file")
         (noop))
        ((and (url-test? new-name "f") (nin? :overwrite opts))
         (user-confirm "File already exists. Really overwrite?" #f
           (lambda (answ)
             (when answ (save-buffer-as-check-other new-name name opts)))))
        (else (save-buffer-as-check-other new-name name opts))))

(tm-define (save-buffer-as-main new-name . args)
  ;;(display* "save-buffer-as-main " new-name "\n")
  (if (or (null? args) (not (url? (car args))))
      (save-buffer-as-check-permissions new-name (current-buffer) args)
      (save-buffer-as-check-permissions new-name (car args) (cdr args))))

(tm-define (save-buffer-as new-name . args)
  (:argument new-name texmacs-file "Save as")
  (:default  new-name (propose-name-buffer))
  (with-default-view
    (when (string? new-name)
      (set! new-name (string-replace new-name ":" "-"))
      (set! new-name (string-replace new-name ";" "-")))
    (with opts (if (x-gui?) args (cons :overwrite args))
      (apply save-buffer-as-main (cons new-name opts)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Exporting buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (export-buffer-export name to fm opts)
  ;;(display* "export-buffer-export " name ", " to ", " fm "\n")
  (with vto `(verbatim ,(url->system to))
    (if (buffer-export name to fm)
        (set-message `(concat "Could not save " ,vto) "Export file")
        (set-message `(concat "Exported to " ,vto) "Export file"))))

(define (export-buffer-check-permissions name to fm opts)
  ;;(display* "export-buffer-check-permissions " name ", " to ", " fm "\n")
  (cond ((cannot-write? to "Export file")
         (noop))
        ((and (url-test? to "f") (nin? :overwrite opts))
         (user-confirm "File already exists. Really overwrite?" #f
           (lambda (answ)
             (when answ (export-buffer-export name to fm opts)))))
        (else (export-buffer-export name to fm opts))))

(tm-define (export-buffer-main name to fm opts)
  ;;(display* "export-buffer-main " name ", " to ", " fm "\n")
  (when (string? to)
    (set! to (string-replace to ":" "-"))
    (set! to (string-replace to ";" "-"))
    (set! to (url-relative (buffer-get-master name) to)))
  (if (url? name) (set! current-save-source name))
  (if (url? to) (set! current-save-target to))
  (export-buffer-check-permissions name to fm opts))

(tm-define (export-buffer to)
  (with fm (url-format to)
    (if (== fm "generic") (set! fm "verbatim"))
    (export-buffer-main (current-buffer) to fm (list :overwrite))))

(tm-define (buffer-exporter fm)
  (with opts (if (x-gui?) (list) (list :overwrite))
    (lambda (s) (export-buffer-main (current-buffer) s fm opts))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Autosave
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (more-recent file suffix1 suffix2)
  (and (url-exists? (url-glue file suffix1))
       (url-exists? (url-glue file suffix2))
       (url-newer? (url-glue file suffix1) (url-glue file suffix2))))

(define (most-recent-suffix file)
  (if (more-recent file "~" "")
      (if (not (more-recent file "#" "")) "~"
          (if (more-recent file "#" "~") "#" "~"))
      (if (more-recent file "#" "") "#" "")))

(define (autosave-eligible? name)
  (and (not (url-rooted-web? name))
       (or (not (url-rooted-tmfs? name))
           (tmfs-autosave name "~"))))

(define (autosave-propose name)
  (and (autosave-eligible? name)
       (with s (most-recent-suffix name)
         (and (!= s "")
              (url-glue name s)))))

(define (autosave-rescue? name) 
  (and (autosave-eligible? name)
       (== (most-recent-suffix name) "#")))

(define (autosave-remove name)
  (when (url-exists? (url-glue name "~"))
    (url-remove (url-glue name "~")))
  (when (url-exists? (url-glue name "#"))
    (url-remove (url-glue name "#"))))

(tm-define (autosave-buffer name)
  (when (and (buffer-modified-since-autosave? name)
             (url-autosave name "~"))
    (when (debug-get "io")
      (debug-message "debug-io" (string-append "Autosave " (url->system name) "\n")))
    ;; FIXME: incorrectly autosaves after cursor movements only
    (let* ((vname `(verbatim ,(utf8->cork (url->system name))))
           (suffix (if (rescue-mode?) "#" "~"))
           (aname (if (url-scratch? name) name (url-autosave name suffix)))
           (fm (url-format name)))
      (cond ((nin? fm (list "texmacs" "stm" "mgs" "tmu"))
             (when (not (rescue-mode?))
               (set-message `(concat "Warning: " ,vname " not auto-saved")
                            "Auto-save file")))
            ((buffer-export name aname fm)
             (when (not (rescue-mode?))
               (set-message `(concat "Failed to auto-save " ,vname)
                            "Auto-save file")))
            (else
             (when (not (rescue-mode?))
               (buffer-pretend-autosaved name)
               (set-temporary-message `(concat "Auto-saved " ,vname)
                                      "Auto-save file" 2500)))))))

(tm-define (autosave-all)
  (for-each autosave-buffer (buffer-list)))

(tm-define (autosave-now)
  (autosave-all)
  (autosave-delayed))

(tm-define (save-all-buffers)
  (for-each (lambda (buf)
              (when (buffer-modified? buf)
                (buffer-save buf)))
            (buffer-list)))

(tm-define (autosave-delayed)
  (let* ((pref (get-preference "autosave"))
         (len (if (and (string? pref) (integer? (string->number pref)))
                  (* (string->number pref) 1000)
                  120000)))
    (if (> len 0)
        (delayed
          (:pause len)
          (autosave-now)))))

(define (notify-autosave var val)
  (if (current-view) ; delayed-autosave would crash at initialization time
      (autosave-delayed)))

(define-preferences
  ("autosave" "120" notify-autosave))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Opening files using external tools
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (buffer-external? u)
  (or (url-rooted-web? u)
      (not (in? (url-root u) (list "tmfs" "file" "default" "blank" "ramdisc")))
      (file-of-format? u "image")
      (file-of-format? u "pdf")
      (file-of-format? u "postscript")
      (file-of-format? u "generic")))

(tm-define (load-external u)
  (when (not (url-rooted? u))
    (set! u (url-relative (current-buffer) u)))
  (open-url u))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Loading buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (load-buffer-open name opts)
  ;;(display* "load-buffer-open " name ", " opts "\n")
  (cond ((in? :background opts) (noop))
        ((in? :new-window opts)
         (open-buffer-in-window name (buffer-get name) ""))
        (else
         ;; Remember current buffer to check if it's an unmodified scratch buffer
         (let ((prev-buffer (current-buffer)))
           (with wins (buffer->windows-of-tabpage name)
             (if (and (!= wins '())
                      (in? (current-window) wins))
                 (switch-to-buffer* name)
                 (switch-to-buffer name)))
           ;; Close the previous unmodified scratch buffer after loading new file
           (when (and prev-buffer
                      (!= prev-buffer name)
                      (url-scratch? prev-buffer)
                      (not (buffer-modified? prev-buffer)))
             (cpp-buffer-close prev-buffer)))))
  (buffer-notify-recent name)
  ;; Remember directory for file dialog
  (remember-file-dialog-directory name)
  (when (nnull? (select (buffer-get name)
                        '(:* gpg-passphrase-encrypted-buffer)))
    (tm-gpg-dialogue-passphrase-decrypt-buffer name))
  (and-with master (and (url-rooted-tmfs? name) (tmfs-master name))
    (when (!= master name)
      (buffer-set-master name master)))
  (when (and (in-beamer?)
             (== (get-init-page-rendering) "book")
             (inside? 'slideshow)
             (> (nr-pages) 1))
    (delayed (:idle 25) (fit-to-screen-width)))
  (noop))

(define (load-buffer-load name opts)
  ;;(display* "load-buffer-load " name ", " opts "\n")
  (let* ((path (url->system name))
         (vname `(verbatim ,(utf8->cork path))))
    (cond ((buffer-exists? name)
           (load-buffer-open name opts))
          ((url-exists? name)
           (if (buffer-load name)
               (set-message `(concat "Could not load " ,vname) "Load file")
               (load-buffer-open name opts)))
          (else
            (with msg "The file or buffer does not exist:"
              (begin
                (debug-message "debug-io" (string-append msg "\n" path))
                (notify-now `(concat ,msg "<br>" ,vname))))))))

(define (load-buffer-check-permissions name opts)
  ;;(display* "load-buffer-check-permissions " name ", " opts "\n")
  (let* ((path (url->system name))
         (vname `(verbatim ,(utf8->cork path))))
    (cond ((and (not (url-test? name "f")) (url-exists? name))
           (with msg "The file cannot be loaded or created:"
             (begin
               (debug-message "debug-io" (string-append msg "\n" path))
               (notify-now `(concat ,msg "<br>" ,vname)))))
          ((and (url-test? name "f") (not (url-test? name "r")))
           (with msg `(concat ,(translate "You do not have read access to") " " ,vname)
             (show-message msg "Load file")))
          (else (load-buffer-load name opts)))))

(define (load-buffer-check-autosave name opts)
  ;;(display* "load-buffer-check-autosave " name ", " opts "\n")
  (if (and (autosave-propose name) (nin? :strict opts))
      (with question (if (autosave-rescue? name)
                         "Rescue file from crash?"
                         "Load more recent autosave file?")
        (user-confirm question #t
          (lambda (answ)
            (if answ
                (let* ((autosave-name (autosave-propose name))
                       (format (url-format name))
                       (doc (tree-import autosave-name format)))
                  (buffer-set name doc)
                  (load-buffer-open name opts)
                  (buffer-pretend-modified name))
                (load-buffer-check-permissions name opts)))))
      (load-buffer-check-permissions name opts)))

(tm-define (load-buffer-main name . opts)
  ;;(display* "load-buffer-main " name ", " opts "\n")
  (if (and (not (url-exists? name))
           (url-exists? (url-append "$TEXMACS_FILE_PATH" name)))
      (set! name (url-resolve (url-append "$TEXMACS_FILE_PATH" name) "f")))
  (if (not (url-rooted? name))
      (if (current-buffer)
          (set! name (url-relative (current-buffer) name))
          (set! name (url-append (url-pwd) name))))
  (load-buffer-check-autosave name opts))

;; The load flowgraph:
;; load-buffer
;; -> load-buffer-main
;;    -> load-buffer-check-autosave
;;       -> load-buffer-check-permission
;;           -> load-buffer-load
;;              -> load-buffer-open
;;       -> load-buffer-open
(tm-define (load-buffer name . opts)
  (:argument name smart-file "File name")
  (:default  name (propose-name-buffer))
  ;;(display* "load-buffer " name ", " opts "\n")
  (apply load-buffer-main (cons name opts)))

(tm-define (load-buffer-in-new-window name . opts)
  (:argument name smart-file "File name")
  (:default  name (propose-name-buffer))
  (if (buffer->window name)
      (noop) ;;(window-focus (buffer->window name))
      (apply load-buffer-main (cons name (cons :new-window opts)))))

(tm-define (load-browse-buffer name)
  (:synopsis "Load a buffer or switch to it if already open")
  (cond ((buffer-exists? name) (switch-to-buffer name))
        ((and (buffer-external? name)
         (!= (url-suffix name) "tm")
         (!= (url-suffix name) "tmu"))
         (load-external name))
        ((url-rooted-web? name)
         ;; Show wait dialog during remote file loading
         (system-wait "Loading remote file" (url->system name))
         (load-buffer name))
        ((url-rooted-web? (current-buffer)) (load-buffer name))
        (else (load-buffer name))))

(tm-define (open-buffer)
  (:synopsis "Open a new file")
  (with-default-view (choose-file load-buffer "Load file" "action_open")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Reverting buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (revert-buffer-revert . l)
  (with name (if (null? l) (current-buffer) (car l))
    (if (not (buffer-exists? name))
        (load-buffer name)
        (begin
          (when (!= name (current-buffer))
            (switch-to-buffer name))
          (url-cache-invalidate name)
          (with t (tree-import name (url-format name))
            (if (== t (tm->tree "error"))
                (set-message "Error: file not found" "Revert buffer")
                (buffer-set name t)))))))

(tm-define (revert-buffer . l)
  (with name (if (null? l) (current-buffer) (car l))
    (if (and (buffer-exists? name) (buffer-modified? name))
        (user-confirm "Buffer has been modified. Really revert?" #f
          (lambda (answ)
            (when answ (apply revert-buffer-revert l))))
        (apply revert-buffer-revert l))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Importing buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (import-buffer-import name fm opts)
  ;;(display* "import-buffer-import " name ", " fm "\n")
  (if (== fm (url-format name))
      (apply load-buffer-main (cons name opts))
      (let* ((s (url->tmfs-string name))
             (u (string-append "tmfs://import/" fm "/" s)))
        (apply load-buffer-main (cons u opts)))))

(define (import-buffer-check-permissions name fm opts)
  ;;(display* "import-buffer-check-permissions " name ", " fm "\n")
  (with vname `(verbatim ,(utf8->cork (url->system name)))
    (cond ((not (url-test? name "f"))
           (with msg `(concat "The file " ,vname " does not exist")
             (set-message msg "Import file")))
          ((not (url-test? name "r"))
           (with msg `(concat ,(translate "You do not have read access to") " " ,vname)
             (show-message msg "Import file")))
          (else (import-buffer-import name fm opts)))))

(tm-define (import-buffer-main name fm opts)
  ;;(display* "import-buffer-main " name ", " fm "\n")
  (if (and (not (url-exists? name))
           (url-exists? (url-append "$TEXMACS_FILE_PATH" name)))
      (set! name (url-resolve (url-append "$TEXMACS_FILE_PATH" name) "f")))
  (import-buffer-check-permissions name fm opts))

(tm-define (import-buffer name fm . opts)
  (if (window-per-buffer?)
      (import-buffer-main name fm (cons :new-window opts))
      (import-buffer-main name fm opts)))

(tm-define (buffer-importer fm)
  (lambda (s) (import-buffer s fm)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; System dependent conventions for buffer management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-in-window)
  (choose-file load-buffer-in-new-window "Load file" "action_open"))

(tm-define (open-document)
  (if (window-per-buffer?) (open-in-window) (open-buffer)))

(tm-define (open-document*)
  (if (window-per-buffer?) (open-buffer) (open-in-window)))

(tm-define (load-document u)
  (:argument u smart-file "File name")
  (:default  u (propose-name-buffer))
  (when (not (url-none? u))
    (if (window-per-buffer?) (load-buffer-in-new-window u) (load-buffer u))))

(tm-define (load-document* u)
  (:argument u smart-file "File name")
  (:default  u (propose-name-buffer))
  (when (not (url-none? u))
    (if (window-per-buffer?) (load-buffer u) (load-buffer-in-new-window u))))

(tm-define (switch-document u)
  (:argument u smart-file "File name")
  (:default  u (propose-name-buffer))
  (when (not (url-none? u))
    (if (window-per-buffer?)
        (if (buffer->window u)
            (noop) ;;(window-focus (buffer->window u))
            (open-buffer-in-window u (buffer-get u) ""))
        (load-buffer u))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Printing buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (interactive-page-setup)
  (:synopsis "Specify the page setup")
  (:interactive #t)
  (set-message "Not yet implemented" "Printer setup"))

(tm-define (direct-print-buffer)
  (:synopsis "Print the current buffer")
  (print))

(tm-define (interactive-print-buffer)
  (:synopsis "Print the current buffer")
  (:interactive #t)
  (print-to-file "$TEXMACS_HOME_PATH/system/tmp/tmpprint.ps")
  (interactive-print '() "$TEXMACS_HOME_PATH/system/tmp/tmpprint.ps"))

(tm-define (print-buffer)
  (:synopsis "Print the current buffer")
  (:interactive (use-print-dialog?))
  (if (use-print-dialog?)
      (interactive-print-buffer)
      (direct-print-buffer)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Important files to which the buffer is linked (e.g. bibliographies)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (linked-files-inside t)
  (cond ((tree-atomic? t) (list))
        ((tree-is? t 'document)
         (append-map linked-files-inside (tree-children t)))
        ((tree-in? t '(with with-bib))
         (linked-files-inside (tm-ref t :last)))
        ((or (tree-func? t 'bibliography 4)
             (tree-func? t 'bibliography* 5))
         (with name (tm->stree (tm-ref t 2))
           (if (or (== name "") (nstring? name)) (list)
               (with s (if (string-ends? name ".bib") name
                           (string-append name ".bib"))
                 (list (url-relative (current-buffer) s))))))
        (else (list))))

(tm-define (linked-file-list)
  (linked-files-inside (buffer-tree)))
