
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
  (:use (texmacs texmacs tm-files))
) ;texmacs-module

(import (only (liii path) path-join))

(define (regtest-auto-backup-official-url)
  (regression-test-group "auto-backup"
    "official url"
    (lambda (case
            ) ;case
      (and (== case "utm")
        (in? (auto-backup-official-url)
          '("https://liiistem.cn/personal-center/backup.html?utm_source=auto_backup_button"
            "https://liiistem.com/?utm_source=auto_backup_button")
        ) ;in?
      ) ;and
    ) ;lambda
    :none
    (test "utm source" "utm" #t)
  ) ;regression-test-group
) ;define

(define (regtest-auto-backup-texmacs-path)
  (regression-test-group "auto-backup"
    "texmacs path is read-only"
    (lambda (case
            ) ;case
      (and (== case "inside")
        (auto-backup-texmacs-path-buffer? (system->url (path-join (url->system (get-texmacs-path)) "progs" "test.tmu"))
        ) ;auto-backup-texmacs-path-buffer?
      ) ;and
    ) ;lambda
    :none
    (test "skip texmacs path" "inside" #t)
  ) ;regression-test-group
) ;define

(tm-define (regtest-tm-files)
  (let ((n (+ (regtest-auto-backup-official-url) (regtest-auto-backup-texmacs-path))))
    (display* "Total: " (object->string n) " tests.\n")
    (display "Test suite of tm-files: ok\n")
  ) ;let
) ;tm-define
