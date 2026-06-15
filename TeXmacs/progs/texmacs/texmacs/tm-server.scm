
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-server.scm
;; DESCRIPTION : server wide properties and resource management
;; COPYRIGHT   : (C) 2001  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs texmacs tm-server) (:use (generic document-edit)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (get-default-interactive-questions)
  "popup"
) ;define

(define (get-default-buffer-management)
  "shared"
) ;define

(define (notify-buffer-management var val)
  (when (== val (get-default-buffer-management))
    (reset-preference "buffer management")
  ) ;when
) ;define

(define (get-default-show-table-cells)
  (if (qt-gui?) "on" "off")
) ;define

(define (notify-look-and-feel var val)
  (set-message "Restart in order to let the new look and feel take effect"
    "configure look and feel"
  ) ;set-message
) ;define

(define (notify-gui-theme var val)
  (set-message "Restart in order to let the new theme take effect"
    "graphical interface theme"
  ) ;set-message
) ;define

(define (notify-language var val)
  (set-output-language val)
  (if (and (current-view) (== (buffer-tree) (stree->tree '(document ""))))
    (set-document-language val)
  ) ;if
  (cond ((or (== val "bulgarian") (== val "russian") (== val "ukrainian"))
         (notify-preference "cyrillic input method")
        ) ;
  ) ;cond
) ;define

(define (notify-scripting-language var val)
  (if (current-view)
    (if (== val "none") (init-default "prog-scripts") (init-env "prog-scripts" val))
  ) ;if
) ;define

(define (notify-security var val)
  (cond ((== val "accept no scripts") (set-script-status 0))
        ((== val "prompt on scripts") (set-script-status 1))
        ((== val "accept all scripts") (set-script-status 2))
  ) ;cond
) ;define

(define (notify-bibtex-command var val)
  (if (use-plugin-bibtex?) (set-bibtex-command val))
) ;define

(define (notify-tool var val)
  ;; FIXME: the menus sometimes don't get updated,
  ;; but the fix below does not work
  (if (current-view) (notify-change 1))
) ;define

(define (notify-new-fonts var val)
  (set-new-fonts (== val "on"))
) ;define

(define (notify-fast-environments var val)
  (set-fast-environments (== val "on"))
) ;define

(define (notify-new-page-breaking var val)
  (noop)
) ;define

(define (get-default-native-menubar)
  (if (qt4-gui?) "on" "off")
) ;define

(define (get-default-unified-toolbar)
  (if (qt4-gui?) "on" "off")
) ;define

(define-preferences ("profile" "beginner" (lambda args (noop)))
 ("look and feel" "default" notify-look-and-feel)
 ("case sensitive shortcuts" "default" noop)
 ("detailed menus" "detailed" noop)
 ("buffer management" (get-default-buffer-management) notify-buffer-management)
 ("complex actions" "popups" noop)
 ("interactive questions" (get-default-interactive-questions) noop)
 ("language" (get-locale-language) notify-language)
 ("gui theme" "liii" notify-gui-theme)
 ("completion style" "popup" noop)
 ("page medium" "papyrus" (lambda args (noop)))
 ("page screen margin" "false" (lambda args (noop)))
 ("fast environments" "on" notify-fast-environments)
 ("show full context" "on" (lambda args (noop)))
 ("show table cells" (get-default-show-table-cells) (lambda args (noop)))
 ("show focus" "on" (lambda args (noop)))
 ("show only semantic focus" "on" (lambda args (noop)))
 ("semantic editing" "off" (lambda args (noop)))
 ("semantic selections" "on" (lambda args (noop)))
 ("semantic correctness" "off" (lambda args (noop)))
 ("remove superfluous invisible" "off" (lambda args (noop)))
 ("insert missing invisible" "off" (lambda args (noop)))
 ("zealous invisible correct" "off" (lambda args (noop)))
 ("homoglyph correct" "off" (lambda args (noop)))
 ("manual remove superfluous invisible" "on" (lambda args (noop)))
 ("manual insert missing invisible" "on" (lambda args (noop)))
 ("manual zealous invisible correct" "off" (lambda args (noop)))
 ("manual homoglyph correct" "on" (lambda args (noop)))
 ("security" "prompt on scripts" notify-security)
 ("bibtex command" "bibtex" notify-bibtex-command)
 ("scripting language" "none" notify-scripting-language)
 ("speech" "off" noop)
 ("database tool" "off" notify-tool)
 ("debugging tool" "off" notify-tool)
 ("developer tool" "off" notify-tool)
 ("linking tool" "off" notify-tool)
 ("presentation tool" "off" notify-tool)
 ("remote tool" "off" notify-tool)
 ("source tool" "off" notify-tool)
 ("versioning tool" "off" notify-tool)
 ("experimental alpha" "on" notify-tool)
 ("bitmap effects" "on" notify-tool)
 ("new style page breaking" "on" notify-new-page-breaking)
 ("open console on errors" "on" noop)
 ("open console on warnings" "off" noop)
 ("gui:line-input:autocommit" "on" noop)
 ("use native menubar" (get-default-native-menubar) noop)
 ("use unified toolbar" (get-default-unified-toolbar) noop)
 ("texmacs->image:format" "png" noop)
 ("autobackup" "on" noop)
) ;define-preferences

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Properties of some built-in routines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-property (system cmd) (:argument cmd "System command"))

(tm-property (footer-eval cmd) (:argument cmd "Scheme command"))

(define (symbol<=? s1 s2)
  (string<=? (symbol->string s1) (symbol->string s2))
) ;define

(define (get-function-list)
  (list-sort (map car (ahash-table->list tm-defined-table)) symbol<=?)
) ;define

(define (get-interactive-function-list)
  (let* ((funs (get-function-list)) (pred? (lambda (fun) (property fun :arguments))))
    (list-filter funs pred?)
  ) ;let*
) ;define

(tm-define (exec-interactive-command cmd)
  (:argument cmd "Interactive command")
  (:proposals cmd (cons "" (map symbol->string (get-interactive-function-list))))
  (interactive (eval (string->symbol cmd)))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Killing buffers, windows and TeXmacs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (confirm-close-widget cmd buffer-name)
  (resize "500guipx"
    "200guipx"
    (centered (glue #t #f 150 6)
      (text (if buffer-name
              `(verbatim ,(string-append (cork->utf8 (translate "Save change to"))
                            "「 "
                            buffer-name
                            " 」?"))
              `(verbatim ,(string-append (cork->utf8 (translate "Save scratch buffer"))
                            "?"))
            ) ;if
      ) ;text
      (glue #t #f 150 6)
    ) ;centered
    (bottom-buttons >>
      (assuming buffer-name ("Save" (cmd "Save")))
      (assuming (not buffer-name) ("Save as" (cmd "Save")))
      //
      ("Don't save" (cmd "Don't save"))
      //
      ("Cancel" (cmd "Cancel"))
      ///
    ) ;bottom-buttons
  ) ;resize
) ;tm-widget

(define (confirm-close-dialog prompt on-save on-dont-save . opt-buffer)
  (let ((buffer (if (null? opt-buffer) (current-buffer) (car opt-buffer))))
    (dialogue-window (lambda (cmd)
                       (confirm-close-widget cmd
                         (if (or (url-scratch? buffer) (url-rooted-tmfs? buffer))
                           #f
                           (buffer-get-title buffer)
                         ) ;if
                       ) ;confirm-close-widget
                     ) ;lambda
      (lambda (answer)
        (cond ((== answer "Save")
               (if (or (url-scratch? buffer) (url-rooted-tmfs? buffer))
                 (choose-file (lambda (x) (save-buffer-as-simple x buffer (list :overwrite)) (on-save))
                   "Save TeXmacs file"
                   "tmu"
                 ) ;choose-file
                 (begin
                   (if (not (buffer-save buffer)) (on-save))
                 ) ;begin
               ) ;if
              ) ;
              ((== answer "Don't save") (on-dont-save))
              (else #f)
        ) ;cond
      ) ;lambda
      "Save buffer?"
    ) ;dialogue-window
  ) ;let
) ;define

(tm-define (buffer-close name) (cpp-buffer-close name))

(tm-define (buffers-modified?) (list-or (map buffer-modified? (buffer-list))))

(tm-define (safely-kill-buffer)
  (cond ((buffer-embedded? (current-buffer))
         (alt-windows-delete (alt-window-search (current-buffer)))
        ) ;
        ((buffer-modified? (current-buffer))
         (confirm-close-dialog "The document has not been saved. Really close it?"
           (lambda () (buffer-close (current-buffer)))
           (lambda () (buffer-close (current-buffer)))
         ) ;confirm-close-dialog
        ) ;
        (else (buffer-close (current-buffer)))
  ) ;cond
) ;tm-define

(tm-define (safely-kill-tabpage)
  (let* ((tgt-view (current-view))
         (tgt-win (current-window))
         (tgt-buffer (view->buffer tgt-view))
        ) ;
    (cond ((and (auxiliary-widget-visible?) (not (buffer-embedded? tgt-buffer)))
           (show-message "Please close the auxiliary window first" "Notification")
          ) ;
          ((buffer-embedded? tgt-buffer)
           (alt-windows-delete (alt-window-search tgt-buffer))
          ) ;
          ((buffer-modified? tgt-buffer)
           (confirm-close-dialog "The document has not been saved. Really close it?"
             (lambda () (kill-tabpage tgt-win tgt-view))
             (lambda () (kill-tabpage tgt-win tgt-view))
             tgt-buffer
           ) ;confirm-close-dialog
          ) ;
          (else (kill-tabpage tgt-win tgt-view))
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (safely-kill-tabpage-by-url tgt-win tgt-view tgt-buffer)
  (cond ((and (auxiliary-widget-visible?) (not (buffer-embedded? tgt-buffer)))
         (show-message "Please close the auxiliary window first" "Notification")
        ) ;
        ((buffer-embedded? tgt-buffer)
         (alt-windows-delete (alt-window-search tgt-buffer))
        ) ;
        ((buffer-modified? tgt-buffer)
         (confirm-close-dialog "The document has not been saved. Really close it?"
           (lambda () (kill-tabpage tgt-win tgt-view))
           (lambda () (kill-tabpage tgt-win tgt-view))
           tgt-buffer
         ) ;confirm-close-dialog
        ) ;
        (else (kill-tabpage tgt-win tgt-view))
  ) ;cond
) ;tm-define

(define (do-kill-window)
  (with buf
    (current-buffer)
    (kill-window (current-window))
    (delayed (:idle 100) (lambda () (buffer-close buf)))
  ) ;with
) ;define

(define (do-kill-window* u)
  (with buf
    (window->buffer u)
    (kill-window u)
    (delayed (:idle 100) (lambda () (buffer-close buf)))
  ) ;with
) ;define

(tm-define (safely-kill-window . opt-name)
  (cond ((and (buffer-embedded? (current-buffer)) (null? opt-name))
         (alt-windows-delete (alt-window-search (current-buffer)))
        ) ;
        ((<= (windows-number) 1) (safely-quit-TeXmacs))
        ((nnull? opt-name)
         (if (buffer-modified? (window->buffer (car opt-name)))
           (confirm-close-dialog "The document has not been saved. Really close it?"
             (lambda () (do-kill-window* (car opt-name)))
             (lambda () (do-kill-window* (car opt-name)))
             (window->buffer (car opt-name))
           ) ;confirm-close-dialog
           (do-kill-window* (car opt-name))
         ) ;if
        ) ;
        ((buffer-modified? (current-buffer))
         (confirm-close-dialog "The document has not been saved. Really close it?"
           (lambda () (do-kill-window))
           (lambda () (do-kill-window))
         ) ;confirm-close-dialog
        ) ;
        (else (do-kill-window))
  ) ;cond
) ;tm-define

(tm-define (safely-quit-TeXmacs)
  (let* ((m (filter buffer-modified? (buffer-list))) (l (filter (non buffer-aux?) m)))
    (if (null? l)
      (quit-TeXmacs)
      (begin
        (when (nin? (current-buffer) l)
          ;; FIXME: focus on window with buffer, if any
          (switch-to-buffer (car l))
        ) ;when
        (confirm-close-dialog "There are unsaved documents. Really quit?"
          (lambda () (save-all-buffers) (quit-TeXmacs))
          (lambda () (quit-TeXmacs))
        ) ;confirm-close-dialog
      ) ;begin
    ) ;if
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; System dependent conventions for buffer management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; new-document
;; 按默认窗口策略创建一个新文档，并为新建 buffer 绑定 stem-doc-id。
;;
;; 逻辑
;; ----
;; 先确保命令运行在 default view 中，再根据 window-per-buffer? 决定创建
;; 新窗口或新 buffer。new-buffer/open-window 会同步切换 current buffer，
;; 因此可以直接对当前 buffer 绑定 doc id，不需要再额外等待 idle。
;;
;; 与 new-document* 的区别
;; ----
;; new-document 遵循 window-per-buffer? 偏好；new-document* 使用互补策略。
;; 两个入口都会产生一个新的当前文档，因此都需要绑定 doc id。虽然保存和
;; 备份前还有兜底检查，这里显式绑定可以保证两种新建路径从创建起就有
;; 一致的自动备份标识。
;;
;; 注意
;; ----
;; auto-backup-ensure-buffer-doc-id! 只写 init-env，避免触发文档树替换和
;; 菜单/工具栏重新解析；doc id 会在用户保存时随文档持久化。
(tm-define (new-document)
  (with-default-view (if (window-per-buffer?) (open-window) (new-buffer))
    (auto-backup-ensure-buffer-doc-id! (current-buffer))
  ) ;with-default-view
) ;tm-define

;; new-document*
;; 使用与 new-document 互补的窗口策略创建一个新文档。
;;
;; 逻辑
;; ----
;; 在 window-per-buffer? 模式下复用当前窗口创建新 buffer；否则打开新窗口。
;; 这是 new-document 的备用新建路径，但同样会产生新的当前文档，所以也要
;; 立即为当前 buffer 绑定自动备份 doc id，避免这条路径和默认路径表现不一。
;;
;; 注意
;; ----
;; 这里同样不需要 idle 延迟：创建新 buffer 或新窗口后 current buffer 已经
;; 指向新文档，doc id 绑定只作用于当前会话的 init-env。
(tm-define (new-document*)
  (with-default-view (if (window-per-buffer?) (new-buffer) (open-window))
    (auto-backup-ensure-buffer-doc-id! (current-buffer))
  ) ;with-default-view
) ;tm-define

(tm-define (close-document)
  (with-default-view (if (window-per-buffer?) (safely-kill-window) (safely-kill-tabpage))
  ) ;with-default-view
) ;tm-define

(tm-define (close-document*)
  (with-default-view (if (window-per-buffer?) (safely-kill-tabpage) (safely-kill-window))
  ) ;with-default-view
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 保存相关的辅助函数和接口
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 检查文件写入权限

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
           (with msg
             "You do not have write access for:"
             (notify-now `(concat ,msg ,"<br>" ,vname))
           ) ;with
           #t
          ) ;
          (else #f)
    ) ;cond
  ) ;with
) ;define

;; 执行实际保存操作

(define (save-buffer-save name opts . opt-callback)
  (let ((callback (if (null? opt-callback) (lambda (success) (noop)) (car opt-callback))
        ) ;callback
       ) ;
    (with vname
      `(verbatim ,(utf8->cork (url->system name)))
      (if (buffer-save name)
        (begin
          (buffer-pretend-modified name)
          (set-message `(concat ,"Could not save " ,vname) "Save file")
          (callback #f)
        ) ;begin
        (begin
          (if (== (url-suffix name) "ts") (style-clear-cache))
          (set-message `(concat ,"Saved " ,vname) "Save file")
          (save-buffer-post name opts)
          (callback #t)
        ) ;begin
      ) ;if
    ) ;with
  ) ;let
) ;define

;; 保存后的处理

(define (save-buffer-post name opts)
  (cond ((in? :update opts) (update-buffer name))
        ((in? :commit opts) (commit-buffer name))
  ) ;cond
) ;define

;; 保存接口

(define (save-buffer-as-simple new-name name opts)
  (cond ((cannot-write? new-name "Save file") #f)
        ((and (url-test? new-name "f") (nin? :overwrite opts))
         (user-confirm "File already exists. Really overwrite?"
           #f
           (lambda (answ) (when answ (save-buffer-as-simple-continue new-name name opts)))
         ) ;user-confirm
        ) ;
        (else (save-buffer-as-simple-continue new-name name opts))
  ) ;cond
) ;define

(define (save-buffer-as-simple-continue new-name name opts)
  (cond ((buffer-exists? new-name)
         (with s
           (string-append "The file "
             (url->system new-name)
             " is being edited. Discard edits?"
           ) ;string-append
           (user-confirm s
             #f
             (lambda (answ) (when answ (save-buffer-as-simple-save new-name name opts)))
           ) ;user-confirm
         ) ;with
        ) ;
        (else (save-buffer-as-simple-save new-name name opts))
  ) ;cond
) ;define

(define (save-buffer-as-simple-save new-name name opts)
  (if (and (url-scratch? name) (url-exists? name)) (system-remove name))
  (buffer-rename name new-name)
  (buffer-pretend-modified new-name)
  (save-buffer-save new-name opts (lambda (success) (noop)))
) ;define
