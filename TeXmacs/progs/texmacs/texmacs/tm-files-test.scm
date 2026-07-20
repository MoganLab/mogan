
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
  (:use (texmacs texmacs tm-files) (plugin autosave))
) ;texmacs-module

(import (liii check))
(import (only (liii path) path-join path->string))

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
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-tm-files)
  (test-auto-backup-official-url)
  (test-auto-backup-texmacs-path-buffer?)
  (check-report)
) ;tm-define
