
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
    (utils library cursor)
  ) ;:use
) ;texmacs-module

(import (only (liii string) string-contains))
(import (only (liii hashlib) md5))
(import (only (liii uuid) uuid4))
(import (only (liii path)
          path->string
          path-dir?
          path-exists?
          path-from-env
          path-from-string
          path-getsize
          path-join
          path-name
          path-parent
          path-rename
          path-root
          path-stem
          path-unlink
        ) ;only
) ;import
(import (only (liii os) mkdir))
(import (liii njson))
(import (liii json))
(import (only (srfi srfi-1) find))
(import (only (srfi srfi-1) remove))
(import (only (srfi srfi-19)
          TIME-UTC
          current-date
          current-time
          date->string
          time-second
        ) ;only
) ;import

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Remember last save/open directory
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define last-file-dialog-directory #f)

(tm-define (get-last-file-dialog-directory)
  "Get the last directory used in file dialog"
  (or last-file-dialog-directory (get-preference "last-file-dialog-directory"))
) ;tm-define

(tm-define (set-last-file-dialog-directory dir)
  "Set the last directory used in file dialog"
  (let ((u (system->url dir)))
    (when (and (string? dir)
            (url-exists? u)
            (url-directory? u)
            (not (url-descends? u (get-texmacs-path)))
          ) ;and
      (set! last-file-dialog-directory dir)
      (set-preference "last-file-dialog-directory" dir)
    ) ;when
  ) ;let
) ;tm-define

(tm-define (remember-file-dialog-directory name)
  "Remember the directory from a file operation"
  (when (url? name)
    (let ((dir (url->system (url-head name))))
      (set-last-file-dialog-directory dir)
    ) ;let
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Check whether the file name is valid (exclude *)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (url-contains-wildcard? u) (string-contains (url->system u) "*"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Supplementary routines on urls, taking into account the TeXmacs file system
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define cpp-url-last-modified url-last-modified)

(define cpp-url-newer? url-newer?)

(define cpp-buffer-last-save buffer-last-save)

(tm-define (url-last-modified u)
  (if (url-rooted-tmfs? u) (tmfs-date u) (cpp-url-last-modified u))
) ;tm-define

(tm-define (url-newer? u1 u2)
  (if (or (url-rooted-tmfs? u1) (url-rooted-tmfs? u2))
    (and-let* ((d1 (url-last-modified u1)) (d2 (url-last-modified u2))) (> d1 d2))
    (cpp-url-newer? u1 u2)
  ) ;if
) ;tm-define

(tm-define (url-remove u)
  (if (url-rooted-tmfs? u) (tmfs-remove u) (system-remove u))
) ;tm-define

(tm-define (url-autosave u suf)
  (if (url-rooted-tmfs? u)
    (tmfs-autosave u suf)
    (and (or (url-scratch? u) (url-test? u "fw") (not (url-exists? u)) (url-glue u suf))
    ) ;and
  ) ;if
) ;tm-define

(tm-define (url-wrap u) (and (url-rooted-tmfs? u) (tmfs-wrap u)))

(tm-define (buffer-last-save u)
  (with base
    (url-wrap u)
    (cond ((not base) (cpp-buffer-last-save u))
          ((buffer-exists? base) (buffer-last-save base))
          (else (url-last-modified base))
    ) ;cond
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Miscellaneous subroutines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (buffer-missing-style?)
  (with t
    (tree->stree (get-style-tree))
    (and (pair? t) (== (car t) 'tuple) (null? (cdr t)))
  ) ;with
) ;tm-define

(tm-define (sync-buffer-dark-style-with-gui-theme . opt-buf)
  (with buf
    (if (null? opt-buf) (current-buffer) (car opt-buf))
    (with-buffer buf
      (if (== (get-preference "gui theme") "liii-night")
        (when (not (has-style-package? "dark"))
          (add-style-package "dark")
        ) ;when
        (when (has-style-package? "dark")
          (remove-style-package "dark")
        ) ;when
      ) ;if
    ) ;with-buffer
  ) ;with
) ;tm-define

(tm-define (buffer-set-default-style)
  (init-style "generic")
  (with lan
    (get-preference "language")
    (if (!= lan "english") (set-document-language lan))
  ) ;with
  (with psz (get-printer-paper-type) (if (!= psz "a4") (init-page-type psz)))
  (with type
    (get-preference "page medium")
    (if (!= type "papyrus") (init-env "page-medium" type))
  ) ;with
  (with type
    (get-preference "page screen margin")
    (if (!= type "true") (init-env "page-screen-margin" type))
  ) ;with
  (when (!= (get-preference "scripting language") "none")
    (lazy-plugin-force)
    (init-env "prog-scripts" (get-preference "scripting language"))
  ) ;when
  (add-style-package "number-europe")
  (add-style-package "preview-ref")
  (sync-buffer-dark-style-with-gui-theme (current-buffer))
  (buffer-pretend-saved (current-buffer))
) ;tm-define

(tm-define (propose-name-buffer)
  (with name
    (url->unix (current-buffer))
    (cond ((not (url-scratch? name)) name)
          ((os-win32?) "")
          (else (string-append (var-eval-system "pwd") "/"))
    ) ;cond
  ) ;with
) ;tm-define

(tm-property (choose-file fun text type) (:interactive #t))

(tm-define (open-auxiliary aux body . opt-master)
  (let* ((name (aux-name aux))
         (master (if (null? opt-master) (buffer-master) (car opt-master)))
        ) ;
    (aux-set-document aux body)
    (aux-set-master aux master)
    (switch-document name)
  ) ;let*
) ;tm-define

(define-public-macro (with-aux u . prg)
  `(let* ((u ,u)
          (t (tree-import u "texmacs"))
          (name (current-buffer))
          (aux "* Aux *"))
     (aux-set-document aux t)
     (aux-set-master aux u)
     (switch-to-buffer (aux-name aux))
     (with r (begin ,@prg) (switch-to-buffer name) r))
) ;define-public-macro

(tm-define (buffer-copy buf u)
  (:synopsis "Creates a copy of @buf in @u and return @u")
  (with-buffer buf
    (let* ((styles (get-style-list))
           (init (get-all-inits))
           (refl (list-references))
           (refs (map get-reference refl))
           (body (tree-copy (buffer-get-body buf)))
          ) ;
      (view-new u)
      (buffer-set-body u body)
      (with-buffer u
        (set-style-list styles)
        (init-env "global-title" (buffer-get-metadata buf "title"))
        (init-env "global-author" (buffer-get-metadata buf "author"))
        (init-env "global-subject" (buffer-get-metadata buf "subject"))
        (for-each (lambda (t)
                    (if (tree-func? t 'associate)
                      (with (var val)
                        (list (tree-ref t 0) (tree-ref t 1))
                        (init-env-tree (tree->string var) val)
                      ) ;with
                    ) ;if
                  ) ;lambda
          (tree-children init)
        ) ;for-each
        (for-each set-reference refl refs)
      ) ;with-buffer
      u
    ) ;let*
  ) ;with-buffer
) ;tm-define

(tm-define (buffer->windows-of-tabpage buf)
  (remove (lambda (vw) (or (not vw) (url-none? vw)))
    (map view->window-of-tabpage (buffer->views buf))
  ) ;remove
) ;tm-define

(tm-define (switch-to-buffer* buf)
  (let* ((wins (buffer->windows-of-tabpage buf))
         (win (if (member (current-window) wins) (current-window) (car wins)))
         (view (if (member (current-window) wins)
                 (find (lambda (vw) (== (view->window-of-tabpage vw) win)) (buffer->views buf))
                 (car (buffer->views buf))
               ) ;if
         ) ;view
        ) ;
    (cond ((eq? buf (current-buffer)) (noop))
          ((nnull? (buffer->windows-of-tabpage buf))
           (switch-to-window win)
           (window-set-view win view #t)
          ) ;
          (else (switch-to-buffer buf))
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (switch-to-buffer-index index)
  (let* ((lst (buffer-menu-unsorted-list 99)) (len (length lst)))
    (when (and (integer? index) (>= index 0) (< index len))
      (let ((buf (list-ref lst index)))
        (switch-to-buffer* buf)
      ) ;let
    ) ;when
  ) ;let*
) ;tm-define

(tm-define (switch-to-view-index index)
  (let* ((lst (tabpage-list #t))
         ;; #t stands for current window
         (len (length lst))
        ) ;
    (when (and (integer? index) (>= index 0) (< index len))
      (let* ((view (list-ref lst index)) (view-win (view->window-of-tabpage view)))
        (window-set-view view-win view #t)
      ) ;let*
    ) ;when
  ) ;let*
) ;tm-define

(tm-define (ensure-default-view)
  (:synopsis "Switch to parent window if not in default view")
  (if (not (is-view-type? (current-view) "default")) (switch-to-parent-window))
) ;tm-define

(tm-define-macro (with-default-view . body)
  (:synopsis "Ensure we are in a default view, then execute @body")
  `(begin (ensure-default-view) (exec-delayed (lambda ,() ,@body)))
) ;tm-define-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Saving buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define current-save-source (url-none))
(tm-define current-save-target (url-none))

(define (buffer-notify-recent name)
  (learn-interactive 'recent-buffer (list (cons "0" (url->system name))))
) ;define

(define (has-faithful-format? name)
  (in? (url-suffix name) '("tm" "ts" "tp" "stm" "scm" "tmu"))
) ;define

(define (save-buffer-post name opts)
  ;; (display* "save-buffer-post " name "\n")
  (cond ((in? :update opts) (update-buffer name))
        ((in? :commit opts) (commit-buffer name))
  ) ;cond
) ;define

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

(tm-define (save-buffer-save name opts . kind*)
  ;; (display* "save-buffer-save " name "\n")
  (let ((kind (if (null? kind*) "save" (car kind*))))
    (with vname
      `(verbatim ,(utf8->cork (url->system name)))
      (auto-backup-ensure-buffer-doc-id! name)
      (if (buffer-save name)
        (begin
          (buffer-pretend-modified name)
          (set-message `(concat ,"Could not save " ,vname) "Save file")
        ) ;begin
        (begin
          (if (== (url-suffix name) "ts") (style-clear-cache))
          (buffer-notify-recent name)
          ;; Remember directory for file dialog
          (remember-file-dialog-directory name)
          (set-message `(concat ,"Saved " ,vname) "Save file")
          (auto-backup-trig name kind)
          (save-buffer-post name opts)
        ) ;begin
      ) ;if
    ) ;with
  ) ;let
) ;tm-define

(define (save-buffer-check-faithful name opts)
  ;; (display* "save-buffer-check-faithful " name "\n")
  (if (has-faithful-format? name)
    (save-buffer-save name opts)
    (user-confirm "Save requires data conversion. Really proceed?"
      #f
      (lambda (answ) (when answ (save-buffer-save name opts)))
    ) ;user-confirm
  ) ;if
) ;define

(tm-widget (readonly-file-dialog-widget cmd)
  (resize "500guipx"
    "200guipx"
    (centered (bold (text (translate "Read-only")))
      (glue #t #f 12 6)
      (text "The current document or its directory has read-only attributes.")
      (text "You can save the document using Save as.")
    ) ;centered
    (bottom-buttons >>
     ("Save as" (cmd "save_as"))
     ///
     ("Cancel" (cmd "cancel"))
     ///
    ) ;bottom-buttons
  ) ;resize
) ;tm-widget

(define (cannot-write? name action)
  (with vname
    `(verbatim ,(utf8->cork (url->system name)))
    (cond ((and (not (url-test? name "f")) (url-exists? name))
           (with msg
             "The file cannot be created:"
             (notify-now `(concat ,msg ,"<br>" ,vname))
           ) ;with
           #t
          ) ;
          ((and (url-test? name "f") (not (url-test? name "w")))
           (dialogue-window readonly-file-dialog-widget
             (lambda (answer)
               (when (== answer "save_as")
                 (choose-file save-buffer-as "Save TeXmacs file" "action_save_as")
               ) ;when
             ) ;lambda
             "Failed to save"
           ) ;dialogue-window
          ) ;
          (else #f)
    ) ;cond
  ) ;with
) ;define

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
  ;; (display* "save-buffer-check-permissions " name "\n")
  (set! current-save-source name)
  (set! current-save-target name)
  (with vname
    `(verbatim ,(utf8->cork (url->system name)))
    (cond ((url-scratch? name)
           (choose-file (lambda (x) (apply save-buffer-as-main (cons x opts)))
             "Save TeXmacs file"
             "tmu"
           ) ;choose-file
          ) ;
          ((not (buffer-exists? name))
           (with msg
             `(concat ,"The buffer " ,vname ," does not exist")
             (set-message msg "Save file")
           ) ;with
          ) ;
          ((and (not (buffer-modified? name)) (auto-backup-buffer-needs-doc-id? name))
           (if (cannot-write? name "Save file")
             (noop)
             (begin
               (auto-backup-ensure-buffer-doc-id! name)
               (save-buffer-check-faithful name opts)
             ) ;begin
           ) ;if
          ) ;
          ((not (buffer-modified? name))
           (with msg "No changes need to be saved" (set-message msg "Save file"))
           (save-buffer-post name opts)
          ) ;
          ((cannot-write? name "Save file") (noop))
          ((and (url-test? name "fr")
             (and-with mod-t
               (url-last-modified name)
               (and-with save-t (buffer-last-save name) (> mod-t save-t))
             ) ;and-with
           ) ;and
           (user-confirm "The file has changed on disk. Really save?"
             #f
             (lambda (answ) (when answ (save-buffer-check-faithful name opts)))
           ) ;user-confirm
          ) ;
          (else (save-buffer-check-faithful name opts))
    ) ;cond
  ) ;with
) ;define

(tm-define (save-buffer-main . args)
  ;; (display* "save-buffer-main\n")
  (if (or (null? args) (not (url? (car args))))
    (save-buffer-check-permissions (current-buffer) args)
    (save-buffer-check-permissions (car args) (cdr args))
  ) ;if
) ;tm-define

(tm-define (save-buffer . l) (with-default-view (apply save-buffer-main l)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Saving buffers under a new name
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (save-buffer-as-save new-name name opts)
  ;; (display* "save-buffer-as-save " new-name ", " name "\n")
  (if (and (url-scratch? name) (url-exists? name)) (system-remove name))
  (buffer-rename name new-name)
  (buffer-pretend-modified new-name)
  (save-buffer-save new-name opts "save-as")
) ;define

(define (save-buffer-as-check-faithful new-name name opts)
  ;; (display* "save-check-as-check-faithful " new-name ", " name "\n")
  (if (or (== (url-suffix new-name) (url-suffix name))
        (has-faithful-format? new-name)
      ) ;or
    (save-buffer-as-save new-name name opts)
    (user-confirm "Save requires data conversion. Really proceed?"
      #f
      (lambda (answ) (when answ (save-buffer-as-save new-name name opts)))
    ) ;user-confirm
  ) ;if
) ;define

(define (save-buffer-as-check-other new-name name opts)
  ;; (display* "save-buffer-as-check-other " new-name ", " name "\n")
  (cond ((buffer-exists? new-name)
         (with s
           (string-append "The file "
             (url->system new-name)
             " is being edited. Discard edits?"
           ) ;string-append
           (user-confirm s
             #f
             (lambda (answ) (when answ (save-buffer-as-save new-name name opts)))
           ) ;user-confirm
         ) ;with
        ) ;
        (else (save-buffer-as-save new-name name opts))
  ) ;cond
) ;define

(define (save-buffer-as-check-permissions new-name name opts)
  ;; (display* "save-buffer-as-check-permissions " new-name ", " name "\n")
  (cond ((cannot-write? new-name "Save file") (noop))
        ((and (url-test? new-name "f") (nin? :overwrite opts))
         (user-confirm "File already exists. Really overwrite?"
           #f
           (lambda (answ) (when answ (save-buffer-as-check-other new-name name opts)))
         ) ;user-confirm
        ) ;
        (else (save-buffer-as-check-other new-name name opts))
  ) ;cond
) ;define

(tm-define (save-buffer-as-main new-name . args)
  ;; (display* "save-buffer-as-main " new-name "\n")
  (if (or (null? args) (not (url? (car args))))
    (save-buffer-as-check-permissions new-name (current-buffer) args)
    (save-buffer-as-check-permissions new-name (car args) (cdr args))
  ) ;if
) ;tm-define

(tm-define (save-buffer-as new-name . args)
  (:argument new-name texmacs-file "Save as")
  (:default new-name (propose-name-buffer))
  (with-default-view (when (string? new-name)
                       (set! new-name (string-replace new-name ":" "-"))
                       (set! new-name (string-replace new-name ";" "-"))
                     ) ;when
    (with opts
      (if (x-gui?) args (cons :overwrite args))
      (apply save-buffer-as-main (cons new-name opts))
    ) ;with
  ) ;with-default-view
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Exporting buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (export-buffer-export name to fm opts)
  ;; (display* "export-buffer-export " name ", " to ", " fm "\n")
  (with vto
    `(verbatim ,(url->system to))
    (if (buffer-export name to fm)
      (set-message `(concat ,"Could not save " ,vto) "Export file")
      (begin
        (set-message `(concat ,"Exported to " ,vto) "Export file")
        (let ((export-kind (string-append fm "_export")))
          (save-buffer-save name opts export-kind)
        ) ;let
      ) ;begin
    ) ;if
  ) ;with
) ;define

(define (export-buffer-check-permissions name to fm opts)
  ;; (display* "export-buffer-check-permissions " name ", " to ", " fm "\n")
  (cond ((cannot-write? to "Export file") (noop))
        ((and (url-test? to "f") (nin? :overwrite opts))
         (user-confirm "File already exists. Really overwrite?"
           #f
           (lambda (answ) (when answ (export-buffer-export name to fm opts)))
         ) ;user-confirm
        ) ;
        (else (export-buffer-export name to fm opts))
  ) ;cond
) ;define

(tm-define (export-buffer-main name to fm opts)
  ;; (display* "export-buffer-main " name ", " to ", " fm "\n")
  (when (string? to)
    (set! to (string-replace to ":" "-"))
    (set! to (string-replace to ";" "-"))
    (set! to (url-relative (buffer-get-master name) to))
  ) ;when
  (if (url? name) (set! current-save-source name))
  (if (url? to) (set! current-save-target to))
  (export-buffer-check-permissions name to fm opts)
) ;tm-define

(tm-define (export-buffer to)
  (with fm
    (url-format to)
    (if (== fm "generic") (set! fm "verbatim"))
    (export-buffer-main (current-buffer) to fm (list :overwrite))
  ) ;with
) ;tm-define

(tm-define (buffer-exporter fm)
  (with opts
    (if (x-gui?) (list) (list :overwrite))
    (lambda (s) (export-buffer-main (current-buffer) s fm opts))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Autosave
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define autosave-fixed-interval-ms 120000)

(tm-define (autosave-enabled?) (!= (get-preference "autosave") "0"))

(tm-define (auto-backup-enabled?) (!= (get-preference "autobackup") "off"))

(tm-define (liiistem-version) (xmacs-version))

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
  (url-descends? name (get-texmacs-path))
) ;tm-define

(define (auto-backup-path->url p)
  (system->url (path->string p))
) ;define

(define (auto-backup-format name)
  (if (url-scratch? name) "texmacs" (url-format name))
) ;define

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
    (in? (auto-backup-format name) '("texmacs" "stm" "mgs" "tmu"))
  ) ;and
) ;tm-define

(define (auto-backup-valid-doc-id? doc-id)
  (and (string? doc-id) (!= doc-id ""))
) ;define

(tm-define (auto-backup-buffer-doc-id name)
  (catch #t
    (lambda ()
      ;; First try to get from init-env (memory), then from document tree (file)
      (with-buffer name
        (let* ((from-env (get-init-env "stem-doc-id"))
               (doc-id (if (and (string? from-env) (!= from-env ""))
                         from-env
                         (let* ((doc (buffer-get name)) (initial (tmfile-extract doc 'initial)))
                           (and initial (collection-ref initial "stem-doc-id"))
                         ) ;let*
                       ) ;if
               ) ;doc-id
              ) ;
          doc-id
        ) ;let*
      ) ;with-buffer
    ) ;lambda
    (lambda args #f)
  ) ;catch
) ;tm-define

(tm-define (auto-backup-buffer-needs-doc-id? name)
  (and (auto-backup-buffer-eligible? name)
    (not (auto-backup-valid-doc-id? (auto-backup-buffer-doc-id name)))
  ) ;and
) ;tm-define

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
;; 若没有，则生成新的 uuid4 并写入 init-env。
;;
;; 注意
;; ----
;; 这里只写入 init-env，避免触发文档重新解析；doc id 是否持久化到文件由
;; 用户后续保存动作决定。
(tm-define (auto-backup-ensure-buffer-doc-id! name)
  (catch #t
    (lambda ()
      (and (auto-backup-buffer-eligible? name)
        (with-buffer name
          (let ((old-doc-id (auto-backup-buffer-doc-id name)))
            (if (auto-backup-valid-doc-id? old-doc-id)
              old-doc-id
              (let ((doc-id (uuid4)))
                ;; 写入 init-env 即可绑定到当前会话，避免 buffer-set 触发
                ;; 文档重新解析。
                (init-env "stem-doc-id" doc-id)
                doc-id
              ) ;let
            ) ;if
          ) ;let
        ) ;with-buffer
      ) ;and
    ) ;lambda
    (lambda args #f)
  ) ;catch
) ;tm-define

(tm-define (auto-backup-trig-payload name kind)
  (let* ((path (url->system name))
         (doc-id (auto-backup-ensure-buffer-doc-id! name))
         (session-id (uuid4))
         (payload (string->json "{}"))
        ) ;
    (set! payload (json-push payload "path" path))
    (set! payload (json-push payload "type" kind))
    (set! payload (json-push payload "id" doc-id))
    (set! payload (json-push payload "session-id" session-id))
    ;; 云备份请求头所需的 4 个静态字段：autosave 子进程通过 payload 拿到这些值，
    ;; 构造 Authorization / User-Agent / X-Device-Id 头和 upload URL。
    ;; 账号模块或 glue 函数在未登录/未加载时会抛异常，逐个 catch 回退空串，
    ;; 避免 payload 构造失败导致整个 copy 流程中断。
    (set! payload
      (json-push payload
        "site"
        (catch #t (lambda () (current-stem-site)) (lambda args ""))
      ) ;json-push
    ) ;set!
    (set! payload
      (json-push payload
        "token"
        (catch #t (lambda () (account-load-token)) (lambda args ""))
      ) ;json-push
    ) ;set!
    (set! payload
      (json-push payload
        "user-agent"
        (catch #t (lambda () (stem-user-agent)) (lambda args ""))
      ) ;json-push
    ) ;set!
    (set! payload
      (json-push payload
        "device-id"
        (catch #t (lambda () (stem-device-id)) (lambda args ""))
      ) ;json-push
    ) ;set!
    (values (json->string payload) session-id)
  ) ;let*
) ;tm-define

;; auto-backup-trig
;; 自动备份触发入口，当前仅用于调试输出触发参数。
;;
;; 语法
;; ----
;; (auto-backup-trig u kind)
;;
;; 参数
;; ----
;; u : url
;; 需要备份的 buffer url。
;;
;; kind : string
;; 备份类型，例如 "save"、"save-as"、"export-pdf"、"on-open"、"auto"、"manual-open"。

(tm-define (auto-backup-trig u kind)
  (when (and (auto-backup-enabled?) (auto-backup-buffer-eligible? u))
    (receive (s session-id)
      (auto-backup-trig-payload u kind)
      (silent-feed* "autosave"
        session-id
        `(document ,(utf8->cork s))
        (lambda (r) (noop))
        '()
      ) ;silent-feed*
    ) ;receive
  ) ;when
) ;tm-define

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
  (delayed (:pause 100) (auto-backup-trig name "on-open"))
) ;define

(tm-define (auto-backup-official-url)
  (if (== (get-output-language) "chinese")
    "https://liiistem.cn/personal-center/backup.html?utm_source=auto_backup_button"
    "https://liiistem.com/?utm_source=auto_backup_button"
  ) ;if
) ;tm-define

(tm-define (auto-backup-button-label)
  (if (community-stem?) "View help" "Cloud backup")
) ;tm-define

(tm-define (open-auto-backup-location)
  (if (community-stem?)
    (open-url "https://liiistem.cn/docs/guide-auto-backup")
    (open-url (auto-backup-official-url))
  ) ;if
  (auto-backup-trig (current-buffer-url) "visit-cloud-backup")
) ;tm-define

(tm-define (autosave-all)
  (for-each (lambda (name)
              (when (and (buffer-modified? name) (not (url-scratch? name)))
                (save-buffer-save name (list) "auto")
              ) ;when
            ) ;lambda
    (buffer-list)
  ) ;for-each
) ;tm-define

(tm-define (autosave-now)
  (when (autosave-enabled?)
    (let ((name (current-buffer)))
      (when (and (buffer-modified? name) (not (url-scratch? name)))
        (save-buffer-save name (list) "auto")
      ) ;when
    ) ;let
    (autosave-delayed)
  ) ;when
) ;tm-define

(tm-define (save-all-buffers)
  (for-each (lambda (buf)
              (when (buffer-modified? buf)
                (auto-backup-ensure-buffer-doc-id! buf)
                (buffer-save buf)
              ) ;when
            ) ;lambda
    (buffer-list)
  ) ;for-each
) ;tm-define

(tm-define (autosave-delayed)
  (when (autosave-enabled?)
    (delayed (:pause autosave-fixed-interval-ms) (autosave-now))
  ) ;when
) ;tm-define

(define (notify-autosave var val)
  (if (current-view) (begin (autosave-delayed)))
) ;define

(define-preferences ("autosave" "120" notify-autosave))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Opening files using external tools
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (buffer-external? u)
  (or (url-rooted-web? u)
    (not (in? (url-root u) (list "tmfs" "file" "default" "blank" "ramdisc")))
    (file-of-format? u "image")
    (file-of-format? u "postscript")
    (file-of-format? u "generic")
  ) ;or
) ;tm-define

(tm-define (load-external u)
  (when (not (url-rooted? u))
    (set! u (url-relative (current-buffer) u))
  ) ;when
  (open-url u)
) ;tm-define

(tm-define (load-pdf-buffer u)
  (when (not (url-rooted? u))
    (set! u (url-relative (current-buffer) u))
  ) ;when
  (if (buffer-exists? u)
    (switch-to-buffer u)
    (begin
      (buffer-set u '(document))
      (buffer-set-title u (url->system (url-tail u)))
      (switch-to-buffer u)
    ) ;begin
  ) ;if
  (buffer-notify-recent u)
  (remember-file-dialog-directory u)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Loading buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (load-buffer-open name opts)
  ;; (display* "load-buffer-open " name ", " opts "\n")
  (cond ((in? :background opts) (noop))
        ((in? :new-window opts) (open-buffer-in-window name (buffer-get name) ""))
        (else
          ;; Remember current buffer to check if it's an unmodified scratch buffer
          (let ((prev-buffer (current-buffer)))
            (with wins
              (buffer->windows-of-tabpage name)
              (if (and (!= wins '()) (in? (current-window) wins))
                (switch-to-buffer* name)
                (switch-to-buffer name)
              ) ;if
            ) ;with
            ;; Close the previous unmodified scratch buffer after loading new file
            (when (and prev-buffer
                    (!= prev-buffer name)
                    (url-scratch? prev-buffer)
                    (not (buffer-modified? prev-buffer))
                  ) ;and
              (cpp-buffer-close prev-buffer)
            ) ;when
          ) ;let
        ) ;else
  ) ;cond
  (buffer-notify-recent name)
  ;; Remember directory for file dialog
  (remember-file-dialog-directory name)
  (when (nnull? (select (buffer-get name) '(:* gpg-passphrase-encrypted-buffer)))
    (tm-gpg-dialogue-passphrase-decrypt-buffer name)
  ) ;when
  (and-with master
    (and (url-rooted-tmfs? name) (tmfs-master name))
    (when (!= master name)
      (buffer-set-master name master)
    ) ;when
  ) ;and-with
  (when (and (in-beamer?)
          (== (get-init-page-rendering) "book")
          (inside? 'slideshow)
          (> (nr-pages) 1)
        ) ;and
    (delayed (:idle 25) (fit-to-screen-width))
  ) ;when
  (auto-backup-opened-buffer! name)
  (noop)
) ;define

(define (load-buffer-load name opts)
  ;; (display* "load-buffer-load " name ", " opts "\n")
  (let* ((path (url->system name)) (vname `(verbatim ,(utf8->cork path))))
    (cond ((buffer-exists? name)
           (begin
             (load-buffer-open name opts)
             (sync-buffer-dark-style-with-gui-theme name)
           ) ;begin
          ) ;
          ((url-exists? name)
           (if (buffer-load name)
             (set-message `(concat ,"Could not load " ,vname) "Load file")
             (load-buffer-open name opts)
           ) ;if
          ) ;
          (else (with msg
                  "The file or buffer does not exist:"
                  (begin
                    (debug-message "debug-io" (string-append msg "\n" path))
                    (notify-now `(concat ,msg ,"<br>" ,vname))
                  ) ;begin
                ) ;with
          ) ;else
    ) ;cond
  ) ;let*
) ;define

(define (load-buffer-check-permissions name opts)
  ;; (display* "load-buffer-check-permissions " name ", " opts "\n")
  (let* ((path (url->system name)) (vname `(verbatim ,(utf8->cork path))))
    (cond ((and (not (url-test? name "f")) (url-exists? name))
           (with msg
             "The file cannot be loaded or created:"
             (begin
               (debug-message "debug-io" (string-append msg "\n" path))
               (notify-now `(concat ,msg ,"<br>" ,vname))
             ) ;begin
           ) ;with
          ) ;
          ((and (url-test? name "f") (not (url-test? name "r")))
           (with msg
             `(concat ,(translate "You do not have read access to") ," " ,vname)
             (show-message msg "Load file")
           ) ;with
          ) ;
          (else (load-buffer-load name opts))
    ) ;cond
  ) ;let*
) ;define

(define (load-buffer-check-autosave name opts)
  ;; (display* "load-buffer-check-autosave " name ", " opts "\n")
  (load-buffer-check-permissions name opts)
) ;define

(tm-define (load-buffer-main name . opts)
  ;; (display* "load-buffer-main " name ", " opts "\n")
  (if (and (not (url-exists? name))
        (url-exists? (url-append "$TEXMACS_FILE_PATH" name))
      ) ;and
    (set! name (url-resolve (url-append "$TEXMACS_FILE_PATH" name) "f"))
  ) ;if
  (if (not (url-rooted? name))
    (if (current-buffer)
      (set! name (url-relative (current-buffer) name))
      (set! name (url-append (url-pwd) name))
    ) ;if
  ) ;if
  (if (== (url-suffix name) "pdf")
    (load-pdf-buffer name)
    (load-buffer-check-autosave name opts)
  ) ;if
) ;tm-define

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
  (:default name (propose-name-buffer))
  ;; (display* "load-buffer " name ", " opts "\n")
  (apply load-buffer-main (cons name opts))
) ;tm-define

(tm-define (load-buffer-in-new-window name . opts)
  (:argument name smart-file "File name")
  (:default name (propose-name-buffer))
  (if (buffer->window name)
    (noop)
    ;; (window-focus (buffer->window name))
    (apply load-buffer-main (cons name (cons :new-window opts)))
  ) ;if
) ;tm-define

(tm-define (load-browse-buffer name)
  (:synopsis "Load a buffer or switch to it if already open")
  (cond ((buffer-exists? name) (switch-to-buffer name))
        ((== (url-suffix name) "pdf") (load-pdf-buffer name))
        ((and (buffer-external? name)
           (!= (url-suffix name) "tm")
           (!= (url-suffix name) "tmu")
         ) ;and
         (load-external name)
        ) ;
        ((url-rooted-web? name)
         ;; Show wait dialog during remote file loading
         (system-wait "Loading remote file" (url->system name))
         (load-buffer name)
        ) ;
        ((url-rooted-web? (current-buffer)) (load-buffer name))
        (else (load-buffer name))
  ) ;cond
) ;tm-define

(tm-define (open-buffer)
  (:synopsis "Open a new file")
  (with-default-view (choose-file load-buffer "Load file" "action_open"))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Reverting buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (revert-buffer-revert . l)
  (with name
    (if (null? l) (current-buffer) (car l))
    (if (not (buffer-exists? name))
      (load-buffer name)
      (begin
        (when (!= name (current-buffer))
          (switch-to-buffer name)
        ) ;when
        (url-cache-invalidate name)
        (with t
          (tree-import name (url-format name))
          (if (== t (tm->tree "error"))
            (set-message "Error: file not found" "Revert buffer")
            (buffer-set name t)
          ) ;if
        ) ;with
      ) ;begin
    ) ;if
  ) ;with
) ;tm-define

(tm-define (revert-buffer . l)
  (with name
    (if (null? l) (current-buffer) (car l))
    (if (and (buffer-exists? name) (buffer-modified? name))
      (user-confirm "Buffer has been modified. Really revert?"
        #f
        (lambda (answ) (when answ (apply revert-buffer-revert l)))
      ) ;user-confirm
      (apply revert-buffer-revert l)
    ) ;if
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Importing buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (import-buffer-import name fm opts)
  ;; (display* "import-buffer-import " name ", " fm "\n")
  (if (== fm (url-format name))
    (apply load-buffer-main (cons name opts))
    (let* ((s (url->tmfs-string name)) (u (string-append "tmfs://import/" fm "/" s)))
      (apply load-buffer-main (cons u opts))
    ) ;let*
  ) ;if
) ;define

(define (import-buffer-check-permissions name fm opts)
  ;; (display* "import-buffer-check-permissions " name ", " fm "\n")
  (with vname
    `(verbatim ,(utf8->cork (url->system name)))
    (cond ((not (url-test? name "f"))
           (with msg
             `(concat ,"The file " ,vname ," does not exist")
             (set-message msg "Import file")
           ) ;with
          ) ;
          ((not (url-test? name "r"))
           (with msg
             `(concat ,(translate "You do not have read access to") ," " ,vname)
             (show-message msg "Import file")
           ) ;with
          ) ;
          (else (import-buffer-import name fm opts))
    ) ;cond
  ) ;with
) ;define

(tm-define (import-buffer-main name fm opts)
  ;; (display* "import-buffer-main " name ", " fm "\n")
  (if (and (not (url-exists? name))
        (url-exists? (url-append "$TEXMACS_FILE_PATH" name))
      ) ;and
    (set! name (url-resolve (url-append "$TEXMACS_FILE_PATH" name) "f"))
  ) ;if
  (import-buffer-check-permissions name fm opts)
) ;tm-define

(tm-define (import-buffer name fm . opts)
  (if (window-per-buffer?)
    (import-buffer-main name fm (cons :new-window opts))
    (import-buffer-main name fm opts)
  ) ;if
) ;tm-define

(tm-define (buffer-importer fm) (lambda (s) (import-buffer s fm)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; System dependent conventions for buffer management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-in-window)
  (choose-file load-buffer-in-new-window "Load file" "action_open")
) ;tm-define

(tm-define (open-document)
  (if (window-per-buffer?) (open-in-window) (open-buffer))
) ;tm-define

(tm-define (open-document*)
  (if (window-per-buffer?) (open-buffer) (open-in-window))
) ;tm-define

(tm-define (load-document u)
  (:argument u smart-file "File name")
  (:default u (propose-name-buffer))
  (when (not (url-none? u))
    (if (== (url-suffix u) "pdf")
      (load-pdf-buffer u)
      (if (window-per-buffer?) (load-buffer-in-new-window u) (load-buffer u))
    ) ;if
  ) ;when
) ;tm-define

(tm-define (load-document* u)
  (:argument u smart-file "File name")
  (:default u (propose-name-buffer))
  (when (not (url-none? u))
    (if (== (url-suffix u) "pdf")
      (load-pdf-buffer u)
      (if (window-per-buffer?) (load-buffer u) (load-buffer-in-new-window u))
    ) ;if
  ) ;when
) ;tm-define

(tm-define (switch-document u)
  (:argument u smart-file "File name")
  (:default u (propose-name-buffer))
  (when (not (url-none? u))
    (if (window-per-buffer?)
      (if (buffer->window u)
        (noop)
        ;; (window-focus (buffer->window u))
        (open-buffer-in-window u (buffer-get u) "")
      ) ;if
      (load-buffer u)
    ) ;if
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Printing buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (interactive-page-setup)
  (:synopsis "Specify the page setup")
  (:interactive #t)
  (set-message "Not yet implemented" "Printer setup")
) ;tm-define

(tm-define (direct-print-buffer) (:synopsis "Print the current buffer") (print))

(tm-define (interactive-print-buffer)
  (:synopsis "Print the current buffer")
  (:interactive #t)
  (print-to-file "$TEXMACS_HOME_PATH/system/tmp/tmpprint.ps")
  (interactive-print '() "$TEXMACS_HOME_PATH/system/tmp/tmpprint.ps")
) ;tm-define

(tm-define (print-buffer)
  (:synopsis "Print the current buffer")
  (:interactive (use-print-dialog?))
  (if (use-print-dialog?) (interactive-print-buffer) (direct-print-buffer))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Important files to which the buffer is linked (e.g. bibliographies)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (linked-files-inside t)
  (cond ((tree-atomic? t) (list))
        ((tree-is? t 'document) (append-map linked-files-inside (tree-children t)))
        ((tree-in? t '(with with-bib)) (linked-files-inside (tm-ref t :last)))
        ((or (tree-func? t 'bibliography 4) (tree-func? t 'bibliography* 5))
         (with name
           (tm->stree (tm-ref t 2))
           (if (or (== name "") (nstring? name))
             (list)
             (with s
               (if (string-ends? name ".bib") name (string-append name ".bib"))
               (list (url-relative (current-buffer) s))
             ) ;with
           ) ;if
         ) ;with
        ) ;
        (else (list))
  ) ;cond
) ;define

(tm-define (linked-file-list) (linked-files-inside (buffer-tree)))
