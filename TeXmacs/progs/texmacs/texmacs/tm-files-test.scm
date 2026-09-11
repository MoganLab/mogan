
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-files-test.scm
;; DESCRIPTION : test suite for file handling helpers
;; COPYRIGHT   : (C) 2026  LiiiSTEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs texmacs tm-files-test)
  (:use (texmacs texmacs tm-files) (autosave plugin) (utils library cursor))
) ;texmacs-module

(import (liii check))
(import (only (liii path) path-join path->string))
(import (only (liii string) string-starts? string-ends? string-index string-contains?)
) ;import
(import (only (liii time) current-date date->string))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for auto-backup-official-url
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-auto-backup-official-url)
  (check (in? (auto-backup-official-url)
           '("https://liiistem.cn/personal-center/backup.html?utm_source=auto_backup_button"
             "https://liiistem.com/?utm_source=auto_backup_button")
         ) ;in?
    =>
    #t
  ) ;check
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for auto-backup-texmacs-path-buffer?
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-auto-backup-texmacs-path-buffer?)
  ;; TeXmacs 安装路径下的文件被视为只读资源，应当被跳过
  (check (auto-backup-texmacs-path-buffer? (system->url (path->string (path-join (url->system (get-texmacs-path)) "progs" "test.tmu"))
                                           ) ;system->url
         ) ;auto-backup-texmacs-path-buffer?
    =>
    #t
  ) ;check
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for draft_YYYYMMDD_HHMMSS naming (1257)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (draft-test-url name)
  (system->url name)
) ;define

(define (draft-name-stamp-part name)
  (let* ((dot (string-rindex name #\.))
         (body (substring name 6 dot))
         (cut (or (string-index body #\-) (string-length body)))
        ) ;
    (substring body 0 cut)
  ) ;let*
) ;define

(define (test-scratch-buffer-name-has-date-time-underscore)
  (let* ((path (scratch-buffer-name ".tmu"))
         (name (url->string (url-tail (system->url path))))
         (stamp (draft-name-stamp-part name))
        ) ;
    (check (string-starts? name "draft_") => #t)
    (check (string-ends? name ".tmu") => #t)
    ;; YYYYMMDD_HHMMSS → 长度 15,第 9 个字符是 _
    (check (string-length stamp) => 15)
    (check (substring stamp 8 9) => "_")
  ) ;let*
) ;define

(define (test-scratch-buffer-name-stem)
  (let* ((path (scratch-buffer-name ".stem"))
         (name (url->string (url-tail (system->url path))))
         (stamp (draft-name-stamp-part name))
        ) ;
    (check (string-starts? name "draft_") => #t)
    (check (string-ends? name ".stem") => #t)
    (check (string-length stamp) => 15)
    (check (substring stamp 8 9) => "_")
  ) ;let*
) ;define

(define (test-scratch-buffer-title-stem)
  (let ((tmu-title (scratch-buffer-title (draft-test-url "draft_20250802_153000.tmu")))
        (stem-title (scratch-buffer-title (draft-test-url "draft_20250802_153000.stem"))
        ) ;stem-title
        (stem-n-title (scratch-buffer-title (draft-test-url "draft_20250802_153000-1.stem"))
        ) ;stem-n-title
       ) ;
    (check stem-title => tmu-title)
    (check stem-n-title => tmu-title)
  ) ;let
) ;define

(define (test-scratch-buffer-title-old-and-new-stamp)
  ;; 往年草稿不显示时刻,新旧文件名必须得到同一标题
  (let ((old (scratch-buffer-title (draft-test-url "draft_20250802153000.tmu")))
        (new (scratch-buffer-title (draft-test-url "draft_20250802_153000.tmu")))
        (new-n (scratch-buffer-title (draft-test-url "draft_20250802_153000-1.tmu")))
       ) ;
    (check old => new)
    (check new => new-n)
  ) ;let
) ;define

(define (test-scratch-buffer-title-legacy-one-underscore-this-week)
  ;; 本周旧名只有 draft_ 一个下划线;标题须含文件名里的 HH:MM:SS
  (let* ((now (date->string (current-date) "~Y~m~d~H~M~S"))
         (day (substring now 0 8))
         (hms (substring now 8 14))
         (clock (string-append (substring hms 0 2)
                  ":"
                  (substring hms 2 4)
                  ":"
                  (substring hms 4 6)
                ) ;string-append
         ) ;clock
         (old (scratch-buffer-title (draft-test-url (string-append "draft_" now ".tmu")))
         ) ;old
         (new (scratch-buffer-title (draft-test-url (string-append "draft_" day "_" hms ".tmu"))
              ) ;scratch-buffer-title
         ) ;new
        ) ;
    (check old => new)
    (check (string-contains? old clock) => #t)
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for style-buffer? and stem-doc-id eligibility
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-style-buffer-and-doc-id)
  ;; 1. .ts 后缀 buffer 属于样式 buffer，不生成 stem-doc-id
  (let* ((orig (current-buffer))
         (buf (new-buffer ".tmu"))
         (ts-url (system->url "/tmp/test_custom_style.ts"))
        ) ;
    (buffer-rename buf ts-url)
    (check (style-buffer? ts-url) => #t)
    (check (document-buffer? ts-url) => #f)
    (check (auto-backup-buffer-eligible? ts-url) => #f)
    (check (auto-backup-ensure-buffer-doc-id! ts-url) => #f)
    (buffer-close ts-url)
    (switch-to-buffer orig)
  ) ;let*

  ;; 2. 实际为样式的 .stem buffer（style 为 source）不生成 stem-doc-id
  (let* ((orig (current-buffer))
         (stem-style-buf (new-buffer ".stem"))
         (style-doc '(document (TeXmacs "2.1.4")
                       (style (tuple "source"))
                       (body (document (active* (src-title (document (src-package "my-pkg"
                                                                       "1.0")))))))
         ) ;style-doc
        ) ;
    (buffer-set stem-style-buf style-doc)
    (check (style-buffer? stem-style-buf) => #t)
    (check (document-buffer? stem-style-buf) => #f)
    (check (auto-backup-buffer-eligible? stem-style-buf) => #f)
    (check (auto-backup-ensure-buffer-doc-id! stem-style-buf) => #f)
    ;; 验证若环境中已存在 doc-id，clear 会将其清除
    (with-buffer stem-style-buf (init-env "stem-doc-id" "test-uuid"))
    (auto-backup-clear-buffer-doc-id! stem-style-buf)
    (with-buffer stem-style-buf (check (get-init-env "stem-doc-id") => #f))
    (check (auto-backup-buffer-doc-id stem-style-buf) => #f)
    (buffer-close stem-style-buf)
    (switch-to-buffer orig)
  ) ;let*

  ;; 3. 普通文档 .stem buffer（style 为 generic）属于文档 buffer，生成 stem-doc-id
  (let* ((orig (current-buffer))
         (stem-doc-buf (new-buffer ".stem"))
         (doc '(document (TeXmacs "2.1.4")
                 (style (tuple "generic"))
                 (body (document "Hello STEM document")))
         ) ;doc
        ) ;
    (buffer-set stem-doc-buf doc)
    (check (style-buffer? stem-doc-buf) => #f)
    (check (document-buffer? stem-doc-buf) => #t)
    (check (auto-backup-buffer-eligible? stem-doc-buf) => #t)
    (let ((doc-id (auto-backup-ensure-buffer-doc-id! stem-doc-buf)))
      (check (string? doc-id) => #t)
      (check (!= doc-id "") => #t)
      (check (auto-backup-buffer-doc-id stem-doc-buf) => doc-id)
    ) ;let
    (buffer-close stem-doc-buf)
    (switch-to-buffer orig)
  ) ;let*

  ;; 4. 普通文档 .tmu 属于文档 buffer，生成 stem-doc-id
  (let* ((orig (current-buffer))
         (tmu-buf (new-buffer ".tmu"))
         (doc '(document (TeXmacs "2.1.4")
                 (style (tuple "generic"))
                 (body (document "Hello TMU document")))
         ) ;doc
        ) ;
    (buffer-set tmu-buf doc)
    (check (style-buffer? tmu-buf) => #f)
    (check (document-buffer? tmu-buf) => #t)
    (check (auto-backup-buffer-eligible? tmu-buf) => #t)
    (let ((doc-id (auto-backup-ensure-buffer-doc-id! tmu-buf)))
      (check (string? doc-id) => #t)
      (check (!= doc-id "") => #t)
      (check (auto-backup-buffer-doc-id tmu-buf) => doc-id)
    ) ;let
    (buffer-close tmu-buf)
    (switch-to-buffer orig)
  ) ;let*

  ;; 5. 导出样式包生成的草稿 buffer 属于样式 buffer，不生成 stem-doc-id
  (let ((orig-buf (current-buffer)))
    (when (defined? 'extract-style-package)
      (extract-style-package)
      (let ((style-draft (current-buffer)))
        (check (style-buffer? style-draft) => #t)
        (check (document-buffer? style-draft) => #f)
        (check (auto-backup-buffer-eligible? style-draft) => #f)
        (check (auto-backup-ensure-buffer-doc-id! style-draft) => #f)
        (buffer-close style-draft)
        (switch-to-buffer orig-buf)
      ) ;let
    ) ;when
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-tm-files)
  (test-auto-backup-official-url)
  (test-auto-backup-texmacs-path-buffer?)
  (test-scratch-buffer-name-has-date-time-underscore)
  (test-scratch-buffer-name-stem)
  (test-scratch-buffer-title-stem)
  (test-scratch-buffer-title-old-and-new-stamp)
  (test-scratch-buffer-title-legacy-one-underscore-this-week)
  (test-style-buffer-and-doc-id)
  (check-report)
) ;tm-define
