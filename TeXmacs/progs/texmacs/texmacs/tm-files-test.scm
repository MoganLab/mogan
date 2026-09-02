
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
  (:use (texmacs texmacs tm-files) (autosave plugin))
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
  (let* ((body (substring name 6 (- (string-length name) 4)))
         (cut (or (string-index body #\-) (string-length body)))
        ) ;
    (substring body 0 cut)
  ) ;let*
) ;define

(define (test-scratch-buffer-name-has-date-time-underscore)
  (let* ((path (scratch-buffer-name))
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
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-tm-files)
  (test-auto-backup-official-url)
  (test-auto-backup-texmacs-path-buffer?)
  (test-scratch-buffer-name-has-date-time-underscore)
  (test-scratch-buffer-title-old-and-new-stamp)
  (test-scratch-buffer-title-legacy-one-underscore-this-week)
  (check-report)
) ;tm-define
