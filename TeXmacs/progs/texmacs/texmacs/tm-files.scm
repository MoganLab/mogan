
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
        (kernel texmacs tm-convert)
        (kernel library content)
        (language locale)
        (utils library cursor)))

(import (only (liii string) string-contains))
(import (only (liii hashlib) md5))
(import (only (liii uuid) uuid4))
(import (only (liii path)
              path->string path-dir? path-exists? path-from-env
              path-from-string path-getsize path-join path-name path-parent
              path-rename path-root path-stem path-unlink))
(import (only (liii os) mkdir))
(import (liii njson))
(import (only (srfi srfi-1) find))
(import (only (srfi srfi-1) remove))
(import (only (srfi srfi-19)
              TIME-UTC current-date current-time date->string time-second))

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

(define (sync-buffer-dark-style-with-gui-theme . opt-buf)
  (with buf (if (null? opt-buf) (current-buffer) (car opt-buf))
    (with-buffer buf
      (if (== (get-preference "gui theme") "liii-night")
          (when (not (has-style-package? "dark"))
            (add-style-package "dark"))
          (when (has-style-package? "dark")
            (remove-style-package "dark"))))))

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
  (sync-buffer-dark-style-with-gui-theme (current-buffer))
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
  (remove (lambda (vw) (or (not vw) (url-none? vw)))
          (map view->window-of-tabpage (buffer->views buf))))

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

;; save-buffer-save
;; 保存指定 buffer，并在保存前确保自动备份使用的稳定 doc id 已绑定。
;;
;; 语法
;; ----
;; (save-buffer-save name opts)
;;
;; 参数
;; ----
;; name : url
;; 要保存的 buffer 名称。
;;
;; opts : list
;; 保存后的附加动作，例如 :update 或 :commit。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 通过消息栏和 buffer 状态体现保存结果。
;;
;; 逻辑
;; ----
;; 先用 init-env 补齐缺失的 stem-doc-id，再执行原有 buffer-save 流程；
;; 保存成功后清理旧 autosave 文件、记录最近文件并执行后续动作。
;;
;; 注意
;; ----
;; doc id 只在用户明确保存时随文档持久化；打开已有文件时不会静默
;; 写回源文件。
(define (save-buffer-save name opts . kind*)
  ;;(display* "save-buffer-save " name "\n")
  (let ((kind (if (null? kind*) "save" (car kind*))))
  (with vname `(verbatim ,(utf8->cork (url->system name)))
    (auto-backup-ensure-buffer-doc-id! name)
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
          (auto-backup-buffer name kind)
          (save-buffer-post name opts))))))

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

;; save-buffer-check-permissions
;; 保存前检查目标 buffer 是否存在、是否可写以及是否需要用户确认。
;;
;; 语法
;; ----
;; (save-buffer-check-permissions name opts)
;;
;; 参数
;; ----
;; name : url
;; 要保存的 buffer 名称。
;;
;; opts : list
;; 保存后的附加动作，例如 :update 或 :commit。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 根据检查结果继续保存、弹出提示或结束流程。
;;
;; 逻辑
;; ----
;; 保留原有权限和磁盘更新时间检查；若 buffer 本身没有修改，但缺少
;; stem-doc-id，则在通过写权限检查后走正常保存链路，把 doc id 随本次
;; 用户保存写入文档。
;;
;; 注意
;; ----
;; 这个分支只响应用户主动保存，不会因为自动备份发现缺少 doc id 而
;; 立刻静默写回已有文件。
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
          ((and (not (buffer-modified? name))
                (auto-backup-buffer-needs-doc-id? name))
           (if (cannot-write? name "Save file")
               (noop)
               (begin
                 (auto-backup-ensure-buffer-doc-id! name)
                 (save-buffer-check-faithful name opts))))
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
  (save-buffer-save new-name opts "save-as"))

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
        (begin
          (set-message `(concat "Exported to " ,vto) "Export file")
          (when (== fm "pdf")
            (auto-backup-buffer name "export-pdf"))))))

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

(define autosave-fixed-interval-ms 120000)
(define auto-backup-fixed-interval-ms 120000)
(define auto-backup-retention-count 7)
(define auto-backup-record-retention-days 30)
(define auto-backup-day-seconds 86400)

(tm-define (autosave-enabled?)
  (!= (get-preference "autosave") "0"))

(tm-define (auto-backup-enabled?)
  (!= (get-preference "autobackup") "off"))

(tm-define (liiistem-version)
  (xmacs-version))

(define auto-backup-scheduled? #f)

(define (auto-backup-log msg)
  (debug-message "debug-io" (string-append "Auto-backup " msg "\n")))

(define (auto-backup-now-seconds)
  (time-second (current-time TIME-UTC)))

(define (auto-backup-home-path)
  (path-from-env "TEXMACS_HOME_PATH"))

(define (auto-backup-url-stree-tag x)
  (cond ((symbol? x) (symbol->string x))
        ((string? x) x)
        (else "")))

(define (auto-backup-url-stree-ref t n)
  (list-ref t n))

(define (auto-backup-url-stree->path t)
  (cond ((string? t) t)
        ((symbol? t) (symbol->string t))
        ((pair? t)
         (let ((tag (auto-backup-url-stree-tag (car t))))
           (cond ((== tag "concat")
                  (let ((left (auto-backup-url-stree->path
                               (auto-backup-url-stree-ref t 1)))
                        (right (auto-backup-url-stree->path
                                (auto-backup-url-stree-ref t 2))))
                    (cond ((== left "") right)
                          ((== right "") left)
                          (else (path-join left right)))))
                 ((== tag "root")
                  (let ((root (auto-backup-url-stree-tag
                               (auto-backup-url-stree-ref t 1))))
                    (if (in? root '("default" "file" "blank"))
                        (path->string (path-root))
                        "")))
                 ((== tag "none") "untitled")
                 ((== tag "or")
                  (auto-backup-url-stree->path
                   (auto-backup-url-stree-ref t 1)))
                 (else tag))))
        (else "untitled")))

(define (auto-backup-buffer-path name)
  (catch #t
    (lambda ()
      (path->string
       (path-from-string
        (auto-backup-url-stree->path (url->stree name)))))
    (lambda args "untitled")))

(define (auto-backup-trim-trailing-separators s)
  (let loop ((n (string-length s)))
    (if (and (> n 1)
             (let ((c (string-ref s (- n 1))))
               (or (char=? c #\/) (char=? c #\\))))
        (loop (- n 1))
        (substring s 0 n))))

(define (auto-backup-path-normal-string path)
  (auto-backup-trim-trailing-separators (path->string path)))

(define (auto-backup-path-descends? child parent)
  (catch #t
    (lambda ()
      (let ((parent-path (auto-backup-path-normal-string
                          (path-from-string parent))))
        (let loop ((path (path-from-string child)))
          (let ((current-path (auto-backup-path-normal-string path)))
            (or (== current-path parent-path)
                (let* ((next (path-parent path))
                       (next-path (auto-backup-path-normal-string next)))
                  (and (!= next-path current-path)
                       (loop next))))))))
    (lambda args #f)))

;; auto-backup-texmacs-path-buffer?
;; 判断 buffer 是否位于 get-texmacs-path 返回的目录或其子目录中。
;;
;; 语法
;; ----
;; (auto-backup-texmacs-path-buffer? name)
;;
;; 参数
;; ----
;; name : url
;; 待检查的 buffer 名称。
;;
;; 返回值
;; ----
;; boolean
;; #t 表示 buffer 对应路径位于 get-texmacs-path 下。
;;
;; 逻辑
;; ----
;; 将 buffer url 转成系统路径，再使用 (liii path) 的 path-parent 逐级
;; 向上检查是否能到达 get-texmacs-path。
;;
;; 注意
;; ----
;; TeXmacs 安装路径下的文件被视为只读内置资源，不进入自动备份。
(tm-define (auto-backup-texmacs-path-buffer? name)
  (catch #t
    (lambda ()
      (auto-backup-path-descends?
       (auto-backup-buffer-path name)
       (url->system (get-texmacs-path))))
    (lambda args #f)))

(define (auto-backup-path->url p)
  (system->url (path->string p)))

(tm-define (auto-backup-dir)
  (path-join (auto-backup-home-path) "system" "backup"))

(tm-define (auto-backup-manifest-path)
  (path-join (auto-backup-dir) "auto-backup.json"))

(tm-define (auto-backup-ensure-dir!)
  (catch #t
    (lambda ()
      (let ((system-dir (path-join (auto-backup-home-path) "system"))
            (backup-dir (auto-backup-dir)))
        (when (not (path-exists? system-dir))
          (mkdir system-dir))
        (when (not (path-exists? backup-dir))
          (mkdir backup-dir))
        (and (path-exists? backup-dir) (path-dir? backup-dir))))
    (lambda args
      (auto-backup-log "failed-to-create-backup-dir")
      #f)))

;; auto-backup-empty-manifest
;; 创建新的自动备份 manifest。
;;
;; 语法
;; ----
;; (auto-backup-empty-manifest)
;;
;; 参数
;; ----
;; 无。
;;
;; 返回值
;; ----
;; njson
;; 包含 meta 和 documents 的 manifest 对象，调用者负责释放。
;;
;; 逻辑
;; ----
;; 初始化版本号、固定调度间隔、单文档 rolling 数量和 manifest 记录最长
;; 保留天数。
;;
;; 注意
;; ----
;; manifest 的 njson 句柄需要用 let-njson 包裹，避免泄漏。
(tm-define (auto-backup-empty-manifest)
  (let ((manifest (string->njson
                   "{\"meta\":{\"version\":1,\"interval_seconds\":120,\"retention\":7,\"max_record_age_days\":30,\"updated_at\":0},\"documents\":{}}")))
    (njson-set! manifest "meta" "updated_at" (auto-backup-now-seconds))
    manifest))

(define (auto-backup-manifest-valid? manifest)
  (catch #t
    (lambda ()
      (and (njson-object? manifest)
           (let-njson ((meta (njson-ref manifest "meta"))
                       (documents (njson-ref manifest "documents")))
             (and (njson-object? meta)
                  (njson-object? documents)
                  (== (njson-ref meta "version") 1)))))
    (lambda args #f)))

(define (auto-backup-mark-broken! path)
  (catch #t
    (lambda ()
      (when (path-exists? path)
        (let ((broken (string-append
                       path
                       (string-append ".broken."
                                      (number->string
                                       (auto-backup-now-seconds))))))
          (path-rename path broken)
          (auto-backup-log
           (string-append "manifest-broken moved-to "
                          broken)))))
    (lambda args
      (auto-backup-log "manifest-broken move-failed"))))

(tm-define (auto-backup-load-manifest)
  (let ((path (auto-backup-manifest-path)))
    (if (not (path-exists? path))
        (auto-backup-empty-manifest)
        (catch #t
          (lambda ()
            (let ((manifest (file->njson path)))
              (if (auto-backup-manifest-valid? manifest)
                  manifest
                  (begin
                    (njson-free manifest)
                    (auto-backup-mark-broken! path)
                    (auto-backup-empty-manifest)))))
          (lambda args
            (auto-backup-mark-broken! path)
            (auto-backup-empty-manifest))))))

;; auto-backup-save-manifest!
;; 将自动备份 manifest 原子化写回磁盘。
;;
;; 语法
;; ----
;; (auto-backup-save-manifest! manifest)
;;
;; 参数
;; ----
;; manifest : njson
;; 待写回的 manifest 对象。
;;
;; 返回值
;; ----
;; boolean
;; #t 表示写入成功，#f 表示失败。
;;
;; 逻辑
;; ----
;; 写入前先清理 30 天以上的 manifest 记录，再更新 meta 并通过临时文件
;; 替换正式文件。
;;
;; 注意
;; ----
;; 清理旧记录时会同步删除对应的过期备份文件。
(tm-define (auto-backup-save-manifest! manifest)
  (let* ((path (auto-backup-manifest-path))
         (tmp (string-append
               path
               (string-append ".tmp."
                              (number->string
                               (auto-backup-now-seconds))))))
    (catch #t
      (lambda ()
        (auto-backup-clean-stale-documents! manifest)
        (njson-set! manifest "meta" "interval_seconds" 120)
        (njson-set! manifest "meta" "retention" auto-backup-retention-count)
        (njson-set! manifest "meta" "max_record_age_days"
                    auto-backup-record-retention-days)
        (njson-set! manifest "meta" "updated_at" (auto-backup-now-seconds))
        (njson->file tmp manifest)
        (when (path-exists? path) (path-unlink path))
        (path-rename tmp path)
        #t)
      (lambda args
        (auto-backup-log
         (string-append "manifest-save-failed "
                        path))
        (when (path-exists? tmp) (path-unlink tmp))
        #f))))

(define (auto-backup-format name)
  (if (url-scratch? name) "texmacs" (url-format name)))

;; auto-backup-buffer-eligible?
;; 判断指定 buffer 是否允许进入自动备份。
;;
;; 语法
;; ----
;; (auto-backup-buffer-eligible? name)
;;
;; 参数
;; ----
;; name : url
;; 待检查的 buffer 名称。
;;
;; 返回值
;; ----
;; boolean
;; #t 表示允许自动备份，#f 表示跳过。
;;
;; 逻辑
;; ----
;; 只允许本地、非 tmfs、非 web 且格式为 texmacs/stm/mgs/tmu 的文档备份；
;; 位于 get-texmacs-path 目录或子目录下的内置只读文件直接跳过。
;;
;; 注意
;; ----
;; 这个判断也会影响 doc id 绑定，跳过的只读资源不会被写入 stem-doc-id。
(tm-define (auto-backup-buffer-eligible? name)
  (and (url? name)
       (buffer-exists? name)
       (not (url-rooted-web? name))
       (not (url-rooted-tmfs? name))
       (not (auto-backup-texmacs-path-buffer? name))
       (in? (auto-backup-format name) '("texmacs" "stm" "mgs" "tmu"))))

(define (auto-backup-buffer-last-visited* name)
  (catch #t
    (lambda () (buffer-last-visited name))
    (lambda args 0)))

(define (auto-backup-recent-buffers buffers)
  (list-sort buffers
             (lambda (a b)
               (> (auto-backup-buffer-last-visited* a)
                  (auto-backup-buffer-last-visited* b)))))

(tm-define (auto-backup-manual-target)
  (let ((name (current-buffer)))
    (cond ((auto-backup-buffer-eligible? name) name)
          (else
           (let* ((eligible (auto-backup-recent-buffers
                             (list-filter (buffer-list)
                                          auto-backup-buffer-eligible?)))
                  (modified (list-filter eligible buffer-modified?))
                  (candidates (if (pair? modified) modified eligible)))
             (if (pair? candidates)
                 (begin
                   (auto-backup-log
                    (string-append "manual-target "
                                   (auto-backup-buffer-path
                                    (car candidates))))
                   (car candidates))
                 (begin
                   (auto-backup-log "manual-target none")
                   #f)))))))

(define (auto-backup-safe-char? c)
  (let ((i (char->integer c)))
    (or (and (>= i (char->integer #\a)) (<= i (char->integer #\z)))
        (and (>= i (char->integer #\A)) (<= i (char->integer #\Z)))
        (and (>= i (char->integer #\0)) (<= i (char->integer #\9)))
        (in? c '(#\- #\_ #\.))
        ;; Support UTF-8 multibyte characters (including Chinese)
        ;; UTF-8 continuation bytes: 0x80-0xBF
        ;; UTF-8 leading bytes: 0xC0-0xFD
        (>= i 128))))

(define (auto-backup-sanitize-name name)
  (let* ((chars (string->list name))
         (safe (map (lambda (c) (if (auto-backup-safe-char? c) c #\_))
                    chars))
         (result (list->string safe)))
    (if (== result "") "untitled" result)))

(define (auto-backup-doc-short-id doc-id)
  (if (>= (string-length doc-id) 8) (string-take doc-id 8) doc-id))

(tm-define (auto-backup-safe-base name doc-id)
  (let* ((path (auto-backup-buffer-path name))
         (raw (path-stem path))
         (base (if (or (url-scratch? name) (== raw "") (== raw "."))
                   (string-append "untitled_"
                                  (auto-backup-doc-short-id doc-id))
                   raw)))
    (auto-backup-sanitize-name base)))

(tm-define (auto-backup-timestamp)
  (catch #t
    (lambda () (date->string (current-date) "~Y~m~d~H~M~S"))
    (lambda args (number->string (auto-backup-now-seconds)))))

(tm-define (auto-backup-target-path name doc-id)
  (let* ((base (auto-backup-safe-base name doc-id))
         (stamp (auto-backup-timestamp)))
    (let loop ((i 0))
      (let* ((suffix (if (== i 0) "" (string-append "_" (number->string i))))
             (file (string-append base "_" stamp suffix ".tmu"))
             (target (path-join (auto-backup-dir) file)))
        (if (path-exists? target) (loop (+ i 1)) target)))))

(tm-define (auto-backup-doc-id doc)
  (let ((initial (tmfile-extract doc 'initial)))
    (and initial (collection-ref initial "stem-doc-id"))))

(define (auto-backup-valid-doc-id? doc-id)
  (and (string? doc-id) (!= doc-id "")))

(tm-define (auto-backup-buffer-doc-id name)
  (catch #t
    (lambda ()
      ;; First try to get from init-env (memory), then from document tree (file)
      (with-buffer name
        (let* ((from-env (get-init-env "stem-doc-id"))
               (doc-id (if (and (string? from-env) (!= from-env ""))
                           from-env
                           (let* ((doc (buffer-get name))
                                  (initial (tmfile-extract doc 'initial)))
                             (and initial (collection-ref initial "stem-doc-id"))))))
          (auto-backup-log
           (string-append "buffer-doc-id "
                          (auto-backup-buffer-path name)
                          " -> "
                          (if doc-id doc-id "#f")))
          doc-id)))
    (lambda args
      (auto-backup-log
       (string-append "buffer-doc-id-error "
                      (auto-backup-buffer-path name)))
      #f)))

(tm-define (auto-backup-buffer-needs-doc-id? name)
  (and (auto-backup-buffer-eligible? name)
       (not (auto-backup-valid-doc-id?
             (auto-backup-buffer-doc-id name)))))

;; auto-backup-find-doc-id-by-source-url
;; 在 manifest 中按 source_url 查找已有 doc id。
(define (auto-backup-find-doc-id-by-source-url manifest source-url)
  (catch #t
    (lambda ()
      (let-njson ((docs (njson-ref manifest "documents")))
        (let ((doc-ids (njson-keys docs)))
          (let loop ((keys doc-ids))
            (if (null? keys)
                #f
                (let ((doc-id (car keys)))
                  (let-njson ((doc (njson-ref docs doc-id)))
                    (let ((url (njson-ref doc "source_url")))
                      (if (and url (== url source-url))
                          doc-id
                          (loop (cdr keys)))))))))))
    (lambda args #f)))

;; auto-backup-ensure-buffer-doc-id!
;; 确保可备份 buffer 已经绑定 stem-doc-id。
;;
;; 语法
;; ----
;; (auto-backup-ensure-buffer-doc-id! name)
;;
;; 参数
;; ----
;; name : url
;; 待检查和绑定的 buffer 名称。
;;
;; 返回值
;; ----
;; string or #f
;; 返回已有或新生成的 doc id；不可备份或失败时返回 #f。
;;
;; 逻辑
;; ----
;; 先读取 buffer 当前 init-env 或 initial collection 中的 stem-doc-id；
;; 若没有，则已有文件按 source_url 从 manifest 复用旧 id，新建 scratch
;; 文档直接生成新的 uuid4。
;;
;; 注意
;; ----
;; 这里只写入 init-env，避免触发文档重新解析；已有文件是否持久化由用户
;; 后续保存动作决定。
(tm-define (auto-backup-ensure-buffer-doc-id! name)
  (catch #t
    (lambda ()
      (and (auto-backup-buffer-eligible? name)
           (with-buffer name
             (let ((old-doc-id (auto-backup-buffer-doc-id name)))
               (if (auto-backup-valid-doc-id? old-doc-id)
                   (begin
                     (auto-backup-log
                      (string-append "doc-id-reuse "
                                     (auto-backup-buffer-path name)
                                     " -> "
                                     old-doc-id))
                     old-doc-id)
                   ;; 已保存文件可按 source_url 复用 manifest 中的 doc id；
                   ;; 新建 scratch 文档必须生成新的 doc id。
                   (let-njson ((manifest (auto-backup-load-manifest)))
                     (let* ((source-url (auto-backup-source-url name))
                            (existing-doc-id
                             (and (not (url-scratch? name))
                                  (auto-backup-find-doc-id-by-source-url
                                   manifest source-url)))
                            (doc-id (or existing-doc-id (uuid4))))
                       ;; 写入 init-env 即可绑定到当前会话，避免 buffer-set 触发
                       ;; 文档重新解析。
                       (init-env "stem-doc-id" doc-id)
                       (auto-backup-log
                        (string-append "doc-id-created "
                                       (auto-backup-buffer-path name)
                                       (if existing-doc-id
                                           " (reused from manifest)"
                                           "")))
                       doc-id)))))))
    (lambda args
      (auto-backup-log
       (string-append "doc-id-create-failed "
                      (auto-backup-buffer-path name)))
      #f)))

(define (auto-backup-empty-collection? col)
  (and (pair? col) (== (car col) 'collection) (null? (cdr col))))

(define (auto-backup-tmfile-drop doc what)
  (let ((sdoc (if (tree? doc) (tree->stree doc) doc)))
    (if (and (pair? sdoc) (== (car sdoc) 'document))
        (cons 'document
              (list-filter (cdr sdoc)
                           (lambda (x)
                             (not (and (pair? x) (== (car x) what))))))
        sdoc)))

(tm-define (auto-backup-doc-with-doc-id doc doc-id)
  ;; Keep the live buffer environment in sync, and return an exportable tree
  ;; whose initial collection contains the same stable id.
  (init-env "stem-doc-id" doc-id)
  (let* ((initial (or (tmfile-extract doc 'initial) '(collection)))
         (initial* (collection-set initial "stem-doc-id" doc-id))
         (doc* (and initial* (tmfile-assign doc 'initial initial*))))
    (or doc* doc)))

(tm-define (auto-backup-doc-without-doc-id doc)
  (let* ((initial (tmfile-extract doc 'initial))
         (initial* (and initial (collection-exclude initial '("stem-doc-id")))))
    (cond ((not initial) (if (tree? doc) (tree->stree doc) doc))
          ((auto-backup-empty-collection? initial*)
           (auto-backup-tmfile-drop doc 'initial))
          (else (tmfile-assign doc 'initial initial*)))))

(tm-define (auto-backup-canonical-md5 doc)
  (md5 (object->string (auto-backup-doc-without-doc-id doc))))

(define (auto-backup-display-name name)
  (let ((tail (path-name (auto-backup-buffer-path name))))
    (if (== tail "") "Untitled" tail)))

(define (auto-backup-source-url name)
  (auto-backup-buffer-path name))

(tm-define (auto-backup-buffer-info name device-id app-version)
  (catch #t
    (lambda ()
      (let* ((doc-id (auto-backup-ensure-buffer-doc-id! name))
             (doc (buffer-get name))
             (content-md5 (auto-backup-canonical-md5 doc)))
        (and doc-id
             (list (cons "doc_id" doc-id)
                   (cons "md5" content-md5)
                   (cons "display_name" (auto-backup-display-name name))
                   (cons "source_url" (auto-backup-source-url name))
                   (cons "format" (auto-backup-format name))
                   (cons "device_id" device-id)
                   (cons "liiistem_version" app-version)
                   (cons "doc" doc)))))
    (lambda args
      (auto-backup-log
       (string-append "buffer-info-failed "
                      (auto-backup-buffer-path name)))
      #f)))

(define (auto-backup-file-size target)
  (catch #t
    (lambda () (path-getsize target))
    (lambda args 0)))

(tm-define (auto-backup-export-buffer name target info)
  (catch #t
    (lambda ()
      (let ((doc-id (assoc-ref info "doc_id")))
        ;; Reuse the same export path as manual save/autosave so embedded
        ;; RAW_DATA images stay binary-safe; only the backup file is written.
        (with-buffer name
          (init-env "stem-doc-id" doc-id))
        (if (buffer-export name (auto-backup-path->url target) "tmu")
            #f
            (auto-backup-file-size target))))
    (lambda args
      (auto-backup-log
       (string-append "export-failed " (auto-backup-buffer-path name)
                      " -> " target))
      #f)))

(define (auto-backup-document-ref manifest doc-id)
  (catch #t
    (lambda () (njson-ref manifest "documents" doc-id))
    (lambda args #f)))

(define (auto-backup-new-doc-record doc-id)
  (json->njson `(("doc_id" . ,doc-id) ("versions" . #()))))

(define (auto-backup-set-doc-fields! doc info now)
  (catch #t
    (lambda () (njson-drop! doc "user_id"))
    (lambda args #f))
  (njson-set! doc "doc_id" (assoc-ref info "doc_id"))
  (njson-set! doc "display_name" (assoc-ref info "display_name"))
  (njson-set! doc "source_url" (assoc-ref info "source_url"))
  (njson-set! doc "format" (assoc-ref info "format"))
  (njson-set! doc "device_id" (assoc-ref info "device_id"))
  (njson-set! doc "liiistem_version" (assoc-ref info "liiistem_version"))
  (njson-set! doc "last_checked_at" now)
  (when (not (catch #t
               (lambda ()
                 (let-njson ((versions (njson-ref doc "versions")))
                   (njson-array? versions)))
               (lambda args #f)))
    (let-njson ((versions (string->njson "[]")))
      (njson-set! doc "versions" versions)))
  doc)

(define (auto-backup-ensure-doc-record! manifest info now)
  (let* ((doc-id (assoc-ref info "doc_id"))
         (old (auto-backup-document-ref manifest doc-id)))
    (let-njson ((doc (or old (auto-backup-new-doc-record doc-id))))
      (auto-backup-set-doc-fields! doc info now)
      (njson-set! manifest "documents" doc-id doc))))

(define (auto-backup-version-created-at version)
  (with t (assoc-ref version "created_at")
    (if (number? t) t 0)))

(define (auto-backup-sort-versions versions)
  (list-sort versions
             (lambda (a b)
               (> (auto-backup-version-created-at a)
                  (auto-backup-version-created-at b)))))

(define (auto-backup-doc-versions doc)
  (catch #t
    (lambda ()
      (let-njson ((versions (njson-ref doc "versions")))
        (if (njson-array? versions)
            (vector->list (njson->json versions))
            '())))
    (lambda args '())))

(tm-define (auto-backup-latest-version manifest doc-id)
  (let-njson ((doc (auto-backup-document-ref manifest doc-id)))
    (and doc
         (let ((versions (auto-backup-sort-versions
                          (auto-backup-doc-versions doc))))
           (and (not (null? versions)) (car versions))))))

(define (auto-backup-version-json target kind info size now)
  `(("path" . ,target)
    ("created_at" . ,now)
    ("kind" . ,kind)
    ("md5" . ,(assoc-ref info "md5"))
    ("liiistem_version" . ,(assoc-ref info "liiistem_version"))
    ("size" . ,size)
    ("uploaded" . #f)
    ("upload_status" . "pending")))

(define (auto-backup-remove-version-file version)
  (let ((path (assoc-ref version "path")))
    (when (and (string? path) (!= path ""))
      (catch #t
        (lambda ()
          (when (path-exists? path)
            (path-unlink path)
            (auto-backup-log
             (string-append "retention-removed " path))))
        (lambda args
          (auto-backup-log
           (string-append "retention-remove-failed " path)))))))

(define (auto-backup-njson-number obj key)
  (catch #t
    (lambda ()
      (let ((v (njson-ref obj key)))
        (if (number? v) v 0)))
    (lambda args 0)))

(define (auto-backup-latest-version-time versions)
  (if (null? versions)
      0
      (auto-backup-version-created-at
       (car (auto-backup-sort-versions versions)))))

(define (auto-backup-doc-last-activity doc)
  (let ((versions (auto-backup-doc-versions doc)))
    (max (auto-backup-njson-number doc "last_checked_at")
         (auto-backup-njson-number doc "last_backup_at")
         (auto-backup-latest-version-time versions))))

(define (auto-backup-stale-version? version cutoff)
  (let ((created-at (auto-backup-version-created-at version)))
    (and (> created-at 0) (< created-at cutoff))))

(define (auto-backup-stale-doc? doc cutoff)
  (let ((last-activity (auto-backup-doc-last-activity doc)))
    (and (> last-activity 0) (< last-activity cutoff))))

(define (auto-backup-clean-stale-versions! manifest doc-id doc cutoff)
  (let* ((versions (auto-backup-doc-versions doc))
         (dropped (filter (lambda (version)
                            (auto-backup-stale-version? version cutoff))
                          versions)))
    (if (null? dropped)
        #f
        (begin
          (for-each auto-backup-remove-version-file dropped)
          (let* ((kept (remove (lambda (version)
                                 (auto-backup-stale-version? version cutoff))
                               versions)))
            (let-njson ((kept-json (json->njson (list->vector kept))))
              (njson-set! doc "versions" kept-json))
            (njson-set! manifest "documents" doc-id doc)
            (auto-backup-log
             (string-append "manifest-removed-stale-versions "
                            doc-id)))
          #t))))

;; auto-backup-clean-stale-documents!
;; 清理 manifest 中超过保留时间的文档记录和版本记录。
;;
;; 语法
;; ----
;; (auto-backup-clean-stale-documents! manifest)
;;
;; 参数
;; ----
;; manifest : njson
;; 自动备份 manifest 对象。
;;
;; 返回值
;; ----
;; boolean
;; #t 表示 manifest 有清理改动，#f 表示无改动或清理失败。
;;
;; 逻辑
;; ----
;; 以当前时间向前 30 天作为 cutoff。文档最后检查、最后备份和最新版本
;; 都早于 cutoff 时，删除整个文档记录；仍活跃的文档只删除过期版本。
;;
;; 注意
;; ----
;; 被清理的版本会同步删除本地备份文件，manifest 的 njson 释放仍由外层
;; let-njson 负责。
(tm-define (auto-backup-clean-stale-documents! manifest)
  (let* ((now (auto-backup-now-seconds))
         (cutoff (- now (* auto-backup-record-retention-days
                           auto-backup-day-seconds))))
    (catch #t
      (lambda ()
        (let-njson ((docs (njson-ref manifest "documents")))
          (let ((changed? #f))
            (for-each
             (lambda (doc-id)
               (let-njson ((doc (njson-ref docs doc-id)))
                 (when doc
                   (if (auto-backup-stale-doc? doc cutoff)
                       (begin
                         (for-each auto-backup-remove-version-file
                                   (auto-backup-doc-versions doc))
                         (njson-drop! manifest "documents" doc-id)
                         (set! changed? #t)
                         (auto-backup-log
                          (string-append "manifest-removed-stale-doc "
                                         doc-id)))
                       (when (auto-backup-clean-stale-versions!
                              manifest doc-id doc cutoff)
                         (set! changed? #t))))))
             (njson-keys docs))
            changed?)))
      (lambda args
        (auto-backup-log "manifest-clean-stale-failed")
        #f))))

(tm-define (auto-backup-apply-retention! manifest doc-id)
  (let-njson ((doc (auto-backup-document-ref manifest doc-id)))
    (when doc
      (let* ((versions (auto-backup-sort-versions
                        (auto-backup-doc-versions doc)))
             (count (length versions))
             (kept (if (> count auto-backup-retention-count)
                       (list-head versions auto-backup-retention-count)
                       versions))
             (dropped (if (> count auto-backup-retention-count)
                          (list-tail versions auto-backup-retention-count)
                          '())))
        (for-each auto-backup-remove-version-file dropped)
        (let-njson ((kept-json (json->njson (list->vector kept))))
          (njson-set! doc "versions" kept-json))
        (njson-set! manifest "documents" doc-id doc)))))

(tm-define (auto-backup-touch-manifest! manifest info)
  (let ((now (auto-backup-now-seconds)))
    (auto-backup-ensure-doc-record! manifest info now)))

(tm-define (auto-backup-upsert-version! manifest info target kind size)
  (let* ((now (auto-backup-now-seconds))
         (doc-id (assoc-ref info "doc_id")))
    (auto-backup-ensure-doc-record! manifest info now)
    (let-njson ((doc (auto-backup-document-ref manifest doc-id)))
      (when doc
        (let* ((version (auto-backup-version-json target kind info size now))
               (versions (cons version (auto-backup-doc-versions doc))))
          (njson-set! doc "last_backup_at" now)
          (let-njson ((versions-json (json->njson (list->vector versions))))
            (njson-set! doc "versions" versions-json))
          (njson-set! manifest "documents" doc-id doc))))
    (auto-backup-apply-retention! manifest doc-id)))

(define (auto-backup-remove-partial target)
  (catch #t
    (lambda ()
      (when (path-exists? target) (path-unlink target)))
    (lambda args
      (auto-backup-log
       (string-append "partial-remove-failed " target)))))

;; auto-backup-buffer-do
;; 执行实际的自动备份写文件和 manifest 更新逻辑。
(define (auto-backup-buffer-do name kind)
  (let-njson ((manifest (auto-backup-load-manifest)))
    (let* ((device-id (stem-device-id))
           (app-version (liiistem-version))
           (info (auto-backup-buffer-info
                  name device-id app-version)))
      (if (not info)
          'backup-failed
          (let* ((doc-id (assoc-ref info "doc_id"))
                 (content-md5 (assoc-ref info "md5"))
                 (latest (auto-backup-latest-version manifest doc-id))
                 (latest-md5 (and latest (assoc-ref latest "md5"))))
            (if (and (string? latest-md5)
                     (== latest-md5 content-md5))
                (begin
                  (auto-backup-touch-manifest! manifest info)
                  (auto-backup-save-manifest! manifest)
                  (auto-backup-log
                   (string-append "skip-same-md5 "
                                  (auto-backup-buffer-path name)))
                  'skip-same-md5)
                (let* ((target (auto-backup-target-path name doc-id))
                       (size (auto-backup-export-buffer
                              name target info)))
                  (if (not size)
                      (begin
                        (auto-backup-remove-partial target)
                        'backup-failed)
                      (begin
                        (auto-backup-upsert-version!
                         manifest info target kind size)
                        (auto-backup-save-manifest! manifest)
                        (auto-backup-log
                         (string-append "saved " target))
                        'backup)))))))))

(tm-define (auto-backup-buffer name . kind*)
  (let ((kind (if (null? kind*) "auto" (car kind*))))
    (cond ((and (== kind "auto") (not (buffer-modified? name)))
           (auto-backup-log
            (string-append "skip-clean "
                           (auto-backup-buffer-path name)))
           'skip-clean)
          ((not (auto-backup-buffer-eligible? name))
           (auto-backup-log
            (string-append "skip-ineligible "
                           (auto-backup-buffer-path name)))
           'skip-ineligible)
          ((not (auto-backup-ensure-dir!))
           'backup-failed)
          (else
           ;; For on-open: proceed even if not modified
           (when (== kind "on-open")
             (auto-backup-log
              (string-append "on-open "
                             (auto-backup-buffer-path name))))
           (auto-backup-buffer-do name kind)))))

;; auto-backup-opened-buffer!
;; 文件打开后的自动备份准备流程。
;;
;; 语法
;; ----
;; (auto-backup-opened-buffer! name)
;;
;; 参数
;; ----
;; name : url
;; 已经打开并切换完成的 buffer 名称。
;;
;; 逻辑
;; ----
;; 打开文件时只在当前会话中绑定缺失的 stem-doc-id，避免静默改写源文件；
;; 随后延迟触发一次 on-open 备份，由 md5 去重避免重复版本。
(define (auto-backup-opened-buffer! name)
  (auto-backup-ensure-buffer-doc-id! name)
  (delayed (:pause 100)
    (auto-backup-buffer name "on-open")))

(tm-define (auto-backup-all)
  (let ((buffers (buffer-list)))
    (auto-backup-log
     (string-append "periodic-scan buffers="
                    (number->string (length buffers))))
    (for-each (lambda (name) (auto-backup-buffer name "periodic"))
              buffers)))

(tm-define (auto-backup-now)
  (set! auto-backup-scheduled? #f)
  (if (auto-backup-enabled?)
      (begin
        (auto-backup-log "timer-fired")
        (auto-backup-all)
        (auto-backup-delayed))
      (auto-backup-log "timer-skip-disabled")))

(tm-define (auto-backup-delayed)
  (if (auto-backup-enabled?)
      (if auto-backup-scheduled?
          (auto-backup-log "schedule-skip-already-pending")
          (begin
            (set! auto-backup-scheduled? #t)
            (auto-backup-log "schedule-next 120s")
            (delayed
              (:pause auto-backup-fixed-interval-ms)
              (auto-backup-now))))
      (auto-backup-log "schedule-disabled")))

(tm-define (auto-backup-official-url)
  (if (== (get-output-language) "chinese")
      "https://liiistem.cn/?utm_source=auto_backup_button"
      "https://liiistem.com/?utm_source=auto_backup_button"))

(tm-define (auto-backup-upload-buffer name backup-result)
  (noop))

(tm-define (auto-backup-button-label)
  (if (community-stem?) "Open backup folder" "Cloud backup"))

(tm-define (open-auto-backup-location)
  (let ((name (auto-backup-manual-target)))
    (when name
      (let ((backup-result (auto-backup-buffer name "manual-open")))
        (when (not (community-stem?))
          (auto-backup-upload-buffer name backup-result)))))
  (if (community-stem?)
      (begin
        (auto-backup-ensure-dir!)
        (open-url (auto-backup-path->url (auto-backup-dir))))
      (open-url (auto-backup-official-url))))

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
  (when (autosave-enabled?)
    (autosave-all)
    (autosave-delayed)))

(tm-define (save-all-buffers)
  (for-each (lambda (buf)
              (when (buffer-modified? buf)
                (auto-backup-ensure-buffer-doc-id! buf)
                (buffer-save buf)))
            (buffer-list)))

(tm-define (autosave-delayed)
  (when (autosave-enabled?)
    (delayed
      (:pause autosave-fixed-interval-ms)
      (autosave-now))))

(define (notify-autosave var val)
  (if (current-view) ; delayed-autosave would crash at initialization time
      (begin
        (autosave-delayed)
        (auto-backup-delayed))))

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
  (auto-backup-opened-buffer! name)
  (noop))

(define (load-buffer-load name opts)
  ;;(display* "load-buffer-load " name ", " opts "\n")
  (let* ((path (url->system name))
         (vname `(verbatim ,(utf8->cork path))))
    (cond ((buffer-exists? name)
           (begin
             (load-buffer-open name opts)
             (sync-buffer-dark-style-with-gui-theme name)))
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
